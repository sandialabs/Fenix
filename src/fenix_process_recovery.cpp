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

#include <mpi.h>
#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif

#include "fenix_ext.hpp"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"

namespace fenix {

static int __fenix_create_new_world();
static int __fenix_repair_ranks();
static void __fenix_test_MPI(MPI_Comm*, int*, ...);
static int* __fenix_get_fail_ranks(int *, int, int);
static int __fenix_spare_rank();
static void __fenix_finalize_spare();

static int preinit(const args::FenixInitArgs& args, jmp_buf* jump_env = nullptr){
    fenix_rt.finalized = false;

    fenix_rt.world = (MPI_Comm *)malloc(sizeof(MPI_Comm));
    MPI_Comm_dup(args.in_comm, fenix_rt.world);
    
    MPI_Comm_create_errhandler(__fenix_test_MPI, &fenix_rt.mpi_errhandler);
    PMPI_Comm_set_errhandler(*fenix_rt.world, fenix_rt.mpi_errhandler);

    fenix_rt.user_world = args.out_comm;
    fenix_rt.spare_ranks = args.spares;
    fenix_rt.recover_environment = jump_env;

    fenix_rt.ret_role = args.role ? args.role : &fenix_rt.role;
    fenix_rt.ret_error = args.err ? args.err : &fenix_rt.repair_result;

    *fenix_rt.ret_role = fenix_rt.role;
    *fenix_rt.ret_error = FENIX_SUCCESS;

    fenix_rt.settings = fenix_default_settings;
    if (fenix_rt.settings.resume == FENIX_RESUME_MODE_MAXCODE) {
        fenix_rt.settings.resume = jump_env ? JUMP : THROW;
    }
    fenix_assert(
        fenix_rt.settings.resume != JUMP || jump_env != nullptr,
        "Must use Fenix_Init to use FENIX_RESUME_JUMP"
    );

    MPI_Op_create((MPI_User_function *) __fenix_ranks_agree, 1, &fenix_rt.agree_op);

    if (fenix_rt.spare_ranks >= __fenix_get_world_size(*fenix_rt.world)) {
        debug_print("Fenix: <%d> spare ranks requested are unavailable\n",
                    fenix_rt.spare_ranks);
    }

    fenix_rt.data_recovery = data::__fenix_data_recovery_init();

    /*****************************************************/
    /* Note: fenix_rt.new_world is only valid for the   */
    /*       active MPI ranks. Spare ranks do not        */
    /*       allocate any communicator content with this.*/
    /*       Any MPI calls in spare ranks with new_world */
    /*       trigger an abort.                           */
    /*****************************************************/

    //Try to create new_world until success
    while (__fenix_create_new_world());

    if ( __fenix_spare_rank() != 1) {
        fenix_rt.num_initial_ranks = __fenix_get_world_size(fenix_rt.new_world);
        if (fenix_rt.options.verbose == 0) {
            verbose_print("rank: %d, role: %d, number_initial_ranks: %d\n",
                          __fenix_get_current_rank(*fenix_rt.world), fenix_rt.role,
                          fenix_rt.num_initial_ranks);
        }

    } else {
        fenix_rt.num_initial_ranks = fenix_rt.spare_ranks;

        if (fenix_rt.options.verbose == 0) {
            verbose_print("rank: %d, role: %d, number_initial_ranks: %d\n",
                          __fenix_get_current_rank(*fenix_rt.world), fenix_rt.role,
                          fenix_rt.num_initial_ranks);
        }
    }

    fenix_rt.fenix_init_flag = true;

    while ( __fenix_spare_rank() == 1) {
        int a, ret;
        MPI_Status mpi_status;
        {
            util::ScopedIgnoreAndReturn opts;
            ret = PMPI_Recv(&a, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                            *fenix_rt.world, &mpi_status);
        }
        if (ret == MPI_SUCCESS) {
            if (fenix_rt.options.verbose == 0) {
                verbose_print("Finalize the program; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*fenix_rt.world), fenix_rt.role);
            }
            __fenix_finalize_spare();
        } else if(ret == MPI_ERR_REVOKED){
            fenix_rt.repair_result = __fenix_repair_ranks();
            if (fenix_rt.options.verbose == 0) {
                verbose_print("spare rank exiting from MPI_Recv - repair ranks; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*fenix_rt.world), fenix_rt.role);
            }
        } else {
#ifdef MPICH_VERSION
            MPIX_Comm_failure_ack(*fenix_rt.world);
#else
            MPIX_Comm_ack_failed(*fenix_rt.world, __fenix_get_world_size(*fenix_rt.world), &a);
#endif
        }
        fenix_rt.role = FENIX_ROLE_RECOVERED_RANK;
    }

    
    if(fenix_rt.role != FENIX_ROLE_RECOVERED_RANK) MPI_Comm_dup(fenix_rt.new_world, fenix_rt.user_world);
    fenix_rt.user_world_exists = true;

    return fenix_rt.role;
}

void init(const args::FenixInitArgs args){

    preinit(args);
    __fenix_postinit();
}

Fenix_Resume_mode get_resume_mode(const std::string_view& name){
    if (name == "JUMP") {
        return fenix::JUMP;
    } else if (name == "RETURN") {
        return fenix::RETURN;
    } else if (name == "THROW") {
        return fenix::THROW;
    }
    fatal_print("Unsupported FENIX_RESUME_MODE %s", name.data());
}

Fenix_Unhandled_mode get_unhandled_mode(const std::string_view& name){
    if (name == "SILENT") {
        return fenix::SILENT;
    } else if (name == "PRINT") {
        return fenix::PRINT;
    } else if (name == "ABORT") {
        return fenix::ABORT;
    }
    fatal_print("Unsupported FENIX_UNHANDLED_MODE %s", name.data());
}

int __fenix_spare_rank_within(MPI_Comm refcomm)
{
    int result = -1;
    int current_rank = __fenix_get_current_rank(refcomm);
    int new_world_size = __fenix_get_world_size(refcomm) - fenix_rt.spare_ranks;
    if (current_rank >= new_world_size) {
        if (fenix_rt.options.verbose == 6) {
            verbose_print("current_rank: %d, new_world_size: %d\n", current_rank, new_world_size);
        }
        result = 1;
    }
    return result;
}

int __fenix_create_new_world_from(MPI_Comm from_comm)
{
    int ret;

    if ( __fenix_spare_rank_within(from_comm) == 1) {
        int current_rank = __fenix_get_current_rank(from_comm);

        /*************************************************************************/
        /** MPI_UNDEFINED makes the new communicator "undefined" at spare ranks **/
        /** This means no data is allocated at spare ranks                      **/
        /** Use of the new communicator triggers the program to abort .         **/
        /*************************************************************************/

        if (fenix_rt.options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(from_comm),
                          fenix_rt.role);
        }

        ret = PMPI_Comm_split(from_comm, MPI_UNDEFINED, current_rank,
                              &fenix_rt.new_world);
        fenix_rt.new_world_exists = 0; //Should already be this

    } else {

        int current_rank = __fenix_get_current_rank(from_comm);

        if (fenix_rt.options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(from_comm),
                          fenix_rt.role);
        }

        ret = PMPI_Comm_split(from_comm, 0, current_rank, &fenix_rt.new_world);
        fenix_rt.new_world_exists = 1;
        if (ret != MPI_SUCCESS){
            fenix_rt.new_world_exists = 0;
        }

    }
    return ret;
}

int __fenix_create_new_world(){
    return __fenix_create_new_world_from(*fenix_rt.world);
}

int __fenix_repair_ranks()
{
    util::ScopedIgnoreAndReturn scoped_opts;
    util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
    int recovery = scoped_opts.recovery.old;
    if(recovery == NOOP) return FENIX_SUCCESS;

    /*********************************************************/
    /* Do not forget comm_free for broken communicators      */
    /*********************************************************/
    int ret;
    int survived_flag;
    int *survivor_world;
    int *fail_world;
    int current_rank;
    int survivor_world_size;
    int world_size;
    int fail_world_size;
    int rt_code = FENIX_SUCCESS;
    int repair_success = 0;
    int num_try = 0;
    int flag_g_world_freed = 0;
    MPI_Comm world_without_failures, fixed_world;

    
    /* current_rank means the global MPI rank before failure */
    current_rank = __fenix_get_current_rank(*fenix_rt.world);
    world_size = __fenix_get_world_size(*fenix_rt.world);

    //Double check that every process is here, not in some local error handling elsewhere.
    //Assume that other locations will converge here.
    if(__fenix_spare_rank() != 1){
    	int location = FENIX_ERRHANDLER_LOC;
    	do {
	    location = FENIX_ERRHANDLER_LOC;
	    MPIX_Comm_agree(*fenix_rt.user_world, &location);
        } while(location != FENIX_ERRHANDLER_LOC);
    }
    
    while (!repair_success) {
        repair_success = 1;

        ret = MPIX_Comm_shrink(*fenix_rt.world, &world_without_failures);
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            goto END_LOOP;
        }

        /*********************************************************/
        /* Free up the storage for active process communicator   */
        /*********************************************************/
        if ( __fenix_spare_rank() != 1) {
            if(fenix_rt.new_world_exists) PMPI_Comm_free(&fenix_rt.new_world);
            if(fenix_rt.user_world_exists) PMPI_Comm_free(fenix_rt.user_world);
            fenix_rt.user_world_exists = 0;
            fenix_rt.new_world_exists = 0;
        }
        /*********************************************************/
        /* Need closer look above                                */
        /*********************************************************/

        survivor_world_size = __fenix_get_world_size(world_without_failures);
        fenix_rt.fail_world_size = world_size - survivor_world_size;

        if (fenix_rt.options.verbose == 2) {
            verbose_print(
                "current_rank: %d, role: %d, world_size: %d, fail_world_size: %d, survivor_world_size: %d\n",
                current_rank, fenix_rt.role, world_size,
                fenix_rt.fail_world_size, survivor_world_size);
        }

        if (fenix_rt.spare_ranks < fenix_rt.fail_world_size) {
            /* Not enough spare ranks */

            if (fenix_rt.options.verbose == 2) {
                verbose_print(
                    "current_rank: %d, role: %d, spare_ranks: %d, fail_world_size: %d\n",
                    current_rank, fenix_rt.role, fenix_rt.spare_ranks,
                    fenix_rt.fail_world_size);
            }

            if (recovery == SPAWN) {
                debug_print("FENIX_RECOVERY_SPAWN is not supported\n");
            } else {

                rt_code = FENIX_WARNING_SPARE_RANKS_DEPLETED;

                /***************************************/
                /* Fill the ranks in increasing order  */
                /***************************************/

                int active_ranks;

                survivor_world = (int *) s_malloc(survivor_world_size * sizeof(int));

                ret = PMPI_Allgather(&current_rank, 1, MPI_INT, survivor_world, 1, MPI_INT,
                                     world_without_failures);

                if (fenix_rt.options.verbose == 2) {
                    int index;
                    for (index = 0; index < survivor_world_size; index++) {
                        verbose_print("current_rank: %d, role: %d, survivor_world[%d]: %d\n",
                                      current_rank, fenix_rt.role, index,
                                      survivor_world[index]);
                    }
                }

                 //if (ret != MPI_SUCCESS) { debug_print("MPI_Allgather. repair_ranks\n"); }
                if (ret != MPI_SUCCESS) {
                    repair_success = 0;
                    if (ret == MPI_ERR_PROC_FAILED) {
                        MPIX_Comm_revoke(world_without_failures);
                    }
                    MPI_Comm_free(&world_without_failures);
                    free(survivor_world);
                    goto END_LOOP;
                }

                survived_flag = 0;
                if (fenix_rt.role == FENIX_ROLE_SURVIVOR_RANK) {
                    survived_flag = 1;
                }

                ret = PMPI_Allreduce(&survived_flag, &fenix_rt.num_survivor_ranks, 1,
                                     MPI_INT, MPI_SUM, world_without_failures);

                //if (ret != MPI_SUCCESS) { debug_print("MPI_Allreduce. repair_ranks\n"); }
                if (ret != MPI_SUCCESS) {
                    repair_success = 0;
                    if (ret == MPI_ERR_PROC_FAILED) {
                        MPIX_Comm_revoke(world_without_failures);
                    }
                    MPI_Comm_free(&world_without_failures);
                    free(survivor_world);
                    goto END_LOOP;
                }

                fenix_rt.num_initial_ranks = 0;

                /* recovered ranks must be the number of spare ranks */
                fenix_rt.num_recovered_ranks = fenix_rt.fail_world_size;

                if (fenix_rt.options.verbose == 2) {
                    verbose_print("current_rank: %d, role: %d, recovered_ranks: %d\n",
                                  current_rank, fenix_rt.role,
                                  fenix_rt.num_recovered_ranks);
                }

                if(fenix_rt.role != FENIX_ROLE_INITIAL_RANK){
                    free(fenix_rt.fail_world);
                }
                fenix_rt.fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size,
                                                    fenix_rt.fail_world_size);

                if (fenix_rt.options.verbose == 2) {
                    int index;
                    for (index = 0; index < fenix_rt.fail_world_size; index++) {
                        verbose_print("fail_world[%d]: %d\n", index, fenix_rt.fail_world[index]);
                    }
                }

                free(survivor_world);

                active_ranks = world_size - fenix_rt.spare_ranks;

                if (fenix_rt.options.verbose == 2) {
                    verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                                  current_rank, fenix_rt.role,
                                  active_ranks);
                }

                /* Assign new rank for reordering */
                if (current_rank >= active_ranks) { // reorder ranks
                    int rank_offset = ((world_size - 1) - current_rank);

                    for(int fail_i = 0; fail_i < fenix_rt.fail_world_size; fail_i++){
                      if(fenix_rt.fail_world[fail_i] > current_rank) rank_offset--;
                    }

                    if (rank_offset < fenix_rt.fail_world_size) {
                        if (fenix_rt.options.verbose == 11) {
                            verbose_print("reorder ranks; current_rank: %d -> new_rank: %d\n",
                                          current_rank, fenix_rt.fail_world[rank_offset]);
                        }
                        current_rank = fenix_rt.fail_world[rank_offset];
                    }
                }

                /************************************/
                /* Update the number of spare ranks */
                /************************************/
                fenix_rt.spare_ranks = 0;
            }
        } else {

            int active_ranks;

            survivor_world = (int *) s_malloc(survivor_world_size * sizeof(int));

            ret = PMPI_Allgather(&current_rank, 1, MPI_INT, survivor_world, 1, MPI_INT,
                                 world_without_failures);
            if (ret != MPI_SUCCESS) {
                repair_success = 0;
                if (ret == MPI_ERR_PROC_FAILED) {
                    MPIX_Comm_revoke(world_without_failures);
                }
                MPI_Comm_free(&world_without_failures);
                free(survivor_world);
                goto END_LOOP;
            }

            survived_flag = 0;
            if (fenix_rt.role == FENIX_ROLE_SURVIVOR_RANK) {
                survived_flag = 1;
            }

            ret = PMPI_Allreduce(&survived_flag, &fenix_rt.num_survivor_ranks, 1,
                                 MPI_INT, MPI_SUM, world_without_failures);
            if (ret != MPI_SUCCESS) {
                repair_success = 0;
                if (ret != MPI_ERR_PROC_FAILED) {
                    MPIX_Comm_revoke(world_without_failures);
                }
                MPI_Comm_free(&world_without_failures);
                free(survivor_world);
                goto END_LOOP;
            }


            fenix_rt.num_initial_ranks = 0;
            fenix_rt.num_recovered_ranks = fenix_rt.fail_world_size;
            
            if(fenix_rt.role != FENIX_ROLE_INITIAL_RANK){
                free(fenix_rt.fail_world);
            }

            fenix_rt.fail_world = (int *) s_malloc(fenix_rt.fail_world_size * sizeof(int));
            fenix_rt.fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size, fenix_rt.fail_world_size);
            free(survivor_world);

            if (fenix_rt.options.verbose == 2) {
                int index;
                for (index = 0; index < fenix_rt.fail_world_size; index++) {
                    verbose_print("fail_world[%d]: %d\n", index, fenix_rt.fail_world[index]);
                }
            }

            active_ranks = world_size - fenix_rt.spare_ranks;

            if (fenix_rt.options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                              current_rank, fenix_rt.role, active_ranks);
            }

            if (current_rank >= active_ranks) { // reorder ranks
                int rank_offset = ((world_size - 1) - current_rank);

                for(int fail_i = 0; fail_i < fenix_rt.fail_world_size; fail_i++){
                  if(fenix_rt.fail_world[fail_i] > current_rank) rank_offset--;
                }

                if (rank_offset < fenix_rt.fail_world_size) {
                    if (fenix_rt.options.verbose == 2) {
                        verbose_print("reorder ranks; current_rank: %d -> new_rank: %d (offset %d)\n",
                                      current_rank, fenix_rt.fail_world[rank_offset], rank_offset);
                    }
                    current_rank = fenix_rt.fail_world[rank_offset];
                }
            }

            /************************************/
            /* Update the number of spare ranks */
            /************************************/
            fenix_rt.spare_ranks = fenix_rt.spare_ranks - fenix_rt.fail_world_size;
            if (fenix_rt.options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, spare_ranks: %d\n",
                              current_rank, fenix_rt.role,
                              fenix_rt.spare_ranks);
            }
        }

        /*********************************************************/
        /* Done with the global communicator                     */
        /*********************************************************/

        ret = PMPI_Comm_split(world_without_failures, 0, current_rank, &fixed_world);
        
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            if (ret != MPI_ERR_PROC_FAILED) {
                MPIX_Comm_revoke(world_without_failures);
            }
            MPI_Comm_free(&world_without_failures);
            goto END_LOOP;
        }

        MPI_Comm_free(&world_without_failures);

        ret = __fenix_create_new_world_from(fixed_world);
        if(ret != MPI_SUCCESS){
            repair_success = 0;
            MPIX_Comm_revoke(fixed_world);
            MPI_Comm_free(&fixed_world);
            goto END_LOOP;
        }

        if(__fenix_spare_rank_within(fixed_world) == -1){
            ret = MPI_Comm_dup(fenix_rt.new_world, fenix_rt.user_world);
            if (ret != MPI_SUCCESS){
                repair_success = 0;
                MPIX_Comm_revoke(fixed_world);
                MPI_Comm_free(&fixed_world);
                goto END_LOOP;
            }
            fenix_rt.user_world_exists = 1;
        }
        
        ret = PMPI_Barrier(fixed_world);
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            MPIX_Comm_revoke(fixed_world);
            MPI_Comm_free(&fixed_world);
            goto END_LOOP;
        }

    END_LOOP:
        num_try++;
    }

    *fenix_rt.world = fixed_world;
    return rt_code;
}

