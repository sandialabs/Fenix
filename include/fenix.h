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
#include "fenix_exception.hpp"

extern "C" {
#endif
#include "fenix_data_subset.h"
#include "fenix_process_recovery.h"

/**
 * @file
 * @brief Contains all API function calls and Fenix types.
 * This is the only header file a user should include.
 */

/**
 * @defgroup ReturnCodes Return Codes
 * @brief All possible return codes from Fenix functions.
 * Errors are negative, warnings are positive.
 * @{
 */
#define FENIX_SUCCESS                         0
#define FENIX_ERROR_UNINITIALIZED            -9
#define FENIX_ERROR_NOCATEGORY              -10
#define FENIX_ERROR_CALLBACK_NOT_REGISTERED -11
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
/**@}*/

//!@internal @brief Agreement code for error handler
#define FENIX_ERRHANDLER_LOC		      1
//!@internal @brief Agreement code for finalize
#define FENIX_FINALIZE_LOC		      2
//!@internal @brief Agreement code for data commit barrier
#define FENIX_DATA_COMMIT_BARRIER_LOC	      4



/**
 * @defgroup ProcessRecovery Process Recovery
 * @brief Functions for managing process recovery in Fenix.
 * @details @include{doc} ProcessRecovery.md
 * @{
 */

/**
 * @brief All possible roles returned by Fenix_Init
 * 
 * Describes the current process's state in reference
 * to process recovery.
 *
 * It is important to note that FENIX_ROLE_RECOVERED_RANK
 * is only guaranteed to be the value after a single failure,
 * so users ought not use the role to directly ensure a valid
 * state if they desire to be resilient to failures during their
 * failure recovery process.
 */
typedef enum {
    //!No failures have occurred yet
    FENIX_ROLE_INITIAL_RANK = 0,
    //!This rank was a spare before the most recent failure, or was just spawned
    FENIX_ROLE_RECOVERED_RANK = 1,
    //!This rank was not a spare before the most recent failure
    FENIX_ROLE_SURVIVOR_RANK = 2
} Fenix_Rank_role;

/**
 * @fn void Fenix_Init(int* role, MPI_Comm comm, MPI_Comm* newcomm, int** argc, char*** argv, int spare_ranks, int spawn, MPI_Info info, int* error);
 * @brief Build a resilient communicator and set the restart point.
 *
 * This function must be called by all ranks in \c comm, after MPI initialization. All calling ranks must
 * pass the same values for the parameters \c comm, \c spare_ranks, \c spawn, and \c info. \c Fenix_init
 * must be called exactly once by each rank. This function is used (1) to activate the Fenix library, (2)
 * to specify extra resources in case of rank failure, and (3) to create a logical resumption point in case
 * of rank failure.
 *
 * For C, the program may rely on the the state of any variables defined and set before the call to \c Fenix_Init.
 * But note that the code executed before \c Fenix_Init is executed by all ranks in the system (including spare 
 * ranks, see below). For C++, the state of objects declared before \c Fenix_Init but within the same scope as
 * \c Fenix_Init is compiler-dependant, and it is recommended to place \c Fenix_Init within a subscope exluding
 * any variables expected to no be destructed.
 *
 * It is recommended to access argc and argv only after executin \c Fenix_Init, since command line arguments
 * passed to this function that apply to Fenix may be removed by \c Fenix_Init.
 *
 * \c Fenix_Init is blocking in the following sense. If it is entered for the first time via a regular, explicit
 * function call, it must be entered by all ranks in communicator \c comm. If it is entered after an error 
 * intercepted by Fenix (it if the default execution resumption point, see _info below), no ranks are allowed 
 * to exit from it until all *non-failed* ranks have returned control to it. **Note**: Typically control is  
 * returned automatically through revocation of the resilient communicator, which means ranks which have long 
 * delays between MPI function calls or ranks which only use communicators unaffected by failure may lead to
 * long delays between a failure and its recovery.
 *
 * Ranks to be used as spare ranks by Fenix will be available to the application only before \c Fenix_Init,
 * or after they are used to replace a failed rank, in which case they turn into active ranks. This document
 * refers to the latter as \c RECOVERED ranks (see #Fenix_Rank_role). Note that all spare
 * ranks that have not been used to recover from failures (and, therefore, are still reserved by Fenix and kept 
 * inside \c Fenix_Init) will automatically call \c MPI_Finalize and exit when all active ranks have entered the 
 * #Fenix_Finalize call.
 *
 * No Fenix functions may be called before \c Fenix_Init, except #Fenix_Initialized.
 *
 * @param[out] role The current role of this rank (see #Fenix_Rank_role)
 * @param[in] comm The base communicator to construct a resilient communicator from,
 *   which must include any spare ranks (see below) the user deems necessary.
 *   MPI_COMM_WORLDis a valid value, but MPI_COMM_SELF is not.
 * @param[out] newcomm Resilient output communicator, managed by Fenix and derived 
 *   from comm, to be used by the application instead of comm.
 * @param[inout] argc Pointer to application main's argc parameter
 * @param[inout] argv Pointer to application main's argv parameter
 * @param[in] spare_ranks The number of ranks in comm that are exempted by Fenix
 *   in the construction of the resilient communicator by Fenix_Init. These ranks
 *   are kept in reserve to substitute for failed ranks. Failed ranks in resilient
 *   communicators are replaced by spare or spawned ranks.
 * @param[in] spawn *Unimplemented*: Whether to enable spawning new ranks to replace
 *   failed ranks when spares are unavailable.
 * @param[in] info Fenix recovery configuration parameters, may be MPI_INFO_NULL
 *   Supports the "FENIX_RESUME_MODE" key, used to indicate where execution should resume upon 
 *   rank failure for all active (non-spare) ranks in any resilient communicators, not only for 
 *   those ranks in communicators that failed. The following values associated with the 
 *   "resume_mode" key are supported:
 *   - "Fenix_init" (default): execution resumes at logical exit of Fenix_Init.
 *   - "NO_JUMP":    execution continues from the failing MPI call. Errors are otherwise handled
 *   as normal, but return the error code as well. Applications should typically
 *   either check for return codes or assign an error callback through Fenix.
 * @param[out] error The return status of \c Fenix_Init<br>
 *   Used to signal that a non-fatal error or special condition was encountered in the execution of
 *   Fenix_Init, or FENIX_SUCCESS otherwise. It has the same value across all ranks released by 
 *   Fenix_Init. If spawning is explicitly disabled (_spawn equals false) and spare ranks have been 
 *   depleted, Fenix will repair resilience communicators by shrinking them and will report such 
 *   shrinkage in this return parameter through the value FENIX_WARNING_SPARE_RANKS_DEPLETED.
 */

//!@internal
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


/**
 * @brief Sets flag to true if Fenix_Init has been called, else false.
 * @param[out] flag Pointer to the flag to be set.
 * @returnstatus
 */
int Fenix_Initialized(int *flag);

/**
 * @brief Register a callback to be invoked after failure process recovery.
 *
 * This function registers a callback to be invoked after a failure has been recovered by Fenix, 
 * and right before resuming application execution (e.g. returning from #Fenix_Init by default).
 * If this function is called more than once, the different callbacks will be called in the 
 * reverse order that they were registered (i.e. as a callback stack).
 *
 * Callback functions are passed the newly-repaired resilient communicator, the error code returned
 * by MPI in the communication action which caused a failure recovery, and the user-provided \c void*
 * callback data.
 *
 * Callbacks will only be invoked by survivor ranks, since spare ranks or respawned ranks had no way
 * to register them before a failure. 
 *
 * @param[in] recover the callback function to register.
 * @param[in] callback_data The user-provided data which will be passed to the callback.
 *
 * @returnstatus
 */
int Fenix_Callback_register(void (*recover)(MPI_Comm, int, void *),
                            void *callback_data);

/**
 * @brief Pop the most recently registered callback from the callback stack.
 * @returnstatus
 */
int Fenix_Callback_pop();

/**
 * @brief Check for any failed ranks
 *
 * @param[in] do_recovery If true, Fenix will attempt to recover from any detected failures.
 *   Else, it will ignore any failures and simply return the MPI return code.
 * @return MPI_SUCCESS if no failures were detected, else the MPI return code.
 */
int Fenix_Process_detect_failures(int do_recovery);

//!@unimplemented Returns the number of ranks with a given #Fenix_Rank_role
int Fenix_get_number_of_ranks_with_role(int, int *);

//!@unimplemented Returns the #Fenix_Rank_role for a given rank
int Fenix_get_role(MPI_Comm comm, int rank, int *role);

/**
 * @brief Get the list of ranks that failed in the most recent failure.
 * @param[out] fail_list Set to a list of failed ranks.
 * @return The number of failed ranks.
 */
int Fenix_Process_fail_list(int** fail_list);

/**
 * @brief Check a pre-recovery request without error
 * @param[in] request The request to check
 * @param[out] status The status of the request
 * @return True if the request was cancelled or has unknown completion status,
 *         false if it completed successfully.
 */
int Fenix_check_cancelled(MPI_Request *request, MPI_Status *status);


/**
 * @brief Clean up Fenix state. Each active rank must call \c Fenix_Finalize before exiting.
 * 
 * This function cleans up all Fenix state, if any. If an MPI program using the Fenix library terminates
 * normally (i.e. not due to a call to \c MPI_Abort, or an unrecoverable error) then each rank must call
 * \c Fenix_Finalize before it exits. It must be called before \c MPI_Finalize, and after #Fenix_Init.
 * There shall be no function calls after this function, except #Fenix_Initialized.
 *
 * As noted in the description of #Fenix_Init, all spare ranks that have not been used to
 * recover from failures (and therefore are still reserved by Fenix and kept inside #Fenix_Init) will call 
 * \c MPI_Finalize and exit when all active ranks have called \c Fenix_Finalize.
 *
 * **Advice**: Sometimes users may want to remove ranks proactively from the execution, for example because
 * monitoring data shows that failure of a rank is imminent or that a rank is executing un-manageably slowly.
 * This can be accomplished by calling \c exit on the targeted ranks, followed by an invocation of MPI_Barrier.
 * The removed ranks will be reported as failed and error handling will progress appropriately. No calls to finalize
 * are needed in this case.
 */
int Fenix_Finalize();

/**@}*/


/**
 * @defgroup DataRecovery Data Recovery
 * @brief Functions for storing and restoring data in Fenix.
 * @details @include{doc} DataRecovery.md
 *
 * @{
 */
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

#define FENIX_DATA_POLICY_IN_MEMORY_RAID     13

/**
 * @unimplemented As MPI_Request, but for Fenix asynchronous data recovery calls
 */
typedef struct {
    MPI_Request mpi_send_req;
    MPI_Request mpi_recv_req;
} Fenix_Request;

//!@brief A standin for checkpointing/recovering all available data in a member.
extern const Fenix_Data_subset  FENIX_DATA_SUBSET_FULL;

//!@brief A standin for checkpointing/recovering none of the available data in a member.
extern const Fenix_Data_subset  FENIX_DATA_SUBSET_EMPTY;


/**
 * @brief Create a Data Group
 * @qualifier collective
 *
 * If a group with this group_id was already created in the past and has not been deleted, the 
 * parameters of this call are ignored and this function simply serves to coordinate with any 
 * ranks that have not yet created this group (e.g. due to a failure).
 *
 * All calling ranks must pass the same values for the parameters \c group_id, \c comm,
 * \c start_time_stamp, \c policy_name, and \c policy_value.
 *
 * @param group_id A unique identifier to this group.
 * @param comm A resilient communicator on which the group is formed.
 * @param start_time_stamp The time_stamp to be used for the first commit in this group.
 * @param depth
 * @parblock
 * The number of successive snapshots of this group that are retained by Fenix, in 
 * addition to the most recent one, and that can be recovered by calling Fenix data member
 * restore functions.
 * 
 * For example, a depth of 0 means Fenix will keep only the necessary data to restore the
 * most recent snapshot, freeing or overwriting older snapshots automatically. A depth
 * of -1 is currently not supported, but would ordinarily indicate that no snapshots should
 * be removed automatically.
 * @endparblock
 * @param policy_name Currently, may only be FENIX_DATA_POLICY_IN_MEMORY_RAID
 * @param policy_value Pointer to data passed along to the policy. 
 *   See the specific policy for more information.
 * @param flag pointer to store policy-specific status or errors
 * @return FENIX_SUCCESS, or an error value.
 */
int Fenix_Data_group_create(int group_id, MPI_Comm comm, int start_time_stamp,
                            int depth, int policy_name, void* policy_value,
                            int* flag);

/**
 * @brief Create a data member for store/restore operations
 * @qualifier collective 
 * @qualifier local
 *
 * All calling ranks in the group's communicator must pass the same values for the parameters
 * \c member_id, \c datatype, and \c group_id.
 *
 * @param group_id Identifier to a data group within which to create the member.
 * @param member_id An integer unique within the data group that identifies the data in 
 *        \c source_buffer. Must be nonnegative and less than FENIX_MEMBER_ID_MAX, which is 
 *        guaranteed to be at least 2^30.
 * @param buffer Address of the data to be copied to redundant storage maintained by Fenix.
 *        Note that this parameter may also be specified using #Fenix_Data_member_attr_set, which
 *        is critical for non-survivor ranks after a failure which will have an invalid address
 *        which was generated on the failed rank and must update.
 * @param count The maximum number of contiguous elements of type \c datatype of the data to be
 *        stored. Need not be the same in all calling ranks.
 * @param datatype The MPI_Datatype of the elements in \c source_buffer
 *
 * @return FENIX_SUCCESS, or an error value.
 */
int Fenix_Data_member_create(int group_id, int member_id, void *buffer,
                             int count, MPI_Datatype datatype);

/**
 * @brief Get the storage policy of a data group
 * 
 * @param group_id Identified to the data group to query
 * @param policy_name The identifier of the policy name of the data group.
 * @param policy_value A location within which to store the policy_values this group's
 *        policy was configured with.
 * @param flag A location set to true if a policy value was extracted, else false.
 * @return FENIX_SUCCESS, or an error value.
 */
int Fenix_Data_group_get_redundancy_policy(int group_id, int* policy_name,
                                           void *policy_value, int *flag);

//!@unimplemented Block on completion of the store operation specified by the request.
int Fenix_Data_wait(Fenix_Request request);


//!@unimplemented Query completion of the store operation specified by the request.
int Fenix_Data_test(Fenix_Request request, int *flag);


/**
 * @brief Store a particular group member into the group's resilient storage space, in uncommitted storage.
 * @qualifier collective
 *
 * The user can safely modify the member's data buffer after this call, as the current state is copied immediately.
 * Multiple calls may be used to incrementally store data (using subset_specifiers), or overwrite old data prior to a commit.
 *
 * @param group_id All ranks must provide the same group_id
 * @param member_id All ranks must provide the same member_id
 * @param subset_specifier Which subset of the data to store. It is always valid for every rank to provide the same 
 *        subset_specifier; depending on the group's policy, varying combinations of specifiers may be possible.
 * @return FENIX_SUCCESS, or an error value.
 */
int Fenix_Data_member_store(int group_id, int member_id,
                            Fenix_Data_subset subset_specifier);


//!@unimplemented As [store](#Fenix_Data_member_store), but subsets may vary rank-to-rank.
int Fenix_Data_member_storev(int group_id, int member_id,
                             Fenix_Data_subset subset_specifier);

//!@unimplemented As [store](#Fenix_Data_member_store), but asynchronous.
int Fenix_Data_member_istore(int group_id, int member_id,
                             Fenix_Data_subset subset_specifier,
                             Fenix_Request *request);

//!@unimplemented As [istore](#Fenix_Data_member_istore), but asynchronous.
int Fenix_Data_member_istorev(int group_id, int member_id,
                              Fenix_Data_subset subset_specifier,
                              Fenix_Request *request);

/**
 * @brief Commit stored data members to the group's next snapshot.
 * @qualifier collective
 * @qualifier local
 *
 * This function is used to freeze the current state of a data group,
 * together with all its application data that has been stored in Fenix’
 * redundant storage, and label it with a time stamp, thus creating a 
 * snapshot of the stored application data. Only data that has been 
 * committed is eligible for recovery through #Fenix_Data_member_restore. 
 * An application needs to call #Fenix_Data_wait for all pending asynchronous 
 * [Fenix_Data_member_istore(v)](@ref Fenix_Data_member_istore) operations 
 * in the group before committing.
 *
 * @param[in] group_id The group to commit
 * @param[out] time_stamp The time stamp of the new snapshot
 * @returnstatus
 */
int Fenix_Data_commit(int group_id, int *time_stamp);

/**
 * @brief As [commit](#Fenix_Data_commit), but ensures a globally consistent commit.
 * @qualifier collective
 *
 * This function does not function as a traditional barrier.
 * The commit will proceed if all *non-failed* ranks reach the barrier.
 * This allows for commits to be made when a rank fails after storing all
 * of its data into resilient storage.
 *
 * @param[in] group_id The group to commit
 * @param[out] time_stamp The time stamp of the new snapshot
 * @returnstatus
 */
int Fenix_Data_commit_barrier(int group_id, int *time_stamp);

//!@unimplemented Block until all ranks in the group have reached this point.
int Fenix_Data_barrier(int group_id);

/**
 * @brief Restore the data of a group member from a snapshot.
 * @qualifier collective
 *
 * All ranks in the group’s resilient communicator must pass the 
 * same values for the parameters group_id, member_id, and time_stamp.
 * This function is used to retrieve data from consistent snapshot
 * members. This function can only be used if the size of the 
 * communicator used to store the data is the same as that at the time
 * of data recovery (this implies non-shrinking communicator recovery
 * in case of a rank loss).
 *
 * If the size of the buffer needing to receive the recovery data is
 * unknown for a particular rank, it can be queried using 
 * #Fenix_Data_member_attr_get.
 *
 * @param[in] group_id The group to restore from
 * @param[in] member_id The member to restore
 * @param[out] target_buffer The buffer to store the restored data
 * @param[in] max_count The maximum number of elements to restore
 * @param[in] time_stamp The time stamp of the snapshot to restore from
 * @param[out] found_data The subset of the data that was found in the snapshot
 * @returnstatus
 */
int Fenix_Data_member_restore(int group_id, int member_id, void *target_buffer,
                              int max_count, int time_stamp, Fenix_Data_subset* found_data);

/**
 * @brief Local-only version of Fenix_Data_member_restore
 *
 * This function restores the data of a group member from the local
 * snapshot.
 *
 * @param[in] group_id The group to restore from
 * @param[in] member_id The member to restore
 * @param[out] target_buffer The buffer to store the restored data
 * @param[in] max_count The maximum number of elements to restore
 * @param[in] time_stamp The time stamp of the snapshot to restore from
 * @param[out] found_data The subset of the data that was found in the snapshot
 * @returnstatus
 */
int Fenix_Data_member_lrestore(int group_id, int member_id, void *target_buffer,
                              int max_count, int time_stamp, Fenix_Data_subset* found_data);

//!@unimplemented As #Fenix_Data_member_restore, but restores from a specific rank's data.
int Fenix_Data_member_restore_from_rank(int member_id, void *data, int max_count,
                                        int time_stamp, int group_id,
                                        int source_rank);

/**
 * @brief Create a data subset for use in store operations.
 *
 * Creates a subset based on num_blocks pairs of 
 * {start_offset,end_offset}, 
 * {start_offset+stride,end_offset+stride}, 
 * {start_offset+2*stride,end_offset+2*stride},
 * etc. 
 *
 * The value of start_offset must be smaller than or equal
 * to the value of end_offset to indicate non-negative block
 * size. Otherwise, the function returns an error code.
 *
 * Created subsets must be deleted with #Fenix_Data_subset_delete
 * to free memory.
 *
 * @param[in] num_blocks The number of contiguous data blocks.
 * @param[in] start_offset The index of the first element in the first data block.
 * @param[in] end_offset The index of the last element in the first data block.
 * @param[in] stride Regular shift between successive data blocks.
 * @param[out] subset_specifier The created subset.
 * @returnstatus
 */
int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset,
                             int stride, Fenix_Data_subset *subset_specifier);

