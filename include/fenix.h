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
#include <stdio.h>

#include "fenix_init.h"

#if defined(MPIX_ERR_PROC_FAILED) && !defined(MPI_ERR_PROC_FAILED)
#define MPI_ERR_PROC_FAILED MPIX_ERR_PROC_FAILED
#endif

#if defined(MPIX_ERR_PROC_FAILED_PENDING) &&                                   \
  !defined(MPI_ERR_PROC_FAILED_PENDING)
#define MPI_ERR_PROC_FAILED_PENDING MPIX_ERR_PROC_FAILED_PENDING
#endif

#if defined(MPIX_ERR_REVOKED) && !defined(MPI_ERR_REVOKED)
#define MPI_ERR_REVOKED MPIX_ERR_REVOKED
#endif

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

// Return codes from Fenix functions. Errors are negative; warnings are positive.
typedef enum {
  FENIX_SUCCESS = 0,

  // Error values are negative
  FENIX_ERROR_UNINITIALIZED = -100,
  FENIX_ERROR_NOCATEGORY,
  FENIX_ERROR_CALLBACK_NOT_REGISTERED,
  FENIX_ERROR_GROUP_CREATE,
  FENIX_ERROR_MEMBER_CREATE,
  FENIX_ERROR_MEMBER_EXISTS,
  FENIX_ERROR_MEMBER_STAGING,
  FENIX_ERROR_MEMBER_LOADING,
  FENIX_ERROR_MEMBER_UNSTORED,
  FENIX_ERROR_COMMIT_BARRIER,
  FENIX_ERROR_INVALID_GROUPID,
  FENIX_ERROR_INVALID_MEMBERID,
  FENIX_ERROR_INVALID_LOGIC_CALL,
  FENIX_ERROR_INVALID_POLICY_NAME,
  FENIX_ERROR_INVALID_POLICY_VALUE,
  FENIX_ERROR_INVALID_TIMESTAMP,
  FENIX_ERROR_INVALID_TIMESTART,
  FENIX_ERROR_INVALID_DEPTH,
  FENIX_ERROR_INVALID_ATTRIBUTE_NAME,
  FENIX_ERROR_INVALID_ATTRIBUTE_VALUE,
  FENIX_ERROR_INVALID_POSITION,
  FENIX_ERROR_INVALID_SUBSET,
  FENIX_ERROR_DATA_WAIT,
  FENIX_ERROR_SUBSET_NUM_BLOCKS,
  FENIX_ERROR_SUBSET_START_OFFSET,
  FENIX_ERROR_SUBSET_END_OFFSET,
  FENIX_ERROR_SUBSET_STRIDE,
  FENIX_ERROR_NODATA_FOUND,
  FENIX_ERROR_INTERN,
  FENIX_ERROR_CANCELLED,
  FENIX_ERROR_INVALID_SETTING_NAME,
  FENIX_ERROR_INVALID_SETTING_OPTION,
  FENIX_ERROR_INVALID_MLOGID,
  FENIX_ERROR_MLOG_EXISTS,
  FENIX_ERROR_MLOG_LIBRARY_UNAVAILABLE,
  FENIX_ERROR_PROCESS_FAILURE,

  //Warnings are positive
  FENIX_WARNING_SPARE_RANKS_DEPLETED = 100,
  FENIX_WARNING_PARTIAL_RESTORE,
} Fenix_Return_codes;

//Internal location codes.
#define FENIX_ERRHANDLER_LOC 1
#define FENIX_FINALIZE_LOC 2
#define FENIX_DATA_COMMIT_BARRIER_LOC 4

typedef enum {
  FENIX_ROLE_INITIAL_RANK   = 0,
  FENIX_ROLE_RECOVERED_RANK = 1,
  FENIX_ROLE_SURVIVOR_RANK  = 2,
  FENIX_ROLE_SPARE_RANK     = 3,
  FENIX_ROLE_MISSING_RANK   = 4,
} Fenix_Rank_role;

typedef enum {
  FENIX_RECOVERY_MODE,
  FENIX_RESUME_MODE,
  FENIX_UNHANDLED_MODE,
  FENIX_CALLBACK_EXCEPTION_MODE,
  FENIX_MLOG_RECOVERY_MODE,
  FENIX_SPARE_WAIT_MODE,
  FENIX_SPARE_FINALIZE_MODE,
  FENIX_SETTING_NAME_MAXCODE // Internal.
} Fenix_Setting_name;

