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
//        Rob Van der Wijngaart, and Michael Heroux
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

int __fenix_preinit(int *role, MPI_Comm comm, MPI_Comm *new_comm, int *argc, char ***argv,
                    int spare_ranks,
                    int spawn,
                    MPI_Info info, int *error, jmp_buf *jump_environment)
{

    int ret;
<<<<<<< HEAD
    *role = __fenix_g_role;
    *error = 0;

    if (new_comm != NULL) {
        __fenix_g_user_world = new_comm;
        __fenix_g_replace_comm_flag = 0;
    } else {
        __fenix_g_replace_comm_flag = 1;
=======
    *role = fenix.role;
    *error = 0;

    if (new_comm != NULL) {
        fenix.user_world = new_comm;
        fenix.replace_comm_flag = 0;
    } else {
        fenix.replace_comm_flag = 1;
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
    }

#warning "I think that by setting this errhandler, all other derived comms inherit it! no need for the gazillion set_errhandler calls in this file!"
#warning "When was the last time I tried to set an actual errhndlr? Maybe it works, now"
    PMPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

<<<<<<< HEAD
    __fenix_g_original_comm = comm;
    assert(__fenix_g_original_comm == comm);

    __fenix_g_finalized = 0;
    __fenix_g_spare_ranks = spare_ranks;
    __fenix_g_spawn_policy = spawn;
    __fenix_g_recover_environment = jump_environment;
    __fenix_g_role = FENIX_ROLE_INITIAL_RANK;
    __fenix_g_resume_mode = __FENIX_RESUME_AT_INIT;
    __fenix_g_repair_result = 0;

    __fenix_options.verbose = -1;
=======
    fenix.original_comm = comm;
    assert(fenix.original_comm == comm);

    fenix.finalized = 0;
    fenix.spare_ranks = spare_ranks;
    fenix.spawn_policy = spawn;
    fenix.recover_environment = jump_environment;
    fenix.role = FENIX_ROLE_INITIAL_RANK;
    fenix.resume_mode = __FENIX_RESUME_AT_INIT;
    fenix.repair_result = 0;

    fenix.options.verbose = -1;
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
    // __fenix_init_opt(*argc, *argv);

    // For request tracking, make sure we can save at least an integer
    // in MPI_Request
    if(sizeof(MPI_Request) < sizeof(int)) {
        fprintf(stderr, "FENIX ERROR: __fenix_preinit: sizeof(MPI_Request) < sizeof(int)!\n");
        MPI_Abort(comm, -1);
    }

    // Initialize request store
<<<<<<< HEAD
    __fenix_request_store_init(&__fenix_g_request_store);


    MPI_Op_create((MPI_User_function *) __fenix_ranks_agree, 1, &__fenix_g_agree_op);
=======
    __fenix_request_store_init(&fenix.request_store);


    MPI_Op_create((MPI_User_function *) __fenix_ranks_agree, 1, &fenix.agree_op);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c

    /* Check the values in info */
    if (info != MPI_INFO_NULL) {
        char value[MPI_MAX_INFO_VAL + 1];
        int vallen = MPI_MAX_INFO_VAL;
        int flag;
        MPI_Info_get(info, "FENIX_RESUME_MODE", vallen, value, &flag);

        if (flag == 1) {
            if (strcmp(value, "Fenix_init") == 0) {
<<<<<<< HEAD
                __fenix_g_resume_mode = __FENIX_RESUME_AT_INIT;
                if (__fenix_options.verbose == 0) {
                    verbose_print("rank: %d, role: %d, value: %s\n",
                                  __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, value);
                }
            } else if (strcmp(value, "NO_JUMP") == 0) {
                __fenix_g_resume_mode = __FENIX_RESUME_NO_JUMP;
                if (__fenix_options.verbose == 0) {
                    verbose_print("rank: %d, role: %d, value: %s\n",
                                  __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, value);
=======
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
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
                }

            } else {
                /* No support. Setting it to Fenix_init */
<<<<<<< HEAD
                __fenix_g_resume_mode = __FENIX_RESUME_AT_INIT;
=======
                fenix.resume_mode = __FENIX_RESUME_AT_INIT;
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
            }
        }
    }

<<<<<<< HEAD
    if (__fenix_g_spare_ranks >= __fenix_get_world_size(comm)) {
        debug_print("Fenix: <%d> spare ranks requested are unavailable\n",
                    __fenix_g_spare_ranks);
    }

    __fenix_g_world = (MPI_Comm *) s_malloc(sizeof(MPI_Comm));

    MPI_Comm_dup(comm, __fenix_g_world);

    __fenix_g_data_recovery = __fenix_data_group_init();

    __fenix_g_new_world = (MPI_Comm *) s_malloc(sizeof(MPI_Comm));

    /*****************************************************/
    /* Note: __fenix_g_new_world is only valid for the   */
=======
    if (fenix.spare_ranks >= __fenix_get_world_size(comm)) {
        debug_print("Fenix: <%d> spare ranks requested are unavailable\n",
                    fenix.spare_ranks);
    }

    fenix.world = (MPI_Comm *) s_malloc(sizeof(MPI_Comm));

    MPI_Comm_dup(comm, fenix.world);

    fenix.data_recovery = __fenix_data_group_init();

    fenix.new_world = (MPI_Comm *) s_malloc(sizeof(MPI_Comm));

    /*****************************************************/
    /* Note: fenix.new_world is only valid for the   */
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
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
<<<<<<< HEAD
        __fenix_g_num_inital_ranks = __fenix_get_world_size(*__fenix_g_new_world);
        if (__fenix_options.verbose == 0) {
            verbose_print("rank: %d, role: %d, number_initial_ranks: %d\n",
                          __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role,
                          __fenix_g_num_inital_ranks);   
        }

    } else {
        __fenix_g_num_inital_ranks = spare_ranks;

        if (__fenix_options.verbose == 0) {
            verbose_print("rank: %d, role: %d, number_initial_ranks: %d\n",
                          __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role,
                          __fenix_g_num_inital_ranks);   
        }
    }

    __fenix_g_num_survivor_ranks = 0;
    __fenix_g_num_recovered_ranks = 0;
=======
        fenix.num_inital_ranks = __fenix_get_world_size(*fenix.new_world);
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
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c

    while ( __fenix_spare_rank() == 1) {
        int a;
        int myrank;
        MPI_Status mpi_status;
<<<<<<< HEAD
        debug_print("spare %d going back inside the Recv\n", __fenix_get_current_rank(*__fenix_g_world));
        ret = PMPI_Recv(&a, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, *__fenix_g_world,
                        &mpi_status); // listen for a failure
        if (ret == MPI_SUCCESS) {
            //if (__fenix_options.verbose == 0) {
                verbose_print("Finalize the program; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role);
                //}
            __fenix_finalize_spare();
        } else {
            __fenix_g_repair_result = __fenix_repair_ranks();
            //if (__fenix_options.verbose == 0) {
                verbose_print("spare rank exiting from MPI_Recv - repair ranks; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role);
                //}
        }
        __fenix_g_role = FENIX_ROLE_RECOVERED_RANK;
    }

    return __fenix_g_role;
=======
        debug_print("spare %d going back inside the Recv\n", __fenix_get_current_rank(*fenix.world));
        ret = PMPI_Recv(&a, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, *fenix.world,
                        &mpi_status); // listen for a failure
        if (ret == MPI_SUCCESS) {
            //if (fenix.options.verbose == 0) {
                verbose_print("Finalize the program; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*fenix.world), fenix.role);
                //}
            __fenix_finalize_spare();
        } else {
            fenix.repair_result = __fenix_repair_ranks();
            //if (fenix.options.verbose == 0) {
                verbose_print("spare rank exiting from MPI_Recv - repair ranks; rank: %d, role: %d\n",
                              __fenix_get_current_rank(*fenix.world), fenix.role);
                //}
        }
        fenix.role = FENIX_ROLE_RECOVERED_RANK;
    }

    return fenix.role;
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
}

int __fenix_create_new_world()
{
    int ret;
<<<<<<< HEAD
    ret = PMPI_Comm_set_errhandler(*__fenix_g_world, MPI_ERRORS_RETURN);

    if ( __fenix_spare_rank() == 1) {
        int current_rank = __fenix_get_current_rank(*__fenix_g_world);

        /*************************************************************************/
        /** MPI_UNDEFINED makes the new communicator "undefined" at spare ranks **/
        /** This means no data is allocated at spare ranks                      **/
        /** Use of the new communicator triggers the program to abort .         **/
        /*************************************************************************/

        if (__fenix_options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(*__fenix_g_world),
                          __fenix_g_role);
        }

        ret = PMPI_Comm_split(*__fenix_g_world, MPI_UNDEFINED, current_rank,
                              __fenix_g_new_world);
=======
    ret = PMPI_Comm_set_errhandler(*fenix.world, MPI_ERRORS_RETURN);

    if ( __fenix_spare_rank() == 1) {
        int current_rank = __fenix_get_current_rank(*fenix.world);

        /*************************************************************************/
        /** MPI_UNDEFINED makes the new communicator "undefined" at spare ranks **/
        /** This means no data is allocated at spare ranks                      **/
        /** Use of the new communicator triggers the program to abort .         **/
        /*************************************************************************/

        if (fenix.options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(*fenix.world),
                          fenix.role);
        }

        ret = PMPI_Comm_split(*fenix.world, MPI_UNDEFINED, current_rank,
                              fenix.new_world);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
        if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_split: %d\n", ret); }

    } else {

<<<<<<< HEAD
        int current_rank = __fenix_get_current_rank(*__fenix_g_world);

        if (__fenix_options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(*__fenix_g_world),
                          __fenix_g_role);
        }

        ret = PMPI_Comm_split(*__fenix_g_world, 0, current_rank, __fenix_g_new_world);
        if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_split: %d\n", ret); }
        MPI_Comm_set_errhandler(*__fenix_g_new_world, MPI_ERRORS_RETURN);
=======
        int current_rank = __fenix_get_current_rank(*fenix.world);

        if (fenix.options.verbose == 1) {
            verbose_print("rank: %d, role: %d\n", __fenix_get_current_rank(*fenix.world),
                          fenix.role);
        }

        ret = PMPI_Comm_split(*fenix.world, 0, current_rank, fenix.new_world);
        if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_split: %d\n", ret); }
        MPI_Comm_set_errhandler(*fenix.new_world, MPI_ERRORS_RETURN);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c

    }
    return ret;
}

