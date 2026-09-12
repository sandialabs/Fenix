/*
//@HEADER
// ************************************************************************
//
//
//            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
//            _|        _|        _|_|    _|    _|      _|  _|
//            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
//            _|        _|        _|    _|_|    _|      _|  _|
//            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|
//
//
//
//
// Copyright (C) 2016 Rutgers University and Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Rob Van der Wijngaart, Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <assert.h>
#include <sys/time.h>
#include <chrono>
#include <thread>

#include <mpi.h>
#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif

#include "fenix_ext.hpp"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"

namespace fenix {

static int __fenix_repair_ranks();
static void __fenix_test_MPI(MPI_Comm*, int*, ...);
static void spare_rank_loop();
static void __fenix_finalize_spare();

// Rebuilds fenix_rt Group members using latest fenix_rt world.
// Updates pid_to_rank and rank_to_pid. Does not change any comms.
static void rebuild_proc_groups();

// Attempts to build new_world/user_world and returns FENIX_SUCCESS consistently
// if successful. Uses existing proc groups and world.
static int try_build_active_worlds();

// Returns true if this process is a spare
static bool spare() { return fenix_rt.spare_procs.rank() != MPI_UNDEFINED; }

static int preinit(
  const args::FenixInitArgs& args, jmp_buf* jump_env = nullptr
) {
  fenix_rt.finalized = false;

  // Initialize universe from input communicator before shrinking
  fenix_rt.procs = mpixx::Comm::group(args.in_comm);

  int n_active         = fenix_rt.procs.size() - args.spares;
  fenix_rt.user_procs  = fenix_rt.procs.range_incl({{0, n_active - 1, 1}});
  fenix_rt.spare_procs = fenix_rt.procs - fenix_rt.user_procs;

  fenix_rt.pid_to_rank = std::vector<int>(fenix_rt.procs.size(), MPI_UNDEFINED);
  fenix_rt.rank_to_pid = std::vector<int>(n_active, MPI_UNDEFINED);
  for (int i = 0; i < n_active; i++) {
    fenix_rt.pid_to_rank[i] = fenix_rt.rank_to_pid[i] = i;
  }

  MPI_Comm_create_errhandler(__fenix_test_MPI, &fenix_rt.mpi_errhandler);

  fenix_rt.user_world_ptr      = args.out_comm;
  fenix_rt.spare_ranks         = args.spares;
  fenix_rt.recover_environment = jump_env;

  fenix_rt.ret_role  = args.role ? args.role : &fenix_rt.role;
  fenix_rt.ret_error = args.err ? args.err : &fenix_rt.repair_result;

  *fenix_rt.ret_role  = fenix_rt.role;
  *fenix_rt.ret_error = FENIX_SUCCESS;

  fenix_rt.settings = fenix_default_settings;
  if (fenix_rt.settings.resume == FENIX_RESUME_MODE_MAXCODE) {
    fenix_rt.settings.resume = jump_env ? JUMP : THROW;
  }
  fenix_assert(
    fenix_rt.settings.resume != JUMP || jump_env != nullptr,
    "Must use Fenix_Init to use FENIX_RESUME_JUMP"
  );

  MPI_Op_create((MPI_User_function*)__fenix_ranks_agree, 1, &fenix_rt.agree_op);

  if (fenix_rt.spare_ranks >= fenix_rt.procs.size()) {
    debug_print(
      "Fenix: <%d> spare ranks requested are unavailable\n",
      fenix_rt.spare_ranks
    );
  }

  fenix_rt.data_recovery = new data::DataComponent();

  // Now loop on creating communicators until success.
  do {
    fenix_rt.world = mpixx::Comm::shrink(args.in_comm);
    PMPI_Comm_set_errhandler(fenix_rt.world, fenix_rt.mpi_errhandler);

    rebuild_proc_groups();
  } while (FENIX_SUCCESS != try_build_active_worlds());

  if (!spare()) {
    fenix_rt.num_initial_ranks = fenix_rt.new_world.size();
    if (fenix_rt.options.verbose == 0) {
      verbose_print(
        "rank: %d, role: %d, number_initial_ranks: %d\n", fenix_rt.world.rank(),
        fenix_rt.role, fenix_rt.num_initial_ranks
      );
    }

  } else {
    fenix_rt.num_initial_ranks = fenix_rt.spare_ranks;

    if (fenix_rt.options.verbose == 0) {
      verbose_print(
        "rank: %d, role: %d, number_initial_ranks: %d\n", fenix_rt.world.rank(),
        fenix_rt.role, fenix_rt.num_initial_ranks
      );
    }
  }

  fenix_rt.fenix_init_flag = true;

  if (spare()) {
    spare_rank_loop();
    if (fenix_rt.role == FENIX_ROLE_SPARE_RANK) {
      // Finalized as a spare rank
      return FENIX_ROLE_SPARE_RANK;
    }
  }

  return fenix_rt.role;
}

void init(const args::FenixInitArgs args) {
  preinit(args);
  __fenix_postinit();
}

void rebuild_proc_groups() {
  auto& procs       = fenix_rt.procs;
  auto& user        = fenix_rt.user_procs;
  auto& dead        = fenix_rt.dead_procs;
  auto& spare       = fenix_rt.spare_procs;
  auto& pid_to_rank = fenix_rt.pid_to_rank;
  auto& rank_to_pid = fenix_rt.rank_to_pid;

  dead = procs - fenix_rt.world;
  spare -= dead; // Remove any newly dead spares

  // Index (in user procs) of any dead user procs. Already sorted.
  auto dead_user = (user | dead).translate_ranks(user);

  // The pids to build user_procs with. Start with current and update.
  auto user_pids = user.translate_ranks(procs);

  // Start by replacing what we can
  int n_replace = std::min((int)dead_user.size(), spare.size());
  for (int i = 0; i < n_replace; i++) {
    int idx  = dead_user[i];
    int pid  = user_pids[idx];
    int rank = pid_to_rank[pid];
    fenix_assert(rank != MPI_UNDEFINED);

    int spare_pid          = spare.translate_rank(i, procs);
    user_pids[idx]         = spare_pid;
    rank_to_pid[rank]      = spare_pid;
    pid_to_rank[spare_pid] = rank;
  }

  // Then shrink the rest (backwards since we are erasing from user_pids)
  for (int i = dead_user.size() - 1; i >= n_replace; i--) {
    int idx  = dead_user[i];
    int pid  = user_pids[idx];
    int rank = pid_to_rank[pid];
    fenix_assert(rank != MPI_UNDEFINED);

    user_pids.erase(user_pids.begin() + idx);
    rank_to_pid[rank] = MPI_UNDEFINED;
    // No new pid assigned to a rank, so no pid_to_rank change
  }

  user = procs.incl(user_pids);
  spare -= user;
  fenix_rt.spare_ranks = spare.size();
}

int try_build_active_worlds() {
  fenix_rt.new_world = fenix_rt.world.create(fenix_rt.user_procs);
  int flag           = (spare() || fenix_rt.new_world) ? 1 : 0;
  MPIX_Comm_agree(fenix_rt.world, &flag);
  if (flag != 1) return FENIX_ERROR_CANCELLED;

  if (!spare()) fenix_rt.user_world = fenix_rt.new_world.dup();
  flag = (spare() || fenix_rt.user_world) ? 1 : 0;
  MPIX_Comm_agree(fenix_rt.world, &flag);
  if (flag != 1) return FENIX_ERROR_CANCELLED;

  *fenix_rt.user_world_ptr = fenix_rt.user_world;
  return FENIX_SUCCESS;
}

void spare_rank_loop() {
  const bool yield_mode = get_option(SPARE_WAIT_MODE) == YIELD;
  const bool sleep_mode = get_option(SPARE_WAIT_MODE) == SLEEP;

  int provided_thread_level;
  MPI_T_init_thread(MPI_THREAD_SINGLE, &provided_thread_level);

  MPI_T_cvar_handle yield_cvar = MPI_T_CVAR_HANDLE_NULL;
  bool old_yield_setting       = false;
  if (yield_mode) {
    int idx, count;
    int ret = MPI_T_cvar_get_index("mpi_yield_when_idle", &idx);
    if (ret == MPI_SUCCESS) {
      MPI_T_cvar_handle_alloc(idx, NULL, &yield_cvar, &count);
      MPI_T_cvar_read(yield_cvar, &old_yield_setting);
      MPI_T_cvar_write(yield_cvar, &yield_mode);
    }
  }

  while (!fenix_rt.finalized && spare()) {
    int a, ret = MPI_SUCCESS, msg_found = true;
    MPI_Status mpi_status;
    {
      util::ScopedIgnoreAndReturn opts;
      int progress_count = 0;
      while (sleep_mode) {
        ret = PMPI_Iprobe(
          MPI_ANY_SOURCE, MPI_ANY_TAG, fenix_rt.world, &msg_found, &mpi_status
        );
        if (ret == MPI_SUCCESS) {
          // Explicit check so older Open MPI versions still work
          int is_revoked;
          MPIX_Comm_is_revoked(fenix_rt.world, &is_revoked);
          if (is_revoked) ret = MPI_ERR_REVOKED;
        }

        if (msg_found || ret != MPI_SUCCESS) break;
        if (++progress_count >= 5) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          progress_count = 0;
        }
      }
      if (ret == MPI_SUCCESS) {
        ret = PMPI_Recv(
          &a, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, fenix_rt.world,
          &mpi_status
        );
      }
    }
    if (ret == MPI_SUCCESS) {
      __fenix_finalize_spare();
    } else if (ret == MPI_ERR_REVOKED) {
      fenix_rt.repair_result = __fenix_repair_ranks();
    } else {
#ifdef MPICH_VERSION
      MPIX_Comm_failure_ack(fenix_rt.world);
#else
      MPIX_Comm_ack_failed(fenix_rt.world, fenix_rt.world.size(), &a);
#endif
    }
  }
  if (!fenix_rt.finalized) fenix_rt.role = FENIX_ROLE_RECOVERED_RANK;

  // Cleanup before exiting as a recovered rank
  if (yield_cvar != MPI_T_CVAR_HANDLE_NULL) {
    MPI_T_cvar_write(yield_cvar, &old_yield_setting);
    MPI_T_cvar_handle_free(&yield_cvar);
  }
  MPI_T_finalize();
}

int __fenix_repair_ranks() {
  using mpixx::Comm;
  using mpixx::Group;

  util::ScopedIgnoreAndReturn scoped_opts;
  util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  int recovery = scoped_opts.recovery.old;
  if (recovery == NOOP) return FENIX_SUCCESS;

  // Double check that every process is here, not in some local error handling
  // elsewhere. Assume that other locations will converge here.
  if (!spare()) {
    int location = FENIX_ERRHANDLER_LOC;
    do {
      location = FENIX_ERRHANDLER_LOC;
      MPIX_Comm_agree(fenix_rt.user_world, &location);
    } while (location != FENIX_ERRHANDLER_LOC);
  }

  // Now entering recovery. Start by saving a reference to old data for
  // updating metadata once recovery completes.
  auto old_world      = std::move(fenix_rt.world);
  auto old_user_procs = fenix_rt.user_procs.dup();

  do {
    fenix_rt.world = old_world.shrink();
    rebuild_proc_groups();
  } while (FENIX_SUCCESS != try_build_active_worlds());

  bool shrank = fenix_rt.user_procs.size() < fenix_rt.rank_to_pid.size();
  if (shrank && recovery == SPAWN && fenix_rt.world.rank() == 0) {
    debug_print("FENIX_RECOVERY_SPAWN is not currently supported. Shrinking.");
  }

  // Recovery complete, now we just update metadata about recovered state
  // Save data on failures recovered from in this recovery operation
  fenix_rt.fail_procs = old_user_procs - fenix_rt.user_procs;
  fenix_rt.fail_ranks = fenix_rt.fail_procs.translate_ranks(old_user_procs);

  fenix_rt.fail_world_size = fenix_rt.fail_ranks.size();
  fenix_rt.fail_world      = fenix_rt.fail_ranks.data();

  fenix_rt.recovered_procs = fenix_rt.user_procs - old_user_procs;
  fenix_rt.survivor_procs  = fenix_rt.user_procs - fenix_rt.recovered_procs;

  fenix_rt.num_recovered_ranks = fenix_rt.recovered_procs.size();
  fenix_rt.num_survivor_ranks  = fenix_rt.survivor_procs.size();

  if (spare()) {
    fenix_rt.role = FENIX_ROLE_SPARE_RANK;
  } else if (fenix_rt.survivor_procs.rank() != MPI_UNDEFINED) {
    fenix_rt.role = FENIX_ROLE_SURVIVOR_RANK;
  } else if (fenix_rt.recovered_procs.rank() != MPI_UNDEFINED) {
    fenix_rt.role = FENIX_ROLE_RECOVERED_RANK;
  } else {
    fatal_print("Internal recovery error - this is a Fenix bug!");
  }

  return shrank ? FENIX_WARNING_SPARE_RANKS_DEPLETED : FENIX_SUCCESS;
}

int detect_failures(bool do_recovery) {
#ifdef FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS
  // Special handling b/c we're doing things outside the API macro
  if (!initialized()) return FENIX_ERROR_UNINITIALIZED;
#endif
  // Create the IgnoreAndReturn scoped option if recovery is disabled.
  // Doing this outside of the API macro so the function behaves as if the
  // user had these settings on.
  std::optional<util::ScopedIgnoreAndReturn> scoped_opts;
  if (!do_recovery) scoped_opts.emplace();
  const bool must_return = get_option(RESUME_MODE) == RETURN;

  FENIX_CPP_API_BEGIN
  util::ScopedActiveMlog scoped_mlog(FENIX_MLOG_NONE);
  const bool inline_recovery = scoped_mlog.old_inline_recovery;

  while (true) {
    try {
      int flag;
      int ret =
        MPI_Test(&fenix_rt.check_failures_req, &flag, MPI_STATUS_IGNORE);
      fenix_assert(!flag, "DETECT_FAILURES_TAG should never be used");
      if (ret == MPI_SUCCESS) return FENIX_SUCCESS;
      else if (!inline_recovery) return FENIX_ERROR_PROCESS_FAILURE;
    } catch (const CommException& e) {
      if (!inline_recovery) {
        if (must_return) return FENIX_ERROR_PROCESS_FAILURE;
        else throw;
      }
    }
  }
  FENIX_CPP_API_END
}

void __fenix_finalize_spare() {
  fenix_rt.fenix_init_flag = false;
  int unused;

#ifdef MPICH_VERSION
  MPIX_Comm_agree(fenix_rt.world, &unused);
#else
  MPI_Request agree_req, recv_req = MPI_REQUEST_NULL;

  MPIX_Comm_iagree(fenix_rt.world, &unused, &agree_req);
  while (true) {
    int completed = 0;
    MPI_Test(&agree_req, &completed, MPI_STATUS_IGNORE);
    if (completed) break;

    int ret = MPI_Test(&recv_req, &completed, MPI_STATUS_IGNORE);
    if (completed) {
      //We may get duplicate messages informing us to exit
      MPI_Irecv(
        &unused, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, fenix_rt.world,
        &recv_req
      );
    }
    if (ret != MPI_SUCCESS) {
      MPIX_Comm_ack_failed(fenix_rt.world, fenix_rt.world.size(), &unused);
    }
  }

  if (recv_req != MPI_REQUEST_NULL) MPI_Cancel(&recv_req);
#endif

  MPI_Op_free(&fenix_rt.agree_op);
  MPI_Comm_set_errhandler(fenix_rt.world, MPI_ERRORS_ARE_FATAL);
  fenix_rt.world.free();

  /* Free data recovery interface */
  delete fenix_rt.data_recovery;

  /* Free up any C++ data structures, reset default variables */
  // Release user_world so it remains valid for the application
  (void)fenix_rt.user_world.release();
  SpareFinalizeMode mode = fenix_rt.settings.spare_finalize;
  fenix_rt               = {};
  fenix_rt.finalized     = true;
  fenix_rt.role          = FENIX_ROLE_SPARE_RANK;

  if (mode == EXIT) {
    MPI_Finalize();
    exit(0);
  }
}

