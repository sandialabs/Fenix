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

#ifndef __FENIX_HPP__
#define __FENIX_HPP__

#include <mpi.h>
#include <functional>
#include <vector>
#include <optional>
#include <variant>

#include "fenix.h"
#include "fenix_exception.hpp"
#include "fenix_data_subset.hpp"

namespace fenix {

using Role                    = Fenix_Rank_role;
constexpr Role INITIAL_RANK   = FENIX_ROLE_INITIAL_RANK;
constexpr Role RECOVERED_RANK = FENIX_ROLE_RECOVERED_RANK;
constexpr Role SURVIVOR_RANK  = FENIX_ROLE_SURVIVOR_RANK;

using SettingName                             = Fenix_Setting_name;
constexpr SettingName RECOVERY_MODE           = FENIX_RECOVERY_MODE;
constexpr SettingName RESUME_MODE             = FENIX_RESUME_MODE;
constexpr SettingName UNHANDLED_MODE          = FENIX_UNHANDLED_MODE;
constexpr SettingName CALLBACK_EXCEPTION_MODE = FENIX_CALLBACK_EXCEPTION_MODE;
constexpr SettingName MLOG_RECOVERY_MODE      = FENIX_MLOG_RECOVERY_MODE;
constexpr SettingName SPARE_WAIT_MODE         = FENIX_SPARE_WAIT_MODE;

using RecoveryMode            = Fenix_Recovery_mode;
constexpr RecoveryMode IGNORE = FENIX_RECOVERY_IGNORE;
constexpr RecoveryMode NOOP   = FENIX_RECOVERY_NOOP;
constexpr RecoveryMode REPAIR = FENIX_RECOVERY_REPAIR;
constexpr RecoveryMode SPAWN  = FENIX_RECOVERY_SPAWN;

using ResumeMode            = Fenix_Resume_mode;
constexpr ResumeMode JUMP   = FENIX_RESUME_JUMP;
constexpr ResumeMode RETURN = FENIX_RESUME_RETURN;
constexpr ResumeMode THROW  = FENIX_RESUME_THROW;

using UnhandledMode            = Fenix_Unhandled_mode;
constexpr UnhandledMode SILENT = FENIX_UNHANDLED_SILENT;
constexpr UnhandledMode PRINT  = FENIX_UNHANDLED_PRINT;
constexpr UnhandledMode ABORT  = FENIX_UNHANDLED_ABORT;

using CallbackExceptionMode             = Fenix_Callback_exception_mode;
constexpr CallbackExceptionMode RETHROW = FENIX_CALLBACK_EXCEPTION_RETHROW;
constexpr CallbackExceptionMode SQUASH  = FENIX_CALLBACK_EXCEPTION_SQUASH;

using MlogRecoveryMode            = Fenix_Mlog_recovery_mode;
constexpr MlogRecoveryMode MANUAL = FENIX_MLOG_RECOVERY_MANUAL;
constexpr MlogRecoveryMode INLINE = FENIX_MLOG_RECOVERY_INLINE;
constexpr MlogRecoveryMode INLINE_AUTOSYNC =
  FENIX_MLOG_RECOVERY_INLINE_AUTOSYNC;

using SpareWaitMode           = Fenix_Spare_wait_mode;
constexpr SpareWaitMode BUSY  = FENIX_SPARE_WAIT_BUSY;
constexpr SpareWaitMode YIELD = FENIX_SPARE_WAIT_YIELD;
constexpr SpareWaitMode SLEEP = FENIX_SPARE_WAIT_SLEEP;

constexpr int STOREV_ALL = FENIX_STOREV_ALL;

enum CallbackLocation { PRE_RECOVERY, POST_RECOVERY };

namespace args {
struct FenixInitArgs {
  int* role          = nullptr;
  MPI_Comm in_comm   = MPI_COMM_WORLD;
  MPI_Comm* out_comm = nullptr;
  int* argc          = nullptr;
  char*** argv       = nullptr;
  int spares         = 0;
  int* err           = nullptr;
};
}

void init(const args::FenixInitArgs args);

//!@brief overload of #Fenix_set_option
void set_option(SettingName setting, unsigned option);

//!@brief overload of #Fenix_get_option returning the option directly
unsigned get_option(SettingName setting);

//!@brief Throw an exception for the most recent fault. Helpful for spares.
void throw_exception();

//!@brief Overload of #Fenix_get_role
Fenix_Rank_role role();

//!@brief Overload of #Fenix_get_error
int error();

//!@brief Overload of #Fenix_get_nspare
int nspare();

using FenixCallbackFunc = std::function<void(MPI_Comm, int)>;

//!@brief Overload of #Fenix_Callback_register
int callback_register(
  FenixCallbackFunc callback, CallbackLocation loc = POST_RECOVERY
);

//@!brief Overload of #Fenix_Callback_pop
int callback_pop(CallbackLocation loc = POST_RECOVERY);

//@!brief Overload of #Fenix_Callback_invoke_all
int callback_invoke_all(CallbackLocation loc = POST_RECOVERY);

/**
 * @brief Get the failed ranks from the most recent recovery
 * @return vector of failed ranks
 */
std::vector<int> fail_list();

//!@brief Overload of #Fenix_Process_detect_failures
int detect_failures(bool recover = true);

//!@brief Overload of #Fenix_Initialized that directly returns true if initialized
bool initialized();

//!@brief Overload of #Fenix_Finalized that directly returns true if finalized
bool finalized();

//!@brief Overload of #MPI_Comm_revoke for apps to use until next MPI release
int comm_revoke(MPI_Comm comm);

} // namespace fenix