int __fenix_repair_ranks()
{
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
    MPI_Comm world_without_failures;

<<<<<<< HEAD
    //if (__fenix_get_current_rank(*__fenix_g_world) == 0) {
    printf("%d Fenix: repairing communicators\n", __fenix_get_current_rank(*__fenix_g_world));
=======
    //if (__fenix_get_current_rank(*fenix.world) == 0) {
    printf("%d Fenix: repairing communicators\n", __fenix_get_current_rank(*fenix.world));
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
        //}

    while (!repair_success) {
        repair_success = 1;
<<<<<<< HEAD
        ret = MPIF_Comm_shrink(*__fenix_g_world, &world_without_failures);
        /* if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_shrink. repair_ranks\n"); } */
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            goto END_LOOP;
        }

        ret = MPI_Comm_set_errhandler(world_without_failures, MPI_ERRORS_RETURN);
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            MPI_Comm_free(&world_without_failures);
            goto END_LOOP;
        }

        /*********************************************************/
        /* Free up the storage for active process communicator   */
        /*********************************************************/
        if ( __fenix_spare_rank() != 1) {
            PMPI_Comm_free(__fenix_g_new_world);
            if ( __fenix_g_replace_comm_flag == 0) {
                PMPI_Comm_free(__fenix_g_user_world);
            }
        }
        /*********************************************************/
        /* Need closer look above                                */
        /*********************************************************/

        /* current_rank means the global MPI rank before failure */
        current_rank = __fenix_get_current_rank(*__fenix_g_world);
        survivor_world_size = __fenix_get_world_size(world_without_failures);
        world_size = __fenix_get_world_size(*__fenix_g_world);
        fail_world_size = world_size - survivor_world_size;

        if (__fenix_options.verbose == 2) {
            verbose_print(
                "current_rank: %d, role: %d, world_size: %d, fail_world_size: %d, survivor_world_size: %d\n",
                __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, world_size,
                fail_world_size, survivor_world_size);
        }

        if (__fenix_g_spare_ranks < fail_world_size) {
            /* Not enough spare ranks */

            if (__fenix_options.verbose == 2) {
                verbose_print(
                    "current_rank: %d, role: %d, spare_ranks: %d, fail_world_size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, __fenix_g_spare_ranks,
                    fail_world_size);
            }

            if (__fenix_g_spawn_policy == 1) {
                debug_print("Spawn policy <%d>is not supported\n", __fenix_g_spawn_policy);
            } else {

                rt_code = FENIX_WARNING_SPARE_RANKS_DEPLETED;

                if (__fenix_g_spare_ranks != 0) {

                    /***************************************/
                    /* Fill the ranks in increasing order  */
                    /***************************************/

                    int active_ranks;

                    survivor_world = (int *) s_malloc(survivor_world_size * sizeof(int));

                    ret = PMPI_Allgather(&current_rank, 1, MPI_INT, survivor_world, 1, MPI_INT,
                                         world_without_failures);

                    if (__fenix_options.verbose == 2) {
                        int index;
                        for (index = 0; index < survivor_world_size; index++) {
                            verbose_print("current_rank: %s, role: %d, survivor_world[%d]: %d\n",
                                          __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, index,
                                          survivor_world[index]);
                        }
                    }

                    /* if (ret != MPI_SUCCESS) { debug_print("MPI_Allgather. repair_ranks\n"); } */
                    if (ret != MPI_SUCCESS) {
                        repair_success = 0;
                        if (ret == MPI_ERR_PROC_FAILED) {
                            MPIF_Comm_revoke(world_without_failures);
                        }
                        MPI_Comm_free(&world_without_failures);
                        free(survivor_world);
                        goto END_LOOP;
                    }

                    survived_flag = 0;
                    if (__fenix_g_role == FENIX_ROLE_SURVIVOR_RANK) {
                        survived_flag = 1;
                    }

                    ret = PMPI_Allreduce(&survived_flag, &__fenix_g_num_survivor_ranks, 1,
                                         MPI_INT, MPI_SUM, world_without_failures);

                    /* if (ret != MPI_SUCCESS) { debug_print("MPI_Allreduce. repair_ranks\n"); } */
                    if (ret != MPI_SUCCESS) {
                        repair_success = 0;
                        if (ret == MPI_ERR_PROC_FAILED) {
                            MPIF_Comm_revoke(world_without_failures);
                        }
                        MPI_Comm_free(&world_without_failures);
                        free(survivor_world);
                        goto END_LOOP;
                    }

                    __fenix_g_num_inital_ranks = 0;

                    /* recovered ranks must be the number of spare ranks */
                    __fenix_g_num_recovered_ranks = fail_world_size;

                    if (__fenix_options.verbose == 2) {
                        verbose_print("current_rank: %d, role: %d, recovered_ranks: %d\n",
                                      __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role,
                                      __fenix_g_num_recovered_ranks);
                    }

                    fail_world = (int *) s_malloc(fail_world_size * sizeof(int));
                    fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size,
                                                        fail_world_size);

                    if (__fenix_options.verbose == 2) {
                        int index;
                        for (index = 0; index < fail_world_size; index++) {
                            verbose_print("fail_world[%d]: %d\n", index, fail_world[index]);
                        }
                    }

                    free(survivor_world);

                    active_ranks = world_size - __fenix_g_spare_ranks;

                    if (__fenix_options.verbose == 2) {
                        verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                                      __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role,
                                      active_ranks);
                    }

                    /* Assign new rank for reordering */
                    if (current_rank >= active_ranks) { // reorder ranks
                        int rank_offset = ((world_size - 1) - current_rank);
                        if (rank_offset < fail_world_size) {
                            if (__fenix_options.verbose == 11) {
                                verbose_print("reorder ranks; current_rank: %d -> new_rank: %d\n",
                                              current_rank, fail_world[rank_offset]);
                            }
                            current_rank = fail_world[rank_offset];
                        }
                    }

                    free(fail_world);

                    /************************************/
                    /* Update the number of spare ranks */
                    /************************************/
                    __fenix_g_spare_ranks = 0;

                    /* debug_print("not enough spare ranks to repair rank failures. repair_ranks\n"); */
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
            /* if (ret != MPI_SUCCESS) { debug_print("MPI_Allgather. repair_ranks\n"); } */
            if (ret != MPI_SUCCESS) {
                repair_success = 0;
                if (ret == MPI_ERR_PROC_FAILED) {
                    MPIF_Comm_revoke(world_without_failures);
                }
                MPI_Comm_free(&world_without_failures);
                free(survivor_world);
                goto END_LOOP;
            }

            survived_flag = 0;
            if (__fenix_g_role == FENIX_ROLE_SURVIVOR_RANK) {
                survived_flag = 1;
            }

            ret = PMPI_Allreduce(&survived_flag, &__fenix_g_num_survivor_ranks, 1,
                                 MPI_INT, MPI_SUM, world_without_failures);
            /*  if (ret != MPI_SUCCESS) { debug_print("MPI_Allreduce. repair_ranks\n"); } */
            if (ret != MPI_SUCCESS) {
                repair_success = 0;
                if (ret != MPI_ERR_PROC_FAILED) {
                    MPIF_Comm_revoke(world_without_failures);
                }
                MPI_Comm_free(&world_without_failures);
                free(survivor_world);
                goto END_LOOP;
            }

            __fenix_g_num_inital_ranks = 0;
            __fenix_g_num_recovered_ranks = fail_world_size;

            fail_world = (int *) s_malloc(fail_world_size * sizeof(int));
            fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size, fail_world_size);
            free(survivor_world);

            if (__fenix_options.verbose == 2) {
                int index;
                for (index = 0; index < fail_world_size; index++) {
                    verbose_print("fail_world[%d]: %d\n", index, fail_world[index]);
                }
            }

            active_ranks = world_size - __fenix_g_spare_ranks;

            if (__fenix_options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, active_ranks);
            }

            if (current_rank >= active_ranks) { // reorder ranks
                int rank_offset = ((world_size - 1) - current_rank);
                if (rank_offset < fail_world_size) {
                    if (__fenix_options.verbose == 2) {
                        verbose_print("reorder ranks; current_rank: %d -> new_rank: %d\n",
                                      current_rank, fail_world[rank_offset]);
                    }
                    current_rank = fail_world[rank_offset];
                }
            }

            free(fail_world);

            /************************************/
            /* Update the number of spare ranks */
            /************************************/
            __fenix_g_spare_ranks = __fenix_g_spare_ranks - fail_world_size;
            if (__fenix_options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, spare_ranks: %d\n",
                              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role,
                              __fenix_g_spare_ranks);