void __fenix_test_MPI(MPI_Comm* pcomm, int* pret, ...) {
  if (!fenix_rt.fenix_init_flag) return;

  util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  fenix_rt.mpi_fail_code = *pret;

  constexpr bool throw_new = true;

  switch (fenix_rt.mpi_fail_code) {
  case MPI_ERR_PROC_FAILED_PENDING:
  case MPI_ERR_PROC_FAILED:
  case MPI_ERR_REVOKED:
    // This is an error type handled by Fenix

    // Skip to resume if recovery mode is IGNORE
    if (fenix_rt.settings.recovery == IGNORE) {
      util::resume_application(throw_new);
      return;
    }

    MPIX_Comm_revoke(fenix_rt.world);
    MPIX_Comm_revoke(fenix_rt.new_world);
    if (fenix_rt.user_world) fenix_rt.user_world.revoke();

    // Revoke all data recovery cohort communicators
    if (fenix_rt.data_recovery) fenix_rt.data_recovery->revoke();

    callback_invoke_all(fenix::PRE_RECOVERY);
    fenix_rt.repair_result = __fenix_repair_ranks();

    fenix_rt.role = FENIX_ROLE_SURVIVOR_RANK;
    __fenix_postinit();

    util::resume_application(throw_new);
    break;

  default:
    // This is an error type not handled by Fenix
    std::string errstr = mpixx::mpi_error_string(fenix_rt.mpi_fail_code);
    switch (fenix_rt.settings.unhandled) {
    case ABORT:
      fprintf(stderr, "UNHANDLED ERR: %s\n", errstr.c_str());
      MPI_Abort(fenix_rt.world, 1);
      break;
    case PRINT:
      fprintf(stderr, "UNHANDLED ERR: %s\n", errstr.c_str());
      break;
    case SILENT:
      break;
    default:
      fatal_print("Unknown unhandled mode %d\n", fenix_rt.settings.unhandled);
      break;
    }
  }
}