int* __fenix_get_fail_ranks(int *survivor_world, int survivor_world_size, int fail_world_size)
{
    qsort(survivor_world, survivor_world_size, sizeof(int), __fenix_comparator);
    int failed_pos = 0;
    
    int *fail_ranks = (int *)calloc(fail_world_size, sizeof(int));

    int i;
    for (i = 0; i < survivor_world_size + fail_world_size; i++) {
        if (__fenix_binary_search(survivor_world, survivor_world_size, i) != 1) {
            if (fenix_rt.options.verbose == 14) {
                verbose_print("fail_rank: %d, fail_ranks[%d]: %d\n", i, failed_pos,
                              fail_ranks[failed_pos++]);
            }
            fail_ranks[failed_pos++] = i;
        }
    }
    return fail_ranks;
}

int __fenix_spare_rank(){
    return __fenix_spare_rank_within(*fenix_rt.world);
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
            int ret = MPI_Test(
                &fenix_rt.check_failures_req, &flag, MPI_STATUS_IGNORE
            );
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

void __fenix_finalize_spare()
{
    fenix_rt.fenix_init_flag = false;
    int unused;

#ifdef MPICH_VERSION
    MPIX_Comm_agree(*fenix_rt.world, &unused);
#else
    MPI_Request agree_req, recv_req = MPI_REQUEST_NULL;

    MPIX_Comm_iagree(*fenix_rt.world, &unused, &agree_req);
    while(true){
        int completed = 0;
        MPI_Test(&agree_req, &completed, MPI_STATUS_IGNORE);
        if(completed) break;

        int ret = MPI_Test(&recv_req, &completed, MPI_STATUS_IGNORE);
        if(completed){
            //We may get duplicate messages informing us to exit
            MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, *fenix_rt.world, &recv_req);
        }
        if(ret != MPI_SUCCESS){
            MPIX_Comm_ack_failed(*fenix_rt.world, __fenix_get_world_size(*fenix_rt.world), &unused);
        }
    }

    if(recv_req != MPI_REQUEST_NULL) MPI_Cancel(&recv_req);
#endif
 
    MPI_Op_free(&fenix_rt.agree_op);
    MPI_Comm_set_errhandler(*fenix_rt.world, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_free(fenix_rt.world);

    /* Free data recovery interface */
    __fenix_data_recovery_destroy( fenix_rt.data_recovery );

    /* Free up any C++ data structures, reset default variables */
    fenix_rt = {};

    /* Future version do not close MPI. Jump to where Fenix_Finalize is called. */
    MPI_Finalize();
    exit(0);
}

void __fenix_test_MPI(MPI_Comm *pcomm, int *pret, ...)
{
    util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
    fenix_rt.mpi_fail_code = *pret;

    if(!fenix_rt.fenix_init_flag) return;

    constexpr bool throw_new = true;

    switch (fenix_rt.mpi_fail_code) {
    case MPI_ERR_PROC_FAILED_PENDING:
    case MPI_ERR_PROC_FAILED:
    case MPI_ERR_REVOKED:
        // This is an error type handled by Fenix

        // Skip to resume if recovery mode is IGNORE
        if(fenix_rt.settings.recovery == IGNORE) {
            util::resume_application(throw_new);
            return;
        }

        MPIX_Comm_revoke(*fenix_rt.world);
        MPIX_Comm_revoke(fenix_rt.new_world);
        if(fenix_rt.user_world_exists) MPIX_Comm_revoke(*fenix_rt.user_world);

        callback_invoke_all(fenix::PRE_RECOVERY);
        fenix_rt.repair_result = __fenix_repair_ranks();

        fenix_rt.role = FENIX_ROLE_SURVIVOR_RANK;
        __fenix_postinit();

        util::resume_application(throw_new);
        break;

    default:
        // This is an error type not handled by Fenix
        std::string errstr = util::mpi_error_string(fenix_rt.mpi_fail_code);
        switch (fenix_rt.settings.unhandled) {
        case ABORT:
            fprintf(stderr, "UNHANDLED ERR: %s\n", errstr.c_str());
            MPI_Abort(*fenix_rt.world, 1);
            break;
        case PRINT:
            fprintf(stderr, "UNHANDLED ERR: %s\n", errstr.c_str());
            break;
        case SILENT:
            break;
        default:
            printf(
                "Fenix internal error: Unknown unhandled mode %d\n",
                fenix_rt.settings.unhandled
            );
            assert(false);
            break;
        }
    }
}

} // namespace fenix