=======
        ret = MPIX_Comm_shrink(*fenix.world, &world_without_failures);
        /* if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_shrink. repair_ranks\n"); } */
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            goto END_LOOP;
        }

        ret = MPI_Comm_set_errhandler(world_without_failures, MPI_ERRORS_RETURN);
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            MPI_Comm_free(&world_without_failures);
            goto END_LOOP;
        }

        /*********************************************************/
        /* Free up the storage for active process communicator   */
        /*********************************************************/
        if ( __fenix_spare_rank() != 1) {
            PMPI_Comm_free(fenix.new_world);
            if ( fenix.replace_comm_flag == 0) {
                PMPI_Comm_free(fenix.user_world);
            }
        }
        /*********************************************************/
        /* Need closer look above                                */
        /*********************************************************/

        /* current_rank means the global MPI rank before failure */
        current_rank = __fenix_get_current_rank(*fenix.world);
        survivor_world_size = __fenix_get_world_size(world_without_failures);
        world_size = __fenix_get_world_size(*fenix.world);
        fail_world_size = world_size - survivor_world_size;

        if (fenix.options.verbose == 2) {
            verbose_print(
                "current_rank: %d, role: %d, world_size: %d, fail_world_size: %d, survivor_world_size: %d\n",
                __fenix_get_current_rank(*fenix.world), fenix.role, world_size,
                fail_world_size, survivor_world_size);
        }

        if (fenix.spare_ranks < fail_world_size) {
            /* Not enough spare ranks */

            if (fenix.options.verbose == 2) {
                verbose_print(
                    "current_rank: %d, role: %d, spare_ranks: %d, fail_world_size: %d\n",
                    __fenix_get_current_rank(*fenix.world), fenix.role, fenix.spare_ranks,
                    fail_world_size);
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
                            verbose_print("current_rank: %s, role: %d, survivor_world[%d]: %d\n",
                                          __fenix_get_current_rank(*fenix.world), fenix.role, index,
                                          survivor_world[index]);
                        }
                    }

                    /* if (ret != MPI_SUCCESS) { debug_print("MPI_Allgather. repair_ranks\n"); } */
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

                    /* if (ret != MPI_SUCCESS) { debug_print("MPI_Allreduce. repair_ranks\n"); } */
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
                    fenix.num_recovered_ranks = fail_world_size;

                    if (fenix.options.verbose == 2) {
                        verbose_print("current_rank: %d, role: %d, recovered_ranks: %d\n",
                                      __fenix_get_current_rank(*fenix.world), fenix.role,
                                      fenix.num_recovered_ranks);
                    }

                    fail_world = (int *) s_malloc(fail_world_size * sizeof(int));
                    fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size,
                                                        fail_world_size);

                    if (fenix.options.verbose == 2) {
                        int index;
                        for (index = 0; index < fail_world_size; index++) {
                            verbose_print("fail_world[%d]: %d\n", index, fail_world[index]);
                        }
                    }

                    free(survivor_world);

                    active_ranks = world_size - fenix.spare_ranks;

                    if (fenix.options.verbose == 2) {
                        verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                                      __fenix_get_current_rank(*fenix.world), fenix.role,
                                      active_ranks);
                    }

                    /* Assign new rank for reordering */
                    if (current_rank >= active_ranks) { // reorder ranks
                        int rank_offset = ((world_size - 1) - current_rank);
                        if (rank_offset < fail_world_size) {
                            if (fenix.options.verbose == 11) {
                                verbose_print("reorder ranks; current_rank: %d -> new_rank: %d\n",
                                              current_rank, fail_world[rank_offset]);
                            }
                            current_rank = fail_world[rank_offset];
                        }
                    }

                    free(fail_world);

                    /************************************/
                    /* Update the number of spare ranks */
                    /************************************/
                    fenix.spare_ranks = 0;

                    /* debug_print("not enough spare ranks to repair rank failures. repair_ranks\n"); */
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
            /* if (ret != MPI_SUCCESS) { debug_print("MPI_Allgather. repair_ranks\n"); } */
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
            /*  if (ret != MPI_SUCCESS) { debug_print("MPI_Allreduce. repair_ranks\n"); } */
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
            fenix.num_recovered_ranks = fail_world_size;

            fail_world = (int *) s_malloc(fail_world_size * sizeof(int));
            fail_world = __fenix_get_fail_ranks(survivor_world, survivor_world_size, fail_world_size);
            free(survivor_world);

            if (fenix.options.verbose == 2) {
                int index;
                for (index = 0; index < fail_world_size; index++) {
                    verbose_print("fail_world[%d]: %d\n", index, fail_world[index]);
                }
            }

            active_ranks = world_size - fenix.spare_ranks;

            if (fenix.options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, active_ranks: %d\n",
                              __fenix_get_current_rank(*fenix.world), fenix.role, active_ranks);
            }

            if (current_rank >= active_ranks) { // reorder ranks
                int rank_offset = ((world_size - 1) - current_rank);
                if (rank_offset < fail_world_size) {
                    if (fenix.options.verbose == 2) {
                        verbose_print("reorder ranks; current_rank: %d -> new_rank: %d\n",
                                      current_rank, fail_world[rank_offset]);
                    }
                    current_rank = fail_world[rank_offset];
                }
            }

            free(fail_world);

            /************************************/
            /* Update the number of spare ranks */
            /************************************/
            fenix.spare_ranks = fenix.spare_ranks - fail_world_size;
            if (fenix.options.verbose == 2) {
                verbose_print("current_rank: %d, role: %d, spare_ranks: %d\n",
                              __fenix_get_current_rank(*fenix.world), fenix.role,
                              fenix.spare_ranks);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
            }
        }

        /*********************************************************/
        /* Done with the global communicator                     */
        /*********************************************************/

        if (!flag_g_world_freed) {
<<<<<<< HEAD
            ret = PMPI_Comm_free(__fenix_g_world);
            if (ret != MPI_SUCCESS) { flag_g_world_freed = 1; }
        }
        ret = PMPI_Comm_split(world_without_failures, 0, current_rank, __fenix_g_world);
