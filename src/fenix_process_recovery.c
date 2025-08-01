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

#include "fenix_ext.h"
#include "fenix_comm_list.h"
#include "fenix_process_recovery_global.h"
#include "fenix_process_recovery.h"
#include "fenix_data_group.h"
#include "fenix_data_recovery.h"
#include "fenix_opt.h"
#include "fenix_util.h"
#include <mpi.h>
#include <mpi-ext.h>

#include <sys/time.h>

int __fenix_preinit(int *role, MPI_Comm comm, MPI_Comm *new_comm, int *argc, char ***argv,
                    int spare_ranks,
                    int spawn,
                    MPI_Info info, int *error, jmp_buf *jump_environment)
{

    int ret;
    *role = fenix.role;
    *error = 0;

    fenix.user_world = new_comm;

    MPI_Comm_create_errhandler(__fenix_test_MPI, &fenix.mpi_errhandler);
    
    fenix.world = malloc(sizeof(MPI_Comm));
    MPI_Comm_dup(comm, fenix.world);
    PMPI_Comm_set_errhandler(*fenix.world, fenix.mpi_errhandler);

    fenix.finalized = 0;
    fenix.spare_ranks = spare_ranks;
    fenix.spawn_policy = spawn;
    fenix.recover_environment = jump_environment;
    fenix.role = FENIX_ROLE_INITIAL_RANK;
    fenix.fail_world_size = 0;
    fenix.ignore_errs = 0;
    fenix.resume_mode = __FENIX_RESUME_AT_INIT;
    fenix.repair_result = 0;
    fenix.ret_role = role;
    fenix.ret_error = error;

    fenix.options.verbose = -1;
    // __fenix_init_opt(*argc, *argv);

    // For request tracking, make sure we can save at least an integer
    // in MPI_Request
    if(sizeof(MPI_Request) < sizeof(int)) {
        fprintf(stderr, "FENIX ERROR: __fenix_preinit: sizeof(MPI_Request) < sizeof(int)!\n");
        MPI_Abort(comm, -1);
    }


    MPI_Op_create((MPI_User_function *) __fenix_ranks_agree, 1, &fenix.agree_op);

    /* Check the values in info */
    if (info != MPI_INFO_NULL) {
        char value[MPI_MAX_INFO_VAL + 1];
        int vallen = MPI_MAX_INFO_VAL;
        int flag;

        MPI_Info_get(info, "FENIX_RESUME_MODE", vallen, value, &flag);
        if (flag == 1) {
            if (strcmp(value, "Fenix_init") == 0) {
                fenix.resume_mode = __FENIX_RESUME_AT_INIT;
                if (fenix.options.verbose == 0) {
                    verbose_print("rank: %d, role: %d, value: %s\n",
                                  __fenix_get_current_rank(*fenix.world), fenix.role, value);
                }
            } else if (strcmp(value, "NO_JUMP") == 0) {
                fenix.resume_mode = __FENIX_RESUME_NO_JUMP;
                if (fenix.options.verbose == 0) {
                    verbose_print("rank: %d, role: %d, value: %s\n",
                                  __fenix_get_current_rank(*fenix.world), fenix.role, value);
                }

            } else {
                /* No support. Setting it to Fenix_init */
                fenix.resume_mode = __FENIX_RESUME_AT_INIT;
            }
        }
        
        
        MPI_Info_get(info, "FENIX_UNHANDLED_MODE", vallen, value, &flag);
        if (flag == 1) {
            if (strcmp(value, "SILENT") == 0) {
                fenix.print_unhandled = 0;
                if (fenix.options.verbose == 0) {
                    verbose_print("rank: %d, role: %d, UNHANDLED_MODE: %s\n",
                                  __fenix_get_current_rank(*fenix.world), fenix.role, value);
                }
            } else if (strcmp(value, "NO_JUMP") == 0) {
                fenix.print_unhandled = 1;
                if (fenix.options.verbose == 0) {
                    verbose_print("rank: %d, role: %d, UNHANDLED_MODE: %s\n",
                                  __fenix_get_current_rank(*fenix.world), fenix.role, value);
                }

            } else {
                /* No support. Setting it to silent */
                fenix.print_unhandled = 0;
            }
        }
    }

    if (fenix.spare_ranks >= __fenix_get_world_size(comm)) {
        debug_print("Fenix: <%d> spare ranks requested are unavailable\n",
                    fenix.spare_ranks);
    }

    fenix.data_recovery = __fenix_data_recovery_init();

    /*****************************************************/
    /* Note: fenix.new_world is only valid for the   */
    /*       active MPI ranks. Spare ranks do not        */
    /*       allocate any communicator content with this.*/
    /*       Any MPI calls in spare ranks with new_world */
    /*       trigger an abort.                           */
    /*****************************************************/

    ret = 1;
    while (ret) {
        ret = __fenix_create_new_world();
        if (ret) {
            // just_repair_process();
        }
    }

    if ( __fenix_spare_rank() != 1) {
        fenix.num_inital_ranks = __fenix_get_world_size(fenix.new_world);
        if (fenix.options.verbose == 0) {
            verbose_print("rank: %d, role: %d, number_initial_ranks: %d\n",
                          __fenix_get_current_rank(*fenix.world), fenix.role,
                          fenix.num_inital_ranks);   
        }

    } else {
        fenix.num_inital_ranks = spare_ranks;

        if (fenix.options.verbose == 0) {
            verbose_print("rank: %d, role: %d, number_initial_ranks: %d\n",
                          __fenix_get_current_rank(*fenix.world), fenix.role,
                          fenix.num_inital_ranks);   
        }
    }

    fenix.num_survivor_ranks = 0;
    fenix.num_recovered_ranks = 0;

    while ( __fenix_spare_rank() == 1) {
        int a;
        int myrank;
        MPI_Status mpi_status;
        fenix.ignore_errs = 1;
        ret = PMPI_Recv(&a, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, *fenix.world,
                        &mpi_status); // listen for a failure
        fenix.ignore_errs = 0;
        if (ret == MPI_SUCCESS) {
            if (fenix.options.verbose == 0) {
                verbose_print("Finalize the program; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*fenix.world), fenix.role);
            }
            __fenix_finalize_spare();
        } else if(ret == MPI_ERR_REVOKED){
            fenix.repair_result = __fenix_repair_ranks();
            if (fenix.options.verbose == 0) {
                verbose_print("spare rank exiting from MPI_Recv - repair ranks; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*fenix.world), fenix.role);
            }
        } else {
            MPIX_Comm_ack_failed(*fenix.world, __fenix_get_world_size(*fenix.world), &a);
        }
        fenix.role = FENIX_ROLE_RECOVERED_RANK;
    }

    
    if(fenix.role != FENIX_ROLE_RECOVERED_RANK) MPI_Comm_dup(fenix.new_world, fenix.user_world);
    fenix.user_world_exists = 1;

    return fenix.role;
}

int __fenix_spare_rank_within(MPI_Comm refcomm)
{
    int result = -1;
    int current_rank = __fenix_get_current_rank(refcomm);
    int new_world_size = __fenix_get_world_size(refcomm) - fenix.spare_ranks;
    if (current_rank >= new_world_size) {
        if (fenix.options.verbose == 6) {
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

        if (fenix.options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(from_comm),
                          fenix.role);
        }

        ret = PMPI_Comm_split(from_comm, MPI_UNDEFINED, current_rank,
                              &fenix.new_world);
        //if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_split: %d\n", ret); }
        fenix.new_world_exists = 0; //Should already be this

    } else {

        int current_rank = __fenix_get_current_rank(from_comm);

        if (fenix.options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(from_comm),
                          fenix.role);
        }

        ret = PMPI_Comm_split(from_comm, 0, current_rank, &fenix.new_world);
        fenix.new_world_exists = 1;
        if (ret != MPI_SUCCESS){
            fenix.new_world_exists = 0;
        }

    }
    return ret;
}

int __fenix_create_new_world(){
    return __fenix_create_new_world_from(*fenix.world);
}

int __fenix_repair_ranks()
{
    /*********************************************************/
    /* Do not forget comm_free for broken communicators      */
    /*********************************************************/
    fenix.ignore_errs = 1;

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
    current_rank = __fenix_get_current_rank(*fenix.world);
    world_size = __fenix_get_world_size(*fenix.world);

    //Double check that every process is here, not in some local error handling elsewhere.
    //Assume that other locations will converge here.
    if(__fenix_spare_rank() != 1){
    	int location = FENIX_ERRHANDLER_LOC;
    	do {
	    location = FENIX_ERRHANDLER_LOC;
	    MPIX_Comm_agree(*fenix.user_world, &location);
        } while(location != FENIX_ERRHANDLER_LOC);
    }
    
    while (!repair_success) {
	
        repair_success = 1;
        ret = MPIX_Comm_shrink(*fenix.world, &world_without_failures);
        //if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_shrink. repair_ranks\n"); }
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            goto END_LOOP;
        }

        /*********************************************************/
        /* Free up the storage for active process communicator   */
        /*********************************************************/
        if ( __fenix_spare_rank() != 1) {
            if(fenix.new_world_exists) PMPI_Comm_free(&fenix.new_world);
            if(fenix.user_world_exists) PMPI_Comm_free(fenix.user_world);
            fenix.user_world_exists = 0;
            fenix.new_world_exists = 0;
        }
        /*********************************************************/
        /* Need closer look above                                */
        /*********************************************************/

        survivor_world_size = __fenix_get_world_size(world_without_failures);
        fenix.fail_world_size = world_size - survivor_world_size;

        if (fenix.options.verbose == 2) {
            verbose_print(
                "current_rank: %d, role: %d, world_size: %d, fail_world_size: %d, survivor_world_size: %d\n",
                current_rank, fenix.role, world_size,
                fenix.fail_world_size, survivor_world_size);
        }

        if (fenix.spare_ranks < fenix.fail_world_size) {
            /* Not enough spare ranks */

            if (fenix.options.verbose == 2) {
                verbose_print(
                    "current_rank: %d, role: %d, spare_ranks: %d, fail_world_size: %d\n",
                    current_rank, fenix.role, fenix.spare_ranks,
                    fenix.fail_world_size);
            }

            if (fenix.spawn_policy == 1) {
                debug_print("Spawn policy <%d>is not supported\n", fenix.spawn_policy);
            } else {

                rt_code = FENIX_WARNING_SPARE_RANKS_DEPLETED;

                if (fenix.spare_ranks != 0) {

                    /***************************************/
                    /* Fill the ranks in increasing order  */
                    /***************************************/

                    int active_ranks;

                    survivor_world = (int *) s_malloc(survivor_world_size * sizeof(int));

                    ret = PMPI_Allgather(&current_rank, 1, MPI_INT, survivor_world, 1, MPI_INT,
                                         world_without_failures);

                    if (fenix.options.verbose == 2) {
                        int index;
                        for (index = 0; index < survivor_world_size; index++) {
                            verbose_print("current_rank: %d, role: %d, survivor_world[%d]: %d\n",
                                          current_rank, fenix.role, index,
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
                    if (fenix.role == FENIX_ROLE_SURVIVOR_RANK) {
                        survived_flag = 1;
                    }

                    ret = PMPI_Allreduce(&survived_flag, &fenix.num_survivor_ranks, 1,
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

                    fenix.num_inital_ranks = 0;

                    /* recovered ranks must be the number of spare ranks */
                    fenix.num_recovered_ranks = fenix.fail_world_size;

                    if (fenix.options.verbose == 2) {
                        verbose_print("current_rank: %d, role: %d, recovered_ranks: %d\n",
                                      current_rank, fenix.role,
                                      fenix.num_recovered_ranks);
                    }
                    
                    if(fenix.role != FENIX_ROLE_INITIAL_RANK){
                        free(fenix.fail_world);
                    }
                    fenix.fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size,
                                                        fenix.fail_world_size);

                    if (fenix.options.verbose == 2) {
                        int index;
                        for (index = 0; index < fenix.fail_world_size; index++) {
                            verbose_print("fail_world[%d]: %d\n", index, fenix.fail_world[index]);
                        }
                    }

                    free(survivor_world);

                    active_ranks = world_size - fenix.spare_ranks;

                    if (fenix.options.verbose == 2) {
                        verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                                      current_rank, fenix.role,
                                      active_ranks);
                    }

                    /* Assign new rank for reordering */
                    if (current_rank >= active_ranks) { // reorder ranks
                        int rank_offset = ((world_size - 1) - current_rank);
                      
                        for(int fail_i = 0; fail_i < fenix.fail_world_size; fail_i++){
                          if(fenix.fail_world[fail_i] > current_rank) rank_offset--;
                        }

                        if (rank_offset < fenix.fail_world_size) {
                            if (fenix.options.verbose == 11) {
                                verbose_print("reorder ranks; current_rank: %d -> new_rank: %d\n",
                                              current_rank, fenix.fail_world[rank_offset]);
                            }
                            current_rank = fenix.fail_world[rank_offset];
                        }
                    }

                    /************************************/
                    /* Update the number of spare ranks */
                    /************************************/
                    fenix.spare_ranks = 0;

                    //debug_print("not enough spare ranks to repair rank failures. repair_ranks\n");
                }

                /****************************************************************/
                /* No rank reordering is required if no spare rank is available */
                /****************************************************************/

            }
        } else {

            int active_ranks;

            survivor_world = (int *) s_malloc(survivor_world_size * sizeof(int));

            ret = PMPI_Allgather(&current_rank, 1, MPI_INT, survivor_world, 1, MPI_INT,
                                 world_without_failures);
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
            if (fenix.role == FENIX_ROLE_SURVIVOR_RANK) {
                survived_flag = 1;
            }

            ret = PMPI_Allreduce(&survived_flag, &fenix.num_survivor_ranks, 1,
                                 MPI_INT, MPI_SUM, world_without_failures);
            //if (ret != MPI_SUCCESS) { debug_print("MPI_Allreduce. repair_ranks\n"); }
            if (ret != MPI_SUCCESS) {
                repair_success = 0;
                if (ret != MPI_ERR_PROC_FAILED) {
                    MPIX_Comm_revoke(world_without_failures);
                }
                MPI_Comm_free(&world_without_failures);
                free(survivor_world);
                goto END_LOOP;
            }


            fenix.num_inital_ranks = 0;
            fenix.num_recovered_ranks = fenix.fail_world_size;
            
            if(fenix.role != FENIX_ROLE_INITIAL_RANK){
                free(fenix.fail_world);
            }

            fenix.fail_world = (int *) s_malloc(fenix.fail_world_size * sizeof(int));
            fenix.fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size, fenix.fail_world_size);
            free(survivor_world);

            if (fenix.options.verbose == 2) {
                int index;
                for (index = 0; index < fenix.fail_world_size; index++) {
                    verbose_print("fail_world[%d]: %d\n", index, fenix.fail_world[index]);
                }
            }

            active_ranks = world_size - fenix.spare_ranks;

            if (fenix.options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                              current_rank, fenix.role, active_ranks);
            }

            if (current_rank >= active_ranks) { // reorder ranks
                int rank_offset = ((world_size - 1) - current_rank);

                for(int fail_i = 0; fail_i < fenix.fail_world_size; fail_i++){
                  if(fenix.fail_world[fail_i] > current_rank) rank_offset--;
                }

                if (rank_offset < fenix.fail_world_size) {
                    if (fenix.options.verbose == 2) {
                        verbose_print("reorder ranks; current_rank: %d -> new_rank: %d (offset %d)\n",
                                      current_rank, fenix.fail_world[rank_offset], rank_offset);
                    }
                    current_rank = fenix.fail_world[rank_offset];
                }
            }

            /************************************/
            /* Update the number of spare ranks */
            /************************************/
            fenix.spare_ranks = fenix.spare_ranks - fenix.fail_world_size;
            if (fenix.options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, spare_ranks: %d\n",
                              current_rank, fenix.role,
                              fenix.spare_ranks);
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
            ret = MPI_Comm_dup(fenix.new_world, fenix.user_world);
            if (ret != MPI_SUCCESS){
                repair_success = 0;
                MPIX_Comm_revoke(fixed_world);
                MPI_Comm_free(&fixed_world);
                goto END_LOOP;
            }
            fenix.user_world_exists = 1;
        }
        
        ret = PMPI_Barrier(fixed_world);
        /* if (ret != MPI_SUCCESS) { debug_print("MPI_Barrier. repair_ranks\n"); } */
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            MPIX_Comm_revoke(fixed_world);
            MPI_Comm_free(&fixed_world);
            goto END_LOOP;
        }

    END_LOOP:
        num_try++;

        /*******************************************************/
        /*** Not sure if we should include verbose statement ***/
        /*******************************************************/

/*
  if (current_rank == FENIX_ROOT) {
  LDEBUG("Fenix: communicators repaired\n");
  }
*/
    }

    *fenix.world = fixed_world;
    fenix.ignore_errs=0;
    return rt_code;
}