/**
 * @brief As #Fenix_Data_subset_create, but with varying start and end offsets.
 *
 * Creates a subset based on num_blocks pairs of {start_offset,end_offset}.
 * The value of start_offset must be smaller than or equal to end_offset
 * to indicate non-negative block size. Otherwise, the function returns an
 * error code.
 *
 * Created subsets must be deleted with #Fenix_Data_subset_delete
 * to free memory.
 *
 * @param[in] num_blocks The number of contiguous data blocks.
 * @param[in] array_start_offsets The index of the first element in each data block.
 * @param[in] array_end_offsets The index of the last element in each data block.
 * @param[out] subset_specifier The created subset.
 */
int Fenix_Data_subset_createv(int num_blocks, int *array_start_offsets,
                              int *array_end_offsets,
                              Fenix_Data_subset *subset_specifier);

/**
 * @brief Delete a data subset.
 *
 * Frees the memory associated with a data subset object.
 *
 * @param[in] subset_specifier The subset to delete.
 * @returnstatus
 */
int Fenix_Data_subset_delete(Fenix_Data_subset *subset_specifier);

//!@unimplemented Get the number of members in a data group.
int Fenix_Data_group_get_number_of_members(int group_id, int *number_of_members);

//!@unimplemented Get member ID based on member index
int Fenix_Data_group_get_member_at_position(int group_id, int *member_id,
                                            int position);