=======
            ret = PMPI_Comm_free(fenix.world);
            if (ret != MPI_SUCCESS) { flag_g_world_freed = 1; }
        }
        ret = PMPI_Comm_split(world_without_failures, 0, current_rank, fenix.world);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c

        /* if (ret != MPI_SUCCESS) { debug_print("MPI_Comm_split. repair_ranks\n"); } */
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            if (ret != MPI_ERR_PROC_FAILED) {
<<<<<<< HEAD
                MPIF_Comm_revoke(world_without_failures);
=======
                MPIX_Comm_revoke(world_without_failures);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
            }
            MPI_Comm_free(&world_without_failures);
            goto END_LOOP;
        }
        ret = PMPI_Comm_free(&world_without_failures);

        /* As of 8/8/2016                            */
        /* Need special treatment for error handling */
        __fenix_create_new_world();

<<<<<<< HEAD
        ret = PMPI_Barrier(*__fenix_g_world);
=======
        ret = PMPI_Barrier(*fenix.world);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
        /* if (ret != MPI_SUCCESS) { debug_print("MPI_Barrier. repair_ranks\n"); } */
        if (ret != MPI_SUCCESS) {
            repair_success = 0;
            if (ret != MPI_ERR_PROC_FAILED) {
<<<<<<< HEAD
                MPIF_Comm_revoke(*__fenix_g_world);
=======
                MPIX_Comm_revoke(*fenix.world);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
            }
        }

    END_LOOP:
        num_try++;

        /*******************************************************/
        /*** Not sure if we should include verbose statement ***/
        /*******************************************************/