int comm_revoke(MPI_Comm comm) { return MPIX_Comm_revoke(comm); }

} // namespace fenix

using namespace fenix;

int __fenix_preinit(
  int* role, MPI_Comm comm, MPI_Comm* new_comm, int* argc, char*** argv,
  int spare_ranks, int* error, jmp_buf* jump_env
) {
  args::FenixInitArgs args;
  args.role     = role;
  args.in_comm  = comm;
  args.out_comm = new_comm;
  args.argc     = argc;
  args.argv     = argv;
  args.spares   = spare_ranks;
  args.err      = error;
  return preinit(args, jump_env);
}

void __fenix_postinit() {
  *fenix_rt.ret_role  = fenix_rt.role;
  *fenix_rt.ret_error = fenix_rt.repair_result;

  if (fenix_rt.finalized) return;

  util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  if (fenix_rt.new_world) {
    //Set up dummy irecv to use for checking for failures.
    MPI_Irecv(
      &fenix_rt.dummy_recv_buffer, 1, MPI_INT, MPI_ANY_SOURCE,
      tags::DETECT_FAILURES_TAG, fenix_rt.new_world,
      &fenix_rt.check_failures_req
    );
  }

  if (fenix_rt.role != FENIX_ROLE_INITIAL_RANK) {
    callback_invoke_all();
    if (fenix_rt.settings.mlog_recovery == INLINE_AUTOSYNC) {
      for (int mlog_id : fenix_rt.mlog_order) {
        mlog::sync(mlog_id, FENIX_MLOG_CONTINUE);
      }
    }
  }

  if (fenix_rt.options.verbose == 9) {
    verbose_print(
      "After barrier. current_rank: %d, role: %d\n", fenix_rt.new_world.rank(),
      fenix_rt.role
    );
  }
}