int* __fenix_get_fail_ranks(int *survivor_world, int survivor_world_size, int fail_world_size)
{
    qsort(survivor_world, survivor_world_size, sizeof(int), __fenix_comparator);
    int failed_pos = 0;
    
    int *fail_ranks = calloc(fail_world_size, sizeof(int));

    int i;
    for (i = 0; i < survivor_world_size + fail_world_size; i++) {
        if (__fenix_binary_search(survivor_world, survivor_world_size, i) != 1) {
            if (fenix.options.verbose == 14) {
                verbose_print("fail_rank: %d, fail_ranks[%d]: %d\n", i, failed_pos,
                              fail_ranks[failed_pos++]);
            }
            fail_ranks[failed_pos++] = i;
        }
    }
    return fail_ranks;
}

int __fenix_spare_rank(){
    return __fenix_spare_rank_within(*fenix.world);
}

void __fenix_postinit(int *error)
{

    //if (fenix.options.verbose == 9) {
    //      verbose_print(" postinit: current_rank: %d, role: %d\n", __fenix_get_current_rank(fenix.new_world),
    //                fenix.role);
        //}

    if(fenix.new_world_exists){
        //Set up dummy irecv to use for checking for failures.
        MPI_Irecv(&fenix.dummy_recv_buffer, 1, MPI_INT, MPI_ANY_SOURCE,
                  34095347, fenix.new_world, &fenix.check_failures_req);
    }

    if (fenix.repair_result != 0) {
        *error = fenix.repair_result;
    }
    fenix.fenix_init_flag = 1;

#if 0
    if (fenix.role != FENIX_ROLE_INITIAL_RANK) {
        init_data_recovery();
    }
#endif

    if (fenix.role == FENIX_ROLE_SURVIVOR_RANK) {
        __fenix_callback_invoke_all(*error);
    }
    if (fenix.options.verbose == 9) {
        verbose_print("After barrier. current_rank: %d, role: %d\n", __fenix_get_current_rank(fenix.new_world),
                      fenix.role);
    }
}