/*
<<<<<<< HEAD
  if (__fenix_get_current_rank(*__fenix_g_world) == FENIX_ROOT) {
=======
  if (__fenix_get_current_rank(*fenix.world) == FENIX_ROOT) {
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
  LDEBUG("Fenix: communicators repaired\n");
  }
*/
    }
    return rt_code;
}

int* __fenix_get_fail_ranks(int *survivor_world, int survivor_world_size, int fail_world_size)
{
    qsort(survivor_world, survivor_world_size, sizeof(int), __fenix_comparator);
    int failed_pos = 0;
    int *fail_ranks = calloc(fail_world_size, sizeof(int));
    int i;
    for (i = 0; i < survivor_world_size; i++) {
        if (__fenix_binary_search(survivor_world, survivor_world_size, i) != 1) {
<<<<<<< HEAD
            if (__fenix_options.verbose == 14) {
=======
            if (fenix.options.verbose == 14) {
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
                verbose_print("fail_rank: %d, fail_ranks[%d]: %d\n", i, failed_pos,
                              fail_ranks[failed_pos++]);
            }
            fail_ranks[failed_pos++] = i;
        }
    }
    return fail_ranks;
}

int __fenix_spare_rank()
{
    int result = -1;
<<<<<<< HEAD
    int current_rank = __fenix_get_current_rank(*__fenix_g_world);
    int new_world_size = __fenix_get_world_size(*__fenix_g_world) - __fenix_g_spare_ranks;
    if (current_rank >= new_world_size) {
        if (__fenix_options.verbose == 6) {
=======
    int current_rank = __fenix_get_current_rank(*fenix.world);
    int new_world_size = __fenix_get_world_size(*fenix.world) - fenix.spare_ranks;
    if (current_rank >= new_world_size) {
        if (fenix.options.verbose == 6) {
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
            verbose_print("current_rank: %d, new_world_size: %d\n", current_rank, new_world_size);
        }
        result = 1;
    }
    return result;
}

void __fenix_postinit(int *error)
{

<<<<<<< HEAD
    //if (__fenix_options.verbose == 9) {
        verbose_print(" postinit: current_rank: %d, role: %d\n", __fenix_get_current_rank(*__fenix_g_new_world),
                      __fenix_g_role);
        //}

    PMPI_Barrier(*__fenix_g_new_world);


    if (__fenix_g_replace_comm_flag == 0) {
        PMPI_Comm_dup(*__fenix_g_new_world, __fenix_g_user_world);
        PMPI_Comm_set_errhandler(*__fenix_g_user_world, MPI_ERRORS_RETURN);
    } else {
        PMPI_Comm_set_errhandler(*__fenix_g_new_world, MPI_ERRORS_RETURN);
    }

    if (__fenix_g_repair_result != 0) {
        *error = __fenix_g_repair_result;
    }
    __fenix_g_fenix_init_flag = 1;

#if 0
    if (__fenix_g_role != FENIX_ROLE_INITIAL_RANK) {
=======
    //if (fenix.options.verbose == 9) {
        verbose_print(" postinit: current_rank: %d, role: %d\n", __fenix_get_current_rank(*fenix.new_world),
                      fenix.role);
        //}

    PMPI_Barrier(*fenix.new_world);


    if (fenix.replace_comm_flag == 0) {
        PMPI_Comm_dup(*fenix.new_world, fenix.user_world);
        PMPI_Comm_set_errhandler(*fenix.user_world, MPI_ERRORS_RETURN);
    } else {
        PMPI_Comm_set_errhandler(*fenix.new_world, MPI_ERRORS_RETURN);
    }

    if (fenix.repair_result != 0) {
        *error = fenix.repair_result;
    }
    fenix.fenix_init_flag = 1;

#if 0
    if (fenix.role != FENIX_ROLE_INITIAL_RANK) {
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
        init_data_recovery();
    }
#endif

<<<<<<< HEAD
    if (__fenix_g_role == FENIX_ROLE_SURVIVOR_RANK) {
        __fenix_callback_invoke_all(*error);
    }
    if (__fenix_options.verbose == 9) {
        verbose_print("After barrier. current_rank: %d, role: %d\n", __fenix_get_current_rank(*__fenix_g_new_world),
                      __fenix_g_role);
=======
    if (fenix.role == FENIX_ROLE_SURVIVOR_RANK) {
        __fenix_callback_invoke_all(*error);
    }
    if (fenix.options.verbose == 9) {
        verbose_print("After barrier. current_rank: %d, role: %d\n", __fenix_get_current_rank(*fenix.new_world),
                      fenix.role);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
    }
}

void __fenix_finalize()
{
<<<<<<< HEAD
    __fenix_g_finalized = 1;

    debug_print("Before barrier1  %d\n", __fenix_get_current_rank(*__fenix_g_new_world));
    /* Last Barrier Statement */
    MPI_Barrier( *__fenix_g_new_world );
=======
    fenix.finalized = 1;

    debug_print("Before barrier1  %d\n", __fenix_get_current_rank(*fenix.new_world));
    /* Last Barrier Statement */
    int ret = MPI_Barrier( *fenix.new_world );
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
    if (ret != MPI_SUCCESS) {
        __fenix_finalize();
        return;
    }
    
<<<<<<< HEAD
    if (__fenix_options.verbose == 10) {
        verbose_print("current_rank: %d, role: %d\n", __fenix_get_current_rank(*__fenix_g_new_world),
                      __fenix_g_role);
    }
    debug_print("After  barrier1  %d\n", __fenix_get_current_rank(*__fenix_g_new_world));


    if (__fenix_get_current_rank(*__fenix_g_world) == 0) {

        int spare_rank;
        MPI_Comm_size(*__fenix_g_world, &spare_rank);
        spare_rank--;
        int a;
        int i;
        for (i = 0; i < __fenix_g_spare_ranks; i++) {
            int ret = MPI_Send(&a, 1, MPI_INT, spare_rank, 1, *__fenix_g_world);
            //if (__fenix_options.verbose == 10) {
            debug_print("%d spare_rank: %d sending msg!\n", __fenix_get_current_rank(*__fenix_g_new_world), spare_rank);
=======
    if (fenix.options.verbose == 10) {
        verbose_print("current_rank: %d, role: %d\n", __fenix_get_current_rank(*fenix.new_world),
                      fenix.role);
    }
    debug_print("After  barrier1  %d\n", __fenix_get_current_rank(*fenix.new_world));


    if (__fenix_get_current_rank(*fenix.world) == 0) {

        int spare_rank;
        MPI_Comm_size(*fenix.world, &spare_rank);
        spare_rank--;
        int a;
        int i;
        for (i = 0; i < fenix.spare_ranks; i++) {
            int ret = MPI_Send(&a, 1, MPI_INT, spare_rank, 1, *fenix.world);
            //if (fenix.options.verbose == 10) {
            debug_print("%d spare_rank: %d sending msg!\n", __fenix_get_current_rank(*fenix.new_world), spare_rank);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
            //}
            if (ret != MPI_SUCCESS) {
                __fenix_finalize();
                return;
            }
            spare_rank--;
        }
    }


<<<<<<< HEAD
    debug_print("%d Finalize before MPI_Barrier2\n", __fenix_get_current_rank(*__fenix_g_world));
    int ret = MPI_Barrier(*__fenix_g_world);
    debug_print("%d Finalize after  MPI_Barrier2: %d\n", __fenix_get_current_rank(*__fenix_g_world), ret);
=======
    debug_print("%d Finalize before MPI_Barrier2\n", __fenix_get_current_rank(*fenix.world));
    ret = MPI_Barrier(*fenix.world);
    debug_print("%d Finalize after  MPI_Barrier2: %d\n", __fenix_get_current_rank(*fenix.world), ret);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
    if (ret != MPI_SUCCESS) {
        __fenix_finalize();
        return;
    }
    //if (ret != MPI_SUCCESS) { debug_print("MPI_Barrier: %d\n", ret); } 
<<<<<<< HEAD

    
    MPI_Op_free( &__fenix_g_agree_op );
    MPI_Comm_set_errhandler( *__fenix_g_world, MPI_ERRORS_ARE_FATAL );
    MPI_Comm_free( __fenix_g_world );
    MPI_Comm_free( __fenix_g_new_world );
    free( __fenix_g_world );
    free( __fenix_g_new_world );

    /* Free Callbacks */
    __fenix_callback_destroy( __fenix_g_callback_list );

    /* Free data recovery interface */
    __fenix_data_group_destroy( __fenix_g_data_recovery );

    /* Free the request store */
    __fenix_request_store_destroy(&__fenix_g_request_store);

    __fenix_g_fenix_init_flag = 0;
=======

    
    MPI_Op_free( &fenix.agree_op );
    MPI_Comm_set_errhandler( *fenix.world, MPI_ERRORS_ARE_FATAL );
    MPI_Comm_free( fenix.world );
    MPI_Comm_free( fenix.new_world );
    free( fenix.world );
    free( fenix.new_world );

    /* Free Callbacks */
    __fenix_callback_destroy( fenix.callback_list );

    /* Free data recovery interface */
    __fenix_data_group_destroy( fenix.data_recovery );

    /* Free the request store */
    __fenix_request_store_destroy(&fenix.request_store);

    fenix.fenix_init_flag = 0;
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
}

void __fenix_finalize_spare()
{
<<<<<<< HEAD
    __fenix_g_fenix_init_flag = 0;
    int ret = PMPI_Barrier(*__fenix_g_world);
    if (ret != MPI_SUCCESS) { debug_print("MPI_Barrier: %d\n", ret); } 
 
    MPI_Op_free(&__fenix_g_agree_op);
    MPI_Comm_set_errhandler(*__fenix_g_world, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_free(__fenix_g_world);

    /* This communicator is invalid for spare processes */
    /* MPI_Comm_free(__fenix_g_new_world); */ 
    free(__fenix_g_world);
    free(__fenix_g_new_world);

    /* Free callbacks */
    __fenix_callback_destroy( __fenix_g_callback_list );

    /* Free data recovery interface */
    __fenix_data_group_destroy( __fenix_g_data_recovery );

    __fenix_g_fenix_init_flag = 0;
=======
    fenix.fenix_init_flag = 0;
    int ret = PMPI_Barrier(*fenix.world);
    if (ret != MPI_SUCCESS) { debug_print("MPI_Barrier: %d\n", ret); } 
 
    MPI_Op_free(&fenix.agree_op);
    MPI_Comm_set_errhandler(*fenix.world, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_free(fenix.world);

    /* This communicator is invalid for spare processes */
    /* MPI_Comm_free(fenix.new_world); */ 
    free(fenix.world);
    free(fenix.new_world);

    /* Free callbacks */
    __fenix_callback_destroy( fenix.callback_list );

    /* Free data recovery interface */
    __fenix_data_group_destroy( fenix.data_recovery );

    fenix.fenix_init_flag = 0;
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c

    /* Future version do not close MPI. Jump to where Fenix_Finalize is called. */
    MPI_Finalize();
    exit(0);
}

void __fenix_test_MPI(int ret, const char *msg)
{
    int ret_repair;
    int index;
<<<<<<< HEAD
    if(!__fenix_g_fenix_init_flag || ret == MPI_SUCCESS || __fenix_spare_rank() == 1) {
=======
    if(!fenix.fenix_init_flag || ret == MPI_SUCCESS || __fenix_spare_rank() == 1) {
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
        return;
    }

    switch (ret) {
    case MPI_ERR_PROC_FAILED:
<<<<<<< HEAD
        MPIF_Comm_revoke(*__fenix_g_world);
        MPIF_Comm_revoke(*__fenix_g_new_world);

        if (__fenix_options.verbose == 20) {
            verbose_print(
                "MPI_ERR_PROC_FAILED; current_rank: %d, role: %d, msg: %s, MPIF_Comm_revoke: %d, MPIF_Comm_revoke: %s\n",
                msg, "world", "new_world");
        }

        if (__fenix_g_replace_comm_flag == 0) {
            MPIF_Comm_revoke(*__fenix_g_user_world);
        }

        __fenix_request_store_waitall_removeall(&__fenix_g_request_store);

        __fenix_comm_list_destroy();

        __fenix_g_repair_result = __fenix_repair_ranks();
        break;
    case MPI_ERR_REVOKED:
        if (__fenix_options.verbose == 20) {
            verbose_print("MPI_ERR_REVOKED; current_rank: %d, role: %d, msg: %s\n", msg);
        }

        __fenix_request_store_waitall_removeall(&__fenix_g_request_store);

        __fenix_comm_list_destroy();

        __fenix_g_repair_result = __fenix_repair_ranks();
        break;
    case MPI_ERR_INTERN:
        printf("MPI_ERR_INTERN\n");
=======
        MPIX_Comm_revoke(*fenix.world);
        MPIX_Comm_revoke(*fenix.new_world);

        if (fenix.replace_comm_flag == 0) {
            MPIX_Comm_revoke(*fenix.user_world);
        }

        __fenix_request_store_waitall_removeall(&fenix.request_store);

        __fenix_comm_list_destroy();

        fenix.repair_result = __fenix_repair_ranks();
        break;
    case MPI_ERR_REVOKED:
        __fenix_request_store_waitall_removeall(&fenix.request_store);

        __fenix_comm_list_destroy();

        fenix.repair_result = __fenix_repair_ranks();
        break;
    case MPI_ERR_INTERN:
        printf("Fenix detected error: MPI_ERR_INTERN\n");
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
    default: 
        return;
        break;
#ifdef MPICH
<<<<<<< HEAD
        MPIF_Comm_revoke(*__fenix_g_world);
        MPIF_Comm_revoke(*__fenix_g_new_world);
        //MPIF_Comm_revoke(*__fenix_g_user_world);
        __fenix_g_repair_result = __fenix_repair_ranks();
#endif
    }

    __fenix_g_role = FENIX_ROLE_SURVIVOR_RANK;
    if(!__fenix_g_finalized)
        longjmp(*__fenix_g_recover_environment, 1);
=======
        MPIX_Comm_revoke(*fenix.world);
        MPIX_Comm_revoke(*fenix.new_world);
        //MPIX_Comm_revoke(*fenix.user_world);
        fenix.repair_result = __fenix_repair_ranks();
#endif
    }

    fenix.role = FENIX_ROLE_SURVIVOR_RANK;
    if(!fenix.finalized)
        longjmp(*fenix.recover_environment, 1);
>>>>>>> 17a8befda45abf9f1eddbcb5c51b82e31977d90c
}
