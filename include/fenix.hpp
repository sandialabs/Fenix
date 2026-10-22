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
#include "fenix/data/subset.hpp"

namespace fenix {

using Role                    = Fenix_Rank_role;
constexpr Role INITIAL_RANK   = FENIX_ROLE_INITIAL_RANK;
constexpr Role RECOVERED_RANK = FENIX_ROLE_RECOVERED_RANK;
constexpr Role SURVIVOR_RANK  = FENIX_ROLE_SURVIVOR_RANK;
constexpr Role SPARE_RANK     = FENIX_ROLE_SPARE_RANK;
constexpr Role MISSING_RANK   = FENIX_ROLE_MISSING_RANK;

using SettingName                             = Fenix_Setting_name;
constexpr SettingName RECOVERY_MODE           = FENIX_RECOVERY_MODE;
constexpr SettingName RESUME_MODE             = FENIX_RESUME_MODE;
constexpr SettingName UNHANDLED_MODE          = FENIX_UNHANDLED_MODE;
constexpr SettingName CALLBACK_EXCEPTION_MODE = FENIX_CALLBACK_EXCEPTION_MODE;
constexpr SettingName MLOG_RECOVERY_MODE      = FENIX_MLOG_RECOVERY_MODE;
constexpr SettingName SPARE_WAIT_MODE         = FENIX_SPARE_WAIT_MODE;
constexpr SettingName SPARE_FINALIZE_MODE     = FENIX_SPARE_FINALIZE_MODE;

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

using SpareFinalizeMode             = Fenix_Spare_finalize_mode;
constexpr SpareFinalizeMode RELEASE = FENIX_SPARE_FINALIZE_RELEASE;
constexpr SpareFinalizeMode EXIT    = FENIX_SPARE_FINALIZE_EXIT;

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

bool initialized();
bool finalized();

void set_option(SettingName setting, unsigned option);
unsigned get_option(SettingName setting);

// Throw an exception for the most recent fault. Helpful for spares.
void throw_exception();

Fenix_Rank_role role();
int error();
int nspare();
std::vector<int> fail_list();

using FenixCallbackFunc = std::function<void(MPI_Comm, int)>;
int callback_register(
  FenixCallbackFunc callback, CallbackLocation loc = POST_RECOVERY
);
int callback_pop(CallbackLocation loc = POST_RECOVERY);
int callback_invoke_all(CallbackLocation loc = POST_RECOVERY);

int detect_failures(bool recover = true);

// Overload of #MPI_Comm_revoke for apps to use until next MPI release
int comm_revoke(MPI_Comm comm);

} // namespace fenix

