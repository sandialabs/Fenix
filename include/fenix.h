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

#ifndef __FENIX__
#define __FENIX__

#include <mpi.h>
#include <setjmp.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
#include "fenix_data_subset.h"
#include "fenix_process_recovery.h"

#define FENIX_SUCCESS                         0
#define FENIX_ERROR_UNINITIALIZED            -9
#define FENIX_ERROR_NOCATEGORY              -10
#define FENIX_ERROR_CALLBACK_NOT_REGISTERD  -11
#define FENIX_ERROR_GROUP_CREATE            -12
#define FENIX_ERROR_MEMBER_CREATE           -13
#define FENIX_ERROR_COMMIT_BARRIER         -133
#define FENIX_ERROR_INVALID_GROUPID         -14
#define FENIX_ERROR_INVALID_MEMBERID        -15
#define FENIX_ERROR_INVALID_LOGIC_CALL     -155
#define FENIX_ERROR_INVALID_TIMESTAMP       -16
#define FENIX_ERROR_INVALID_DEPTH           -17
#define FENIX_ERROR_INVALID_ATTRIBUTE_NAME  -18
#define FENIX_ERROR_INVALID_ATTRIBUTE_VALUE -19
#define FENIX_ERROR_INVALID_POSITION        -20
#define FENIX_ERROR_DATA_WAIT               -21
#define FENIX_ERROR_SUBSET_NUM_BLOCKS       -22
#define FENIX_ERROR_SUBSET_START_OFFSET     -23
#define FENIX_ERROR_SUBSET_END_OFFSET       -24
#define FENIX_ERROR_SUBSET_STRIDE           -25
#define FENIX_ERROR_NODATA_FOUND            -30
#define FENIX_ERROR_INTERN                  -40
#define FENIX_ERROR_CANCELLED               -50
#define FENIX_WARNING_SPARE_RANKS_DEPLETED  100
#define FENIX_WARNING_PARTIAL_RESTORE       101

#define FENIX_DATA_GROUP_WORLD_ID            10
#define FENIX_GROUP_ID_MAX                   11
#define FENIX_TIME_STAMP_MAX                 12
#define FENIX_DATA_MEMBER_ALL                15
#define FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER   11
#define FENIX_DATA_MEMBER_ATTRIBUTE_COUNT    12
#define FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE 13
#define FENIX_DATA_MEMBER_ATTRIBUTE_SIZE     14
#define FENIX_DATA_SNAPSHOT_LATEST           -1
#define FENIX_DATA_SNAPSHOT_ALL              16
#define FENIX_DATA_SUBSET_CREATED             2

#define FENIX_DATA_POLICY_IN_MEMORY_RAID 13

typedef enum {
    FENIX_ROLE_INITIAL_RANK = 0,
    FENIX_ROLE_RECOVERED_RANK = 1,
    FENIX_ROLE_SURVIVOR_RANK = 2
} Fenix_Rank_role;

typedef struct {
    MPI_Request mpi_send_req;
    MPI_Request mpi_recv_req;
} Fenix_Request;

extern const Fenix_Data_subset  FENIX_DATA_SUBSET_FULL;
extern const Fenix_Data_subset  FENIX_DATA_SUBSET_EMPTY;

#define Fenix_Init(_role, _comm, _newcomm, _argc, _argv, _spare_ranks,  \
                   _spawn, _info, _error)                               \
    {                                                                   \
        static jmp_buf bufjmp;                                          \
        *(_role) = __fenix_preinit(_role, _comm, _newcomm, _argc,       \
                                   _argv, _spare_ranks, _spawn, _info,  \
                                   _error, &bufjmp);                    \
        if(setjmp(bufjmp)) {                                            \
            *(_role) = FENIX_ROLE_SURVIVOR_RANK;                        \
        }                                                               \
        __fenix_postinit( _error );                                     \
    }

int Fenix_Initialized(int *);

int Fenix_Callback_register(void (*recover)(MPI_Comm, int, void *),
                            void *callback_data);

int Fenix_get_number_of_ranks_with_role(int, int *);

int Fenix_get_role(MPI_Comm comm, int rank, int *role);

int Fenix_Finalize();

int Fenix_Data_group_create(int group_id, MPI_Comm, int start_time_stamp,
                            int depth, int policy_name, void* policy_value,
                            int* flag);

int Fenix_Data_member_create(int group_id, int member_id, void *buffer,
                             int count, MPI_Datatype datatype);

int Fenix_Data_group_get_redundancy_policy(int group_id, int* policy_name,
                                           void *policy_value, int *flag);

int Fenix_Data_wait(Fenix_Request request);

int Fenix_Data_test(Fenix_Request request, int *flag);

int Fenix_Data_member_store(int group_id, int member_id,
                            Fenix_Data_subset subset_specifier);

int Fenix_Data_member_storev(int member_id, int group_id,
                             Fenix_Data_subset subset_specifier);

int Fenix_Data_member_istore(int member_id, int group_id,
                             Fenix_Data_subset subset_specifier,
                             Fenix_Request *request);

int Fenix_Data_member_istorev(int member_id, int group_id,
                              Fenix_Data_subset subset_specifier,
                              Fenix_Request *request);

int Fenix_Data_commit(int group_id, int *time_stamp);

int Fenix_Data_commit_barrier(int group_id, int *time_stamp);

int Fenix_Data_barrier(int group_id);

int Fenix_Data_member_restore(int group_id, int member_id, void *target_buffer,
                              int max_count, int time_stamp, Fenix_Data_subset* found_data);

int Fenix_Data_member_restore_from_rank(int member_id, void *data, int max_count,
                                        int time_stamp, int group_id,
                                        int source_rank);

int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset,
                             int stride, Fenix_Data_subset *subset_specifier);

int Fenix_Data_subset_createv(int num_blocks, int *array_start_offsets,
                              int *array_end_offsets,
                              Fenix_Data_subset *subset_specifier);

int Fenix_Data_subset_delete(Fenix_Data_subset *subset_specifier);

int Fenix_Data_group_get_number_of_members(int group_id, int *number_of_members);

int Fenix_Data_group_get_member_at_position(int position, int *member_id,
                                            int group_id);

int Fenix_Data_group_get_number_of_snapshots(int group_id,
                                             int *number_of_snapshots);

int Fenix_Data_group_get_snapshot_at_position(int group_id, int position,
                                              int *time_stamp);

int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename,
                               void *attributevalue, int *flag, int source_rank);

int Fenix_Data_member_attr_set(int group_id, int member_id, int attribute_name,
                               void *attribute_value, int *flag);

int Fenix_Data_snapshot_delete(int group_id, int time_stamp);

int Fenix_Data_group_delete(int group_id);

int Fenix_Data_member_delete(int group_id, int member_id);

int Fenix_Process_fail_list(int** fail_list);

int Fenix_check_cancelled(MPI_Request *request, MPI_Status *status);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif // __FENIX__