int __fenix_detect_failures(int do_recovery){
    if(!fenix.new_world_exists) return FENIX_ERROR_UNINITIALIZED;

    int old_ignore_errs = fenix.ignore_errs;
    fenix.ignore_errs = !do_recovery;

    int req_completed;
    int ret = MPI_Test(&fenix.check_failures_req, &req_completed, MPI_STATUS_IGNORE);

    if(req_completed) ret = FENIX_ERROR_INTERN;
    
    fenix.ignore_errs = old_ignore_errs;
    return ret;
}

void __fenix_finalize()
{
    int location = FENIX_FINALIZE_LOC;
    MPIX_Comm_agree(*fenix.user_world, &location);
    if(location != FENIX_FINALIZE_LOC){
        //Some ranks are in error recovery, so trigger error handling.
        MPIX_Comm_revoke(*fenix.user_world);
        MPI_Barrier(*fenix.user_world);
        
        //In case no-jump enabled after recovery
        return __fenix_finalize();
    }

    int first_spare_rank = __fenix_get_world_size(*fenix.user_world);
    int last_spare_rank = __fenix_get_world_size(*fenix.world) - 1;

    //If we've reached here, we will finalized regardless of further errors.
    fenix.ignore_errs = 1;
    while(!fenix.finalized){
        int user_rank = __fenix_get_current_rank(*fenix.user_world);

        if (user_rank == 0) {
            for (int i = first_spare_rank; i <= last_spare_rank; i++) {
                //We don't care if a spare failed, ignore return value
                int unused;
                MPI_Send(&unused, 1, MPI_INT, i, 1, *fenix.world);
            }
        }

        //We need to confirm that rank 0 didn't fail, since it could have
        //failed before notifying some spares to leave.
        int need_retry = user_rank == 0 ? 0 : 1;
        MPIX_Comm_agree(*fenix.user_world, &need_retry);
        if(need_retry == 1){
            //Rank 0 didn't contribute, so we need to retry.
            MPIX_Comm_shrink(*fenix.user_world, fenix.user_world);
            continue;
        } else {
            //If rank 0 did contribute, we know sends made it, and regardless
            //of any other failures we finalize.
            fenix.finalized = 1;
        }
    }

    //Now we do one last agree w/ the spares to let them know they can actually
    //finalize
    int unused;
    MPIX_Comm_agree(*fenix.world, &unused);
    
    
    MPI_Op_free( &fenix.agree_op );
    MPI_Comm_set_errhandler( *fenix.world, MPI_ERRORS_ARE_FATAL );
    MPI_Comm_free( fenix.world );
    free(fenix.world);
    if(fenix.new_world_exists) MPI_Comm_free( &fenix.new_world ); //It should, but just in case. Won't update because trying to free it again ought to generate an error anyway.

    if(fenix.role != FENIX_ROLE_INITIAL_RANK){
        free(fenix.fail_world);
    }

    /* Free Callbacks */
    __fenix_callback_destroy( fenix.callback_list );

    /* Free data recovery interface */
    __fenix_data_recovery_destroy( fenix.data_recovery );

    fenix.fenix_init_flag = 0;
}