namespace fenix::data {

extern const DataSubset& SUBSET_FULL;
extern const DataSubset& SUBSET_EMPTY;
extern const DataSubset& SUBSET_PRESTAGED;
extern DataSubset SUBSET_IGNORE;

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

bool group_created(int group_id);


// file, direction (FENIX_(DE)SERIALIZE), buffer, offset, count 
using SerializeFileFunc = std::function<void(FILE*, int, void*, int, int)>;
// stream, direction (FENIX_(DE)SERIALIZE), buffer, offset, count 
using SerializeStreamFunc =
  std::function<void(std::iostream&, int, void*, int, int)>;

using SerializeFunc = std::variant<SerializeFileFunc, SerializeStreamFunc>;

int member_create(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);
int member_define(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);

int member_create(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  SerializeFunc serializer
);
int member_define(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  SerializeFunc serializer
);

bool member_created(int group_id, int member_id);

int member_attr_set(
  int group_id, int member_id, int attr, void* value, int* flag
);
int member_attr_get(
  int group_id, int member_id, int attr, void* value, int* flag
);

int member_stage(
  int group_id, int member_id, const DataSubset& subset = SUBSET_FULL
);
int member_stage_inplace(
  int group_id, int member_id, void* buf, const DataSubset& subset = SUBSET_FULL
);
int member_stage_begin(int group_id, int member_id, FILE** fp);
int member_stage_begin(int group_id, int member_id, std::iostream** stream);
int member_stage_end(int group_id, int member_id);

int member_store(
  int group_id, int member_id, const DataSubset& subset = SUBSET_FULL
);
inline int member_store(int group_id, const DataSubset& subset = SUBSET_FULL) {
  return member_store(group_id, FENIX_DATA_MEMBER_ALL, subset);
}
int member_storev(int group_id, int member_id, const DataSubset& subset);
inline int member_storev(int group_id, const DataSubset& subset) {
  return member_storev(group_id, FENIX_DATA_MEMBER_ALL, subset);
}

// member_istore variants are currently unimplemented.
int member_istore(
  int group_id, int member_id, const DataSubset& subset, Fenix_Request* request
);
struct MemberIstoreArgs {
  int member_id            = FENIX_DATA_MEMBER_ALL;
  const DataSubset& subset = SUBSET_FULL;
};
inline int member_istore(
  int group_id, Fenix_Request* request, MemberIstoreArgs args = {}
) {
  return member_istore(group_id, args.member_id, args.subset, request);
}
int member_istorev(
  int group_id, int member_id, const DataSubset& subset, Fenix_Request* request
);
inline int member_istorev(
  int group_id, const DataSubset& subset, Fenix_Request* request
) {
  return member_istorev(group_id, FENIX_DATA_MEMBER_ALL, subset, request);
}

int member_repair(int group_id, int member_id);

int member_load(
  int group_id, int member_id, int time_stamp = FENIX_DATA_SNAPSHOT_ALL,
  DataSubset& data_found = SUBSET_IGNORE
);
int member_load(
  int group_id, int member_id, void* target,
  int target_count       = FENIX_DATA_RESTORE_FULL,
  int time_stamp         = FENIX_DATA_SNAPSHOT_ALL,
  DataSubset& data_found = SUBSET_IGNORE
);
int member_load_begin(
  int group_id, int member_id, FILE** fp,
  int time_stamp         = FENIX_DATA_SNAPSHOT_LATEST,
  DataSubset& data_found = SUBSET_IGNORE
);
int member_load_begin(
  int group_id, int member_id, std::iostream** strm,
  int time_stamp         = FENIX_DATA_SNAPSHOT_LATEST,
  DataSubset& data_found = SUBSET_IGNORE
);
int member_load_end(int group_id, int member_id);

int member_restore(
  int group_id, int member_id, void* target_buffer = FENIX_DATA_RESTORE_INPLACE,
  int max_length         = FENIX_DATA_RESTORE_FULL,
  int time_stamp         = FENIX_DATA_SNAPSHOT_ALL,
  DataSubset& data_found = SUBSET_IGNORE
);

[[deprecated("Use member_load functions instead")]]
int member_lrestore(
  int group_id, int member_id, void* target_buffer = FENIX_DATA_RESTORE_INPLACE,
  int max_length         = FENIX_DATA_RESTORE_FULL,
  int time_stamp         = FENIX_DATA_SNAPSHOT_ALL,
  DataSubset& data_found = SUBSET_IGNORE
);

int commit(int group_id, int* time_stamp = nullptr);
int commit_barrier(int group_id, int* time_stamp = nullptr);

int checkpoint(
  int group_id, const DataSubset& subset,
  const std::vector<int>& storev_ids = {}, int* time_stamp = nullptr
);
int checkpointv(
  int group_id, const DataSubset& subset, int* time_stamp = nullptr
);

std::optional<std::vector<int>> group_members(int group_id);
std::optional<std::vector<int>> group_snapshots(int group_id);
MPI_Group group_cohort(int group_id);

int snapshot_delete(int group_id, int timestamp);
int group_delete(int group_id);
int member_delete(int group_id, int member_id);

} // namespace fenix::data

namespace fenix::mlog {

int create(int mlog_id, MPI_Comm& comm, int depth);

int activate(int mlog_id);
int activate(int mlog_id, int region_id);

int active();

int begin_region(int mlog_id, int region_id);

int sync(int mlog_id, int region_id = FENIX_MLOG_CONTINUE);

int create_data_member(int mlog_id, int group_id, int member_id);
int define_data_member(int mlog_id, int group_id, int member_id);

int mlog_delete(int mlog_id);

} // namespace fenix::mlog

#endif