namespace fenix::data {

extern const DataSubset& SUBSET_FULL;
extern const DataSubset& SUBSET_EMPTY;
extern const DataSubset& SUBSET_PRESTAGED;
extern DataSubset SUBSET_IGNORE;

//@!brief Overload of #Fenix_Data_group_create
int group_create(
  int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name,
  void* policy_value, int* flag
);

struct GroupCreateArgs {
  // MPI_COMM_NULL defaults to the resilient communicator
  MPI_Comm comm        = MPI_COMM_NULL;
  int start_time_stamp = 0;
  int depth            = 1;
  int policy_name      = FENIX_DATA_POLICY_IMR;
  void* policy_value   = nullptr;
  int* flag            = nullptr;
};
int group_create(int group_id, GroupCreateArgs args = {});

//@!brief Overload of #Fenix_Data_group_created
bool group_created(int group_id);

//@!brief Overload of #Fenix_Data_member_create
int member_create(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);

//!@brief As #Fenix_Serialize_file_fn without the void* context pointer.
using SerializeFileFunc = std::function<void(FILE*, int, void*, int, int)>;
//!@brief As #SerializeFileFunc using std::iostream instead of a file pointer
using SerializeStreamFunc =
  std::function<void(std::iostream&, int, void*, int, int)>;

using SerializeFunc = std::variant<SerializeFileFunc, SerializeStreamFunc>;

//!@brief Overload of #Fenix_Data_member_fcreate
int member_create(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  SerializeFunc serializer
);

//!@brief Overload of #Fenix_Data_member_define
int member_define(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);
//!@brief Overload of #Fenix_Data_member_fdefine
int member_define(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  SerializeFunc serializer
);

//@!brief Overload of #Fenix_Data_member_created
bool member_created(int group_id, int member_id);

int member_attr_set(
  int group_id, int member_id, int attr, void* value, int* flag
);

//!@brief Overload of #Fenix_Data_member_stage
int member_stage(
  int group_id, int member_id, const DataSubset& subset = SUBSET_FULL
);

//!@brief Overload of #Fenix_Data_member_stage_inplace
int member_stage_inplace(
  int group_id, int member_id, void* buf, const DataSubset& subset = SUBSET_FULL
);

//!@brief Overload of #Fenix_Data_member_store
int member_store(
  int group_id, int member_id, const DataSubset& subset = SUBSET_FULL
);
//!@brief Overload of #Fenix_Data_member_store, stores all members
inline int member_store(int group_id, const DataSubset& subset = SUBSET_FULL) {
  return member_store(group_id, FENIX_DATA_MEMBER_ALL, subset);
}

//!@brief Overload of #Fenix_Data_member_storev
int member_storev(int group_id, int member_id, const DataSubset& subset);
//!@brief Overload of #Fenix_Data_member_storev, stores all members
inline int member_storev(int group_id, const DataSubset& subset) {
  return member_storev(group_id, FENIX_DATA_MEMBER_ALL, subset);
}

//!@brief Overload of #Fenix_Data_member_istore
int member_istore(
  int group_id, int member_id, const DataSubset& subset, Fenix_Request* request
);

struct MemberIstoreArgs {
  int member_id            = FENIX_DATA_MEMBER_ALL;
  const DataSubset& subset = SUBSET_FULL;
};
//!@brief Overload of #Fenix_Data_member_istore
inline int member_istore(
  int group_id, Fenix_Request* request, MemberIstoreArgs args = {}
) {
  return member_istore(group_id, args.member_id, args.subset, request);
}

//!@brief Overload of #Fenix_Data_member_istorev
int member_istorev(
  int group_id, int member_id, const DataSubset& subset, Fenix_Request* request
);
//!@brief Overload of #Fenix_Data_member_istorev, stores all members
inline int member_istorev(
  int group_id, const DataSubset& subset, Fenix_Request* request
) {
  return member_istorev(group_id, FENIX_DATA_MEMBER_ALL, subset, request);
}

//!@brief Overload of #Fenix_Data_member_restore
int member_restore(
  int group_id, int member_id, void* target_buffer = FENIX_DATA_RESTORE_INPLACE,
  int max_length         = FENIX_DATA_RESTORE_FULL,
  int time_stamp         = FENIX_DATA_SNAPSHOT_ALL,
  DataSubset& data_found = SUBSET_IGNORE
);

//!@brief Overload of #Fenix_Data_member_lrestore
int member_lrestore(
  int group_id, int member_id, void* target_buffer = FENIX_DATA_RESTORE_INPLACE,
  int max_length         = FENIX_DATA_RESTORE_FULL,
  int time_stamp         = FENIX_DATA_SNAPSHOT_ALL,
  DataSubset& data_found = SUBSET_IGNORE
);

//!@brief overload of #Fenix_Data_commit
int commit(int group_id, int* time_stamp = nullptr);

//!@brief overload of #Fenix_Data_commit
int commit_barrier(int group_id, int* time_stamp = nullptr);

//!@brief Overload of #Fenix_Data_checkpoint
int checkpoint(
  int group_id, const DataSubset& subset,
  const std::vector<int>& storev_ids = {}, int* time_stamp = nullptr
);

//!@brief Overload of #Fenix_Data_checkpoint for FENIX_STOREV_ALL
int checkpointv(
  int group_id, const DataSubset& subset, int* time_stamp = nullptr
);

/**
 * @brief get the members of a group
 * @return vector of member IDs of each member in group_id if group exists
 */
std::optional<std::vector<int>> group_members(int group_id);

/**
 * @brief get the snapshots of a group
 * @return vector of timestamps of each snapshot in group_id if group exists
 */
std::optional<std::vector<int>> group_snapshots(int group_id);

//@!brief Overload of #Fenix_Data_snapshot_delete
int snapshot_delete(int group_id, int timestamp);

//@!brief Overload of #Fenix_Data_group_delete
int group_delete(int group_id);

//@!brief Overload of #Fenix_Data_member_delete
int member_delete(int group_id, int member_id);

} // namespace fenix::data

namespace fenix::mlog {

//@brief Overload of #Fenix_Mlog_create
int create(int mlog_id, MPI_Comm& comm, int depth);

//@brief Overload of #Fenix_Mlog_activate
int activate(int mlog_id);

//@brief Overload of Fenix_Mlog_active, returns active log
int active();

//@brief Overload of #Fenix_Mlog_begin_region
int begin_region(int mlog_id, int region_id);

//@brief Overload of #Fenix_Mlog_activate_region
int activate(int mlog_id, int region_id);

//@brief Overload of #Fenix_Mlog_sync
int sync(int mlog_id, int region_id = FENIX_MLOG_CONTINUE);

//@brief Overload of #Fenix_Mlog_create_data_member
int create_data_member(int mlog_id, int group_id, int member_id);

//@brief Overload of #Fenix_Mlog_define_data_member
int define_data_member(int mlog_id, int group_id, int member_id);

//@brief Overload of #Fenix_Mlog_delete
int mlog_delete(int mlog_id);

} // namespace fenix::mlog

#endif