void __fenix_finalize_spare()
{
    fenix.fenix_init_flag = 0;

    int unused;
    MPI_Request agree_req, recv_req = MPI_REQUEST_NULL;

    MPIX_Comm_iagree(*fenix.world, &unused, &agree_req);
    while(true){
        int completed = 0;
        MPI_Test(&agree_req, &completed, MPI_STATUS_IGNORE);
        if(completed) break;

        int ret = MPI_Test(&recv_req, &completed, MPI_STATUS_IGNORE);
        if(completed){
            //We may get duplicate messages informing us to exit
            MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, *fenix.world, &recv_req);
        }
        if(ret != MPI_SUCCESS){
            MPIX_Comm_ack_failed(*fenix.world, __fenix_get_world_size(*fenix.world), &unused);
        }
    }

    if(recv_req != MPI_REQUEST_NULL) MPI_Cancel(&recv_req);
 
    MPI_Op_free(&fenix.agree_op);
    MPI_Comm_set_errhandler(*fenix.world, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_free(fenix.world);

    /* Free callbacks */
    __fenix_callback_destroy( fenix.callback_list );

    /* Free data recovery interface */
    __fenix_data_recovery_destroy( fenix.data_recovery );

    fenix.fenix_init_flag = 0;

    /* Future version do not close MPI. Jump to where Fenix_Finalize is called. */
    MPI_Finalize();
    exit(0);
}