using namespace fenix;

int __fenix_preinit(
    int *role, MPI_Comm comm, MPI_Comm *new_comm, int *argc, char ***argv,
    int spare_ranks, int *error, jmp_buf *jump_env
) {
    args::FenixInitArgs args;
    args.role = role;
    args.in_comm = comm;
    args.out_comm = new_comm;
    args.argc = argc;
    args.argv = argv;
    args.spares = spare_ranks;
    args.err = error;
    return preinit(args, jump_env);
}

void __fenix_postinit()
{
    util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
    *fenix_rt.ret_role = fenix_rt.role;
    *fenix_rt.ret_error = fenix_rt.repair_result;

    if(fenix_rt.new_world_exists){
        //Set up dummy irecv to use for checking for failures.
        MPI_Irecv(&fenix_rt.dummy_recv_buffer, 1, MPI_INT, MPI_ANY_SOURCE,
                  tags::DETECT_FAILURES_TAG, fenix_rt.new_world,
                  &fenix_rt.check_failures_req);
    }

    if(fenix_rt.role != FENIX_ROLE_INITIAL_RANK) {
        callback_invoke_all();
        if (fenix_rt.settings.mlog_recovery == INLINE_AUTOSYNC) {
            for (int mlog_id : fenix_rt.mlog_order) {
                mlog::sync(mlog_id, FENIX_MLOG_CONTINUE);
            }
        }
    }

    if (fenix_rt.options.verbose == 9) {
        verbose_print("After barrier. current_rank: %d, role: %d\n", __fenix_get_current_rank(fenix_rt.new_world),
                      fenix_rt.role);
    }
}

