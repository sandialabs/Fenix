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
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY and SANDIA CORPORATION
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RUTGERS
// UNIVERISY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <signal.h>
#include <chrono>
#include <thread>

#include <mpi.h>

#include <fenix.hpp>
#include <fenix_util.hpp>
#include <fenix/logging/message_logging.h>

constexpr int group = 0;
constexpr int state_member = 0;
constexpr int mlogs_member = 1;

constexpr int mlogs = 2;

// Total work iterations
constexpr int app_iterations = 100;
// Iterations between allreduce convergence checks
constexpr int convergence_check_iterations = 5;
// Iterations between checkpoints
constexpr int checkpoint_iterations = 10;

// Synthetic per-iteration work, in milliseconds
constexpr int iteration_work_ms = 10;

// Very simplified application state
struct State {
  int rank = -1, iteration = -1;
};

// Inject a failure on some ranks on some iterations
void check_inject_failure(State& state, int app_ranks) {
  // Use global rank to avoid infinitely repeating the same failures
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  bool kill = false;
  kill |= rank == app_ranks / 2 && state.iteration == 18;
  kill |= rank == app_ranks - 1 && state.iteration == 21;
  kill |= rank == 0 && state.iteration == 78;
  if (kill) {
    printf("Rank %d failing at iteration %d\n", rank, state.iteration);
    raise(SIGKILL);
  }
}

int main(int argc, char** argv) {
  using namespace fenix::data;
  MPI_Init(&argc, &argv);

  // Initialize fenix in exception-based recovery mode
  MPI_Comm res_world;
  fenix::init({.out_comm = &res_world, .spares = 3});
  assert(fenix::error() == FENIX_SUCCESS);

  // Hold on to checkpoint_iterations+1 regions at once, to be sure we can
  // replay any failed neighbor's messages
  fenix::mlog::create(mlogs, res_world, checkpoint_iterations + 1);

  // Grab basic MPI info
  int n_ranks, rank;
  MPI_Comm_size(res_world, &n_ranks);
  MPI_Comm_rank(res_world, &rank);
  const int left_rank = (rank + n_ranks - 1) % n_ranks;
  const int right_rank = (rank + 1) % n_ranks;

  State state;

  // Set up the local state
  if (fenix::role() == fenix::INITIAL_RANK) {
    // Initial ranks initialize state and make the first checkpoint
    state.rank = rank;
    state.iteration = 0;

    fenix::data::group_create(group);

    fenix::data::member_create(group, state_member, &state, 2, MPI_INT);
    fenix::data::member_stage(group, state_member);
    fenix::mlog::stage(mlogs, group, mlogs_member);
    fenix::data::member_store(group, SUBSET_PRESTAGED);
    fenix::data::commit_barrier(group);
  } else {
    // Recovered ranks just recover from the checkpoint instead
    while (true) {
      try {
        fenix::data::group_create(group);
        fenix::data::member_restore(group, state_member, &state, 2);
        fenix::data::member_restore(group, mlogs_member, nullptr, 0);
        fenix::mlog::lrestore(mlogs, group, mlogs_member);
        fenix::mlog::sync(mlogs, state.iteration);
      } catch (fenix::CommException& error) {
        continue;
      }
      break;
    }
    assert(state.rank == rank);
    printf("Rank %d recovered to iteration %d\n", state.rank, state.iteration);
  }

  // Now that our local state is good, add our recovery callback to help
  // others recover their state on failure(s).
  fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
    assert(fenix::error() == FENIX_SUCCESS);

    // Disable logging inside this callback
    fenix::util::ScopedActiveMlog setting(nullptr);

    fenix::data::group_create(group);
    fenix::data::member_restore(group, state_member, NULL, 0);
    fenix::data::member_restore(group, mlogs_member, NULL, 0);

    // We want to continue from exactly where we are
    fenix::mlog::sync(mlogs, FENIX_MLOG_CONTINUE);

    printf(
      "Rank %d continuing inline at iteration %d\n", state.rank, state.iteration
    );
  });

  // Now enter the application work loop.
  for (int i = state.iteration; i < app_iterations; i++) {
    check_inject_failure(state, n_ranks);

    {
      // Enable logging and inline recovery within this scope
      fenix::util::ScopedActiveMlog mlog_setting(mlogs);
      fenix::util::ScopedInlineRecovery setting(true);
      fenix::mlog::begin_region(mlogs, i);

      // Exchange state information, just like exchanging ghost points
      State left_state, right_state;
      MPI_Sendrecv(
        &state,      2, MPI_INT, right_rank, 0,
        &left_state, 2, MPI_INT, left_rank,  0, res_world, MPI_STATUS_IGNORE
      );
      MPI_Sendrecv(
        &state,       2, MPI_INT, left_rank,  0,
        &right_state, 2, MPI_INT, right_rank, 0, res_world, MPI_STATUS_IGNORE
      );
      // We'll always get the expected messages, regardless of faults
      assert(left_state.rank == left_rank && left_state.iteration == i);
      assert(right_state.rank == right_rank && right_state.iteration == i);

      // Do the application work. In this case, just increment our state's iter
      assert(state.iteration == i);
      state.iteration++;
      std::this_thread::sleep_for(std::chrono::milliseconds(iteration_work_ms));

      // Allreduce, as if checking if the stencil has converged on a solution
      if (state.iteration % convergence_check_iterations == 0) {
        double my_part = i, result = -1;
        MPI_Allreduce(&my_part, &result, 1, MPI_DOUBLE, MPI_SUM, res_world);
        assert(result == i * n_ranks);
      }
    }

    if (state.iteration % checkpoint_iterations == 0) {
      // We might have managed to finish our checkpoint remotely before
      // failing locally, so check the timestamp after each attempt
      int old_timestamp;
      Fenix_Data_group_get_snapshot_at_position(group, 0, &old_timestamp);
      int cur_timestamp = old_timestamp;
      while (old_timestamp == cur_timestamp) {
        try {
          fenix::data::member_store(group, state_member);
          fenix::mlog::stage(mlogs, group, mlogs_member);
          fenix::data::member_storev(group, mlogs_member, SUBSET_PRESTAGED);
          fenix::data::commit(group);
        } catch (fenix::CommException& error) {
        }
        Fenix_Data_group_get_snapshot_at_position(group, 0, &cur_timestamp);
      }
    }
  }

  while (fenix::initialized()) {
    try {
      Fenix_Finalize();
    } catch (fenix::CommException& error) {
      // Retry
      continue;
    }
    break;
  }

  MPI_Finalize();
}