typedef enum {
  FENIX_RECOVERY_IGNORE,
  FENIX_RECOVERY_NOOP,
  FENIX_RECOVERY_REPAIR,
  FENIX_RECOVERY_SPAWN,
  FENIX_RECOVERY_MODE_MAXCODE // Internal.
} Fenix_Recovery_mode;

typedef enum {
  FENIX_MLOG_RECOVERY_MANUAL,
  FENIX_MLOG_RECOVERY_INLINE,
  FENIX_MLOG_RECOVERY_INLINE_AUTOSYNC,
  FENIX_MLOG_RECOVERY_MODE_MAXCODE // Internal.
} Fenix_Mlog_recovery_mode;

typedef enum {
  FENIX_RESUME_JUMP,
  FENIX_RESUME_RETURN,
  FENIX_RESUME_THROW,
  FENIX_RESUME_MODE_MAXCODE // Internal.
} Fenix_Resume_mode;

typedef enum {
  FENIX_UNHANDLED_SILENT,
  FENIX_UNHANDLED_PRINT,
  FENIX_UNHANDLED_ABORT,
  FENIX_UNHANDLED_MODE_MAXCODE // Internal.
} Fenix_Unhandled_mode;

typedef enum {
  FENIX_SPARE_WAIT_BUSY,
  FENIX_SPARE_WAIT_YIELD,
  FENIX_SPARE_WAIT_SLEEP,
  FENIX_SPARE_WAIT_MODE_MAXCODE // Internal.
} Fenix_Spare_wait_mode;

typedef enum {
  FENIX_CALLBACK_EXCEPTION_RETHROW,
  FENIX_CALLBACK_EXCEPTION_SQUASH,
  FENIX_CALLBACK_EXCEPTION_MODE_MAXCODE // Internal.
} Fenix_Callback_exception_mode;

typedef enum {
  FENIX_SPARE_FINALIZE_RELEASE,
  FENIX_SPARE_FINALIZE_EXIT,
  FENIX_SPARE_FINALIZE_MODE_MAXCODE // Internal.
} Fenix_Spare_finalize_mode;

// Build a resilient communicator and set the restart longjmp point.
#define Fenix_Init(_role, _comm, _newcomm, _argc, _argv, _spare_ranks, _err)   \
  {                                                                            \
    static jmp_buf bufjmp;                                                     \
    *(_role) = __fenix_preinit(                                                \
      _role, _comm, _newcomm, _argc, _argv, _spare_ranks, _err, &bufjmp        \
    );                                                                         \
    setjmp(bufjmp);                                                            \
    __fenix_postinit();                                                        \
  }

int Fenix_Finalize();

int Fenix_Initialized(int* flag);
int Fenix_Finalized(int* flag);

int Fenix_set_option(Fenix_Setting_name setting, unsigned option);
int Fenix_get_option(Fenix_Setting_name setting, unsigned* option);

Fenix_Rank_role Fenix_get_role();
int Fenix_get_error();
int Fenix_get_nspare();
int Fenix_Process_fail_list(int** fail_list);

// These two are currently unimplemented functions.
int Fenix_get_rank_role(MPI_Comm comm, int rank, int* role);
int Fenix_get_number_of_ranks_with_role(int role, int* n_ranks);

// Register a callback to run after recovery.
int Fenix_Callback_register(
  void (*recover)(MPI_Comm, int, void*), void* callback_data
);
int Fenix_Callback_pop();
int Fenix_Callback_invoke_all();

// Check for locally-detected rank failures
int Fenix_Process_detect_failures(int do_recovery);

// Check whether a pre-recovery request completed successfully.
int Fenix_check_cancelled(MPI_Request* request, MPI_Status* status);



// Data-recovery API.
#define FENIX_DATA_GROUP_WORLD_ID 10
#define FENIX_GROUP_ID_MAX 11
#define FENIX_DATA_MEMBER_ALL -1
#define FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER 11
#define FENIX_DATA_MEMBER_ATTRIBUTE_COUNT 12
#define FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE 13
#define FENIX_DATA_MEMBER_ATTRIBUTE_SIZE 14
#define FENIX_DATA_SNAPSHOT_LATEST -1
#define FENIX_DATA_SNAPSHOT_ALL -2
#define FENIX_DATA_RESTORE_INPLACE NULL
#define FENIX_DATA_RESTORE_FULL INT_MAX
#define FENIX_RESIZEABLE 0
#define FENIX_DATA_SUBSET_CREATED 2
#define FENIX_STOREV_ALL -1
#define FENIX_SERIALIZE -2
#define FENIX_DESERIALIZE -3