int Fenix_Finalize() {
    FENIX_C_API_BEGIN
    util::ScopedActiveMlog scoped_mlog(FENIX_MLOG_NONE);
    bool inline_recovery = scoped_mlog.old_inline_recovery;

    int location = FENIX_FINALIZE_LOC;
    do {
        MPIX_Comm_agree(*fenix_rt.user_world, &location);
        if(location != FENIX_FINALIZE_LOC){
            //Some ranks are in error recovery, so trigger error handling.
            MPIX_Comm_revoke(*fenix_rt.user_world);
            if (inline_recovery) {
                // If we are doing inline recovery for this function, set errors
                // to return so we can just keep retrying this barrier.
                util::ScopedOption(FENIX_RESUME_MODE, RETURN);
                MPI_Barrier(*fenix_rt.user_world);
            } else {
                MPI_Barrier(*fenix_rt.user_world);
            }
        }
    } while (location != FENIX_FINALIZE_LOC);

    int first_spare_rank = __fenix_get_world_size(*fenix_rt.user_world);
    int last_spare_rank = __fenix_get_world_size(*fenix_rt.world) - 1;

    //If we've reached here, we will finalize regardless of further errors.
    fenix_rt.settings.recovery = IGNORE;
    fenix_rt.settings.resume = RETURN;
    while(!fenix_rt.finalized){
        int user_rank = __fenix_get_current_rank(*fenix_rt.user_world);

        if (user_rank == 0) {
            for (int i = first_spare_rank; i <= last_spare_rank; i++) {
                //We don't care if a spare failed, ignore return value
                int unused;
                MPI_Request req;
                MPI_Isend(&unused, 1, MPI_INT, i, 1, *fenix_rt.world, &req);
                MPI_Request_free(&req);
            }
        }

        //We need to confirm that rank 0 didn't fail, since it could have
        //failed before notifying some spares to leave.
        int need_retry = user_rank == 0 ? 0 : 1;
        MPIX_Comm_agree(*fenix_rt.user_world, &need_retry);
        if(need_retry == 1){
            //Rank 0 didn't contribute, so we need to retry.
            MPIX_Comm_shrink(*fenix_rt.user_world, fenix_rt.user_world);
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
    MPIX_Comm_agree(*fenix_rt.world, &unused);


    MPI_Op_free( &fenix_rt.agree_op );
    MPI_Comm_set_errhandler( *fenix_rt.world, MPI_ERRORS_ARE_FATAL );
    MPI_Comm_free( fenix_rt.world );
    free(fenix_rt.world);
    if(fenix_rt.new_world_exists) MPI_Comm_free( &fenix_rt.new_world ); //It should, but just in case. Won't update because trying to free it again ought to generate an error anyway.

    if(fenix_rt.role != FENIX_ROLE_INITIAL_RANK){
        free(fenix_rt.fail_world);
    }

    /* Free data recovery interface */
    __fenix_data_recovery_destroy( fenix_rt.data_recovery );

    /* Free up any C++ data structures, reset default variables */
    fenix_rt = {};
    fenix_rt.finalized = true;
    return FENIX_SUCCESS;
    FENIX_C_API_END
}