int Fenix_Finalize() {
  FENIX_C_API_BEGIN
  util::ScopedActiveMlog scoped_mlog(FENIX_MLOG_NONE);
  bool inline_recovery = scoped_mlog.old_inline_recovery;

  int location = FENIX_FINALIZE_LOC;
  do {
    MPIX_Comm_agree(fenix_rt.user_world, &location);
    if (location != FENIX_FINALIZE_LOC) {
      //Some ranks are in error recovery, so trigger error handling.
      MPIX_Comm_revoke(fenix_rt.user_world);
      if (inline_recovery) {
        // If we are doing inline recovery for this function, set errors
        // to return so we can just keep retrying this barrier.
        util::ScopedOption(FENIX_RESUME_MODE, RETURN);
        MPI_Barrier(fenix_rt.user_world);
      } else {
        MPI_Barrier(fenix_rt.user_world);
      }
    }
  } while (location != FENIX_FINALIZE_LOC);

  int first_spare_rank = fenix_rt.user_world.size();
  int last_spare_rank  = fenix_rt.world.size() - 1;

  //If we've reached here, we will finalize regardless of further errors.
  fenix_rt.settings.recovery = IGNORE;
  fenix_rt.settings.resume   = RETURN;
  while (!fenix_rt.finalized) {
    int user_rank = fenix_rt.user_world.rank();

    if (user_rank == 0) {
      for (int i = first_spare_rank; i <= last_spare_rank; i++) {
        //We don't care if a spare failed, ignore return value
        int unused;
        MPI_Request req;
        MPI_Isend(&unused, 1, MPI_INT, i, 1, fenix_rt.world, &req);
        MPI_Request_free(&req);
      }
    }

    //We need to confirm that rank 0 didn't fail, since it could have
    //failed before notifying some spares to leave.
    int need_retry = user_rank == 0 ? 0 : 1;
    MPIX_Comm_agree(fenix_rt.user_world, &need_retry);
    if (need_retry == 1) {
      //Rank 0 didn't contribute, so we need to retry.
      fenix_rt.user_world = fenix_rt.user_world.shrink();
      continue;
    } else {
      //If rank 0 did contribute, we know sends made it, and regardless
      //of any other failures we finalize.
      fenix_rt.finalized = true;
    }
  }

  //Now we do one last agree w/ the spares to let them know they can actually
  //finalize
  int unused;
  MPIX_Comm_agree(fenix_rt.world, &unused);

  MPI_Op_free(&fenix_rt.agree_op);
  MPI_Comm_set_errhandler(fenix_rt.world, MPI_ERRORS_ARE_FATAL);
  fenix_rt.world.free();
  fenix_rt.new_world.free();

  /* Free data recovery interface */
  delete fenix_rt.data_recovery;

  /* Free up any C++ data structures, reset default variables */
  // Release user_world so it remains valid for the application
  (void)fenix_rt.user_world.release();
  auto role          = fenix_rt.role;
  fenix_rt           = {};
  fenix_rt.finalized = true;
  fenix_rt.role      = role;
  return FENIX_SUCCESS;
  FENIX_C_API_END
}