void __fenix_test_MPI(MPI_Comm *pcomm, int *pret, ...)
{
    int ret_repair;
    int index;
    int ret = *pret;
    if(!fenix.fenix_init_flag || __fenix_spare_rank() == 1 || fenix.ignore_errs) {
        return;
    }

    switch (ret) {
    case MPI_ERR_PROC_FAILED_PENDING:
    case MPI_ERR_PROC_FAILED:
        MPIX_Comm_revoke(*fenix.world);
        MPIX_Comm_revoke(fenix.new_world);
        
        if(fenix.user_world_exists) MPIX_Comm_revoke(*fenix.user_world);


        __fenix_comm_list_destroy();

        fenix.repair_result = __fenix_repair_ranks();
        break;
    case MPI_ERR_REVOKED:
        __fenix_comm_list_destroy();

        fenix.repair_result = __fenix_repair_ranks();
        break;
    case MPI_ERR_INTERN:
        printf("Fenix detected error: MPI_ERR_INTERN\n");
    default:
        if(fenix.print_unhandled){
            int len;
            char errstr[MPI_MAX_ERROR_STRING];
            MPI_Error_string(ret, errstr, &len);
            fprintf(stderr, "UNHANDLED ERR: %s\n", errstr);
        }
        return;
        break;
    }


    fenix.role = FENIX_ROLE_SURVIVOR_RANK;
    if(!fenix.finalized) {
        switch(fenix.resume_mode) {
            case __FENIX_RESUME_AT_INIT:
                longjmp(*fenix.recover_environment, 1);
                break;
            case __FENIX_RESUME_NO_JUMP:
                *(fenix.ret_role) = FENIX_ROLE_SURVIVOR_RANK;
                __fenix_postinit(fenix.ret_error);
                break;
            default:
                printf("Fenix detected error: Unknown resume mode\n");
                assert(false);
                break;
        }
    }
}