#define FENIX_DATA_POLICY_IN_MEMORY_RAID 13
#define FENIX_DATA_POLICY_IMR FENIX_DATA_POLICY_IN_MEMORY_RAID
#define FENIX_DATA_POLICY_LOCAL 14

#define FENIX_TIME_STAMP_IGNORE NULL

// Unused, likely to be changed before usage.
typedef struct {
  MPI_Request mpi_send_req;
  MPI_Request mpi_recv_req;
} Fenix_Request;

typedef struct {
  void* impl;
} Fenix_Data_subset;

extern const Fenix_Data_subset FENIX_DATA_SUBSET_FULL;
extern const Fenix_Data_subset FENIX_DATA_SUBSET_EMPTY;
extern const Fenix_Data_subset FENIX_DATA_SUBSET_PRESTAGED;
extern Fenix_Data_subset* FENIX_DATA_SUBSET_IGNORE;

// file, direction (FENIX_(DE)SERIALIZE), data buffer, offset, count, context
typedef void (*Fenix_Serialize_file_fn)(FILE*, int, void*, int, int, void*);


int Fenix_Data_group_create(
  int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name,
  void* policy_value, int* flag
);

int Fenix_Data_group_created(int group_id);

int Fenix_Data_group_get_redundancy_policy(
  int group_id, int* policy_name, void* policy_value, int* flag
);


int Fenix_Data_member_create(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);
int Fenix_Data_member_fcreate(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  Fenix_Serialize_file_fn serializer, void* ctx
);
int Fenix_Data_member_define(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);
int Fenix_Data_member_fdefine(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  Fenix_Serialize_file_fn serializer, void* ctx
);

int Fenix_Data_member_created(int group_id, int member_id);

// Unimplemented: Block on completion of the store operation specified by the
//! request.
int Fenix_Data_wait(Fenix_Request request);

// Unimplemented: Query completion of the store operation specified by the
//! request.
int Fenix_Data_test(Fenix_Request request, int* flag);

int Fenix_Data_member_stage(
  int group_id, int member_id, const Fenix_Data_subset subset
);
int Fenix_Data_member_stage_inplace(
  int group_id, int member_id, void* buf, const Fenix_Data_subset subset
);
int Fenix_Data_member_stage_begin(int group_id, int member_id, FILE** fpp);
int Fenix_Data_member_stage_end(int group_id, int member_id);

int Fenix_Data_member_store(
  int group_id, int member_id, const Fenix_Data_subset subset
);
int Fenix_Data_member_storev(
  int group_id, int member_id, const Fenix_Data_subset subset
);

// Unimplemented.
int Fenix_Data_member_istore(
  int group_id, int member_id, const Fenix_Data_subset subset,
  Fenix_Request* request
);
// Unimplemented.
int Fenix_Data_member_istorev(
  int group_id, int member_id, const Fenix_Data_subset subset,
  Fenix_Request* request
);

int Fenix_Data_commit(int group_id, int* time_stamp);
int Fenix_Data_commit_barrier(int group_id, int* time_stamp);

// Store all members then commit the group. Supports inline recovery.
int Fenix_Data_checkpoint(
  int group_id, const Fenix_Data_subset subset, int num_storev, int* storev_ids,
  int* time_stamp
);

// Repair the resilient storage for a committed member.
// Behavior is undefined if the group's communicator size changed after commits.
int Fenix_Data_member_repair(int group_id, int member_id);

// Load committed member data into the member's buffer.
// The caller owns found_data unless FENIX_DATA_SUBSET_IGNORE is used.
int Fenix_Data_member_load(
  int group_id, int member_id, int time_stamp, Fenix_Data_subset* found_data
);

// Load committed member data into a custom target buffer.
int Fenix_Data_member_load_to(
  int group_id, int member_id, void* target, int target_count, int time_stamp,
  Fenix_Data_subset* found_data
);

// Open a file for manual loading of committed member data.
// Do not use other staging/store/load/restore calls for this member until load_end.
int Fenix_data_member_load_begin(
  int group_id, int member_id, FILE** fpp, int time_stamp,
  Fenix_Data_subset* found_data
);

// Finish manual loading opened by load_begin.
int Fenix_Data_member_load_end(int group_id, int member_id);