/**
 * @brief Get the number of locally-available snapshots in a data group.
 *
 * May include snapshots that are inconsistent across the group.
 *
 * @param[in] group_id The group to query
 * @param[out] number_of_snapshots The number of snapshots in the group
 * @returnstatus
 */
int Fenix_Data_group_get_number_of_snapshots(int group_id,
                                             int *number_of_snapshots);

/**
 * @brief Get the time stamp of a snapshot at a given index.
 *
 * Snapshots are indexed in reverse order in which the user committed them
 * (e.g. the most recent available snapshot has position=0).
 *
 * @param[in] group_id The group to query
 * @param[in] position The index of the snapshot, which must be [0, number_of_snapshots)
 * @param[out] time_stamp The time stamp of the snapshot
 *
 */
int Fenix_Data_group_get_snapshot_at_position(int group_id, int position,
                                              int *time_stamp);

//!@unimplemented Get the value of a member's attribute.
int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename,
                               void *attributevalue, int *flag, int source_rank);

/**
 * @brief Set the value of a member's attribute.
 *
 * Valid names are #FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, #FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
 * and #FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE.
 *
 * The COUNT and DATATYPE attributes may only be set before the first store operation.
 * Contrary to the Fenix specification, returning to #Fenix_Init after a failure does not 
 * allow the user to set these attributes again.
 *
 * @param[in] group_id The group to update
 * @param[in] member_id The member to update
 * @param[in] attribute_name The attribute to update
 * @param[in] attribute_value The new value of the attribute
 * @param[out] flag Set to true if the attribute was set, else false
 * @returnstatus
 */
int Fenix_Data_member_attr_set(int group_id, int member_id, int attribute_name,
                               void *attribute_value, int *flag);

/**
 * @brief Delete a snapshot from a data group.
 * @qualifier local
 * 
 * @param[in] group_id The group to delete from
 * @param[in] time_stamp The time stamp of the snapshot to delete
 * @returnstatus
 */
int Fenix_Data_snapshot_delete(int group_id, int time_stamp);

/**
 * @brief Delete a data group.
 * @qualifier local
 *
 * @param[in] group_id The group to delete
 * @returnstatus
 */
int Fenix_Data_group_delete(int group_id);

/**
 * @brief Delete a data member.
 * @qualifier local
 *
 * @param[in] group_id The group to delete from
 * @param[in] member_id The member to delete
 * @returnstatus
 */
int Fenix_Data_member_delete(int group_id, int member_id);
/**@}*/

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif // __FENIX__