// Repair a member's storage and load its data.
// May be matched remotely by Fenix_Data_member_repair.
int Fenix_Data_member_restore(
  int group_id, int member_id, void* target_buffer, int max_count,
  int time_stamp, Fenix_Data_subset* found_data
);

// Restore a member from a local snapshot only.
// Deprecated: prefer the member_load functions.
int Fenix_Data_member_lrestore(
  int group_id, int member_id, void* target_buffer, int max_count,
  int time_stamp, Fenix_Data_subset* found_data
);

// Unimplemented: restore a member from a specific rank's data.
int Fenix_Data_member_restore_from_rank(
  int group_id, int member_id, void* data, int max_count, int time_stamp,
  Fenix_Data_subset* found_data, int source_rank
);

// Create a regularly spaced data subset.
// Delete created subsets with Fenix_Data_subset_delete when no longer needed.
int Fenix_Data_subset_create(
  int num_blocks, int start_offset, int end_offset, int stride,
  Fenix_Data_subset* subset
);

// Create an irregularly spaced data subset.
// Delete created subsets with Fenix_Data_subset_delete when no longer needed.
int Fenix_Data_subset_createv(
  int num_blocks, int* array_start_offsets, int* array_end_offsets,
  Fenix_Data_subset* subset
);

// Delete a data subset.
int Fenix_Data_subset_delete(Fenix_Data_subset* subset);

// Get the number of members in a data group.
int Fenix_Data_group_get_number_of_members(
  int group_id, int* number_of_members
);

// Get the member ID at a given group position.
int Fenix_Data_group_get_member_at_position(
  int group_id, int* member_id, int position
);

// Get the number of locally available snapshots in a data group.
// Snapshot availability may be inconsistent across ranks after failures.
int Fenix_Data_group_get_number_of_snapshots(
  int group_id, int* number_of_snapshots
);

// Get a snapshot time stamp by position.
// Position 0 is the most recent locally available snapshot.
int Fenix_Data_group_get_snapshot_at_position(
  int group_id, int position, int* time_stamp
);

// Get the cohort (redundancy partner group) for a data group.
// The returned MPI_Group is duplicated and must be freed with MPI_Group_free.
int Fenix_Data_group_get_cohort(int group_id, MPI_Group* cohort);

// Unimplemented: get the value of a member attribute.
int Fenix_Data_member_attr_get(
  int group_id, int member_id, int attributename, void* attributevalue,
  int* flag
);

// Set the value of a member attribute.
// COUNT and DATATYPE cannot be changed after the first store, even after recovery.
int Fenix_Data_member_attr_set(
  int group_id, int member_id, int attribute_name, void* attribute_value,
  int* flag
);

// Delete a snapshot from a data group.
int Fenix_Data_snapshot_delete(int group_id, int time_stamp);

// Delete a data group.
int Fenix_Data_group_delete(int group_id);

// Delete a data member.
int Fenix_Data_member_delete(int group_id, int member_id);


// Message-logging API.
#define FENIX_MLOG_NONE -1
#define FENIX_MLOG_CONTINUE -1

int Fenix_Mlog_create(int mlog_id, MPI_Comm* comm, int depth);

// Messages can only be logged by the active log. Deactivates any prior logs.
// Activate FENIX_MLOG_NONE to deactivate any currently active log.
int Fenix_Mlog_activate(int mlog_id);

// Query for currently active message log (possibly FENIX_MLOG_NONE)
int Fenix_Mlog_active(int* mlog_id);

// region_id must advance unless no messages were logged in the current region.
int Fenix_Mlog_begin_region(int mlog_id, int region_id);

// Activate a message log and begin a region.
int Fenix_Mlog_activate_region(int mlog_id, int region_id);

// Synchronize message logs across ranks.
// FENIX_MLOG_CONTINUE resumes from this rank's latest logged state.
int Fenix_Mlog_sync(int mlog_id, int region_id);

// Create a data member linked to a message log.
int Fenix_Mlog_create_data_member(int mlog_id, int group_id, int member_id);

// Define a data member linked to a message log.
int Fenix_Mlog_define_data_member(int mlog_id, int group_id, int member_id);

// Delete a message log.
int Fenix_Mlog_delete(int mlog_id);


#if defined(c_plusplus) || defined(__cplusplus)
} // extern "C"

#include "fenix.hpp"
#endif

#endif // __FENIX__
