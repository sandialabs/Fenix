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
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include "fenix_util.hpp"
#include "fenix_ext.hpp"
#include "fenix.hpp"
#include "fenix/data/subset.hpp"

#include <cassert>

using namespace fenix;
using namespace fenix::data;

DataSubset SUBSET_FULL_IMPL{DataSubset::MAX};
DataSubset SUBSET_EMPTY_IMPL{};
DataSubset SUBSET_PRESTAGED_IMPL{DataSubset::PrestagedSubset};

const Fenix_Data_subset FENIX_DATA_SUBSET_FULL      = {&SUBSET_FULL_IMPL};
const Fenix_Data_subset FENIX_DATA_SUBSET_EMPTY     = {&SUBSET_EMPTY_IMPL};
const Fenix_Data_subset FENIX_DATA_SUBSET_PRESTAGED = {&SUBSET_PRESTAGED_IMPL};
Fenix_Data_subset* FENIX_DATA_SUBSET_IGNORE         = NULL;

int Fenix_set_option(Fenix_Setting_name setting, unsigned option) {
  // This function gets special handling, since it can be called before init
#ifdef FENIC_C_CATCH_RUNTIME_EXCEPTIONS
  try {
    set_option(setting, option);
    return FENIX_SUCCESS;
  } catch (const fenix::RuntimeException& e) {
    return e.error;
  }
#else
  set_option(setting, option);
  return FENIX_SUCCESS;
#endif
}

int Fenix_get_option(Fenix_Setting_name setting, unsigned* option) {
  FENIX_C_API_BEGIN
  *option = get_option(setting);
  return FENIX_SUCCESS;
  FENIX_C_API_END
}

int Fenix_Callback_register(
  void (*recover)(MPI_Comm, int, void*), void* callback_data
) {
  FENIX_C_API_BEGIN
  return callback_register([recover,
                            callback_data](MPI_Comm comm, int fenix_error) {
    recover(comm, fenix_error, callback_data);
  });
  FENIX_C_API_END
}

int Fenix_Callback_pop() {
  FENIX_C_API_BEGIN
  return callback_pop();
  FENIX_C_API_END
}

int Fenix_Callback_invoke_all() {
  FENIX_C_API_BEGIN
  return callback_invoke_all();
  FENIX_C_API_END
}

int Fenix_Initialized(int* flag) {
  *flag = (fenix_rt.fenix_init_flag) ? 1 : 0;
  return FENIX_SUCCESS;
}

int Fenix_Finalized(int* flag) {
  *flag = (fenix_rt.finalized) ? 1 : 0;
  return FENIX_SUCCESS;
}

int Fenix_Process_fail_list(int** fail_list) {
  FENIX_C_API_BEGIN
  *fail_list = fenix_rt.fail_world;
  return fenix_rt.fail_world_size;
  FENIX_C_API_END
}

int Fenix_check_cancelled(MPI_Request* request, MPI_Status* status) {
  FENIX_C_API_BEGIN
  // We know this may return as "COMM_REVOKED", but we know the error was
  // already handled
  fenix::util::ScopedIgnoreAndReturn opt;

  int flag;
  int ret = PMPI_Test(request, &flag, status);

  //Request was (potentially) cancelled if ret is MPI_ERR_PROC_FAILED
  return ret == MPI_ERR_PROC_FAILED || ret == MPI_ERR_REVOKED;
  FENIX_C_API_END
}

int Fenix_Process_detect_failures(int do_recovery) {
  FENIX_C_API_BEGIN
  return detect_failures(do_recovery);
  FENIX_C_API_END
}

Fenix_Rank_role Fenix_get_role() {
  assert(initialized() || fenix_rt.finalized);
  return role();
}

int Fenix_get_error() {
  assert(initialized());
  return error();
}

int Fenix_get_nspare() {
  assert(initialized());
  return nspare();
}

namespace fenix {

template <typename T>
static void set_opt_enum(T& t, unsigned opt, unsigned opt_oob) {
  static_assert(std::is_enum_v<T>);
  if (opt >= opt_oob) FENIX_THROW(FENIX_ERROR_INVALID_SETTING_OPTION);
  t = static_cast<T>(opt);
}

#define SET_OPTION_CASE(name, var)                                             \
  case name##_MODE:                                                            \
    set_opt_enum(settings.var, option, FENIX_##name##_MODE_MAXCODE);           \
    break

void set_option(SettingName setting, unsigned option) {
  if (setting >= FENIX_SETTING_NAME_MAXCODE) {
    FENIX_THROW(FENIX_ERROR_INVALID_SETTING_NAME);
  }

  Settings& settings =
    fenix_rt.fenix_init_flag ? fenix_rt.settings : fenix_default_settings;
  switch (setting) {
    SET_OPTION_CASE(RECOVERY, recovery);
    SET_OPTION_CASE(RESUME, resume);
    SET_OPTION_CASE(UNHANDLED, unhandled);
    SET_OPTION_CASE(CALLBACK_EXCEPTION, cb_exception);
    SET_OPTION_CASE(MLOG_RECOVERY, mlog_recovery);
    SET_OPTION_CASE(SPARE_WAIT, spare_wait);
    SET_OPTION_CASE(SPARE_FINALIZE, spare_finalize);
  default:
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

#define GET_OPTION_CASE(n, v)                                                  \
  case n##_MODE:                                                               \
    return fenix_rt.settings.v

unsigned get_option(SettingName setting) {
  if (!fenix_rt.fenix_init_flag) FENIX_THROW(FENIX_ERROR_UNINITIALIZED);
  switch (setting) {
    GET_OPTION_CASE(RECOVERY, recovery);
    GET_OPTION_CASE(RESUME, resume);
    GET_OPTION_CASE(UNHANDLED, unhandled);
    GET_OPTION_CASE(CALLBACK_EXCEPTION, cb_exception);
    GET_OPTION_CASE(MLOG_RECOVERY, mlog_recovery);
    GET_OPTION_CASE(SPARE_WAIT, spare_wait);
    GET_OPTION_CASE(SPARE_FINALIZE, spare_finalize);
  default:
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
}

void throw_exception() {
  assert(initialized());
  throw CommException(
    fenix_rt.user_world, *fenix_rt.ret_error, MPI_ERR_UNKNOWN
  );
}

Fenix_Rank_role role() {
  assert(initialized() || fenix_rt.finalized);
  return (Fenix_Rank_role)fenix_rt.role;
}

int error() {
  assert(initialized());
  return fenix_rt.repair_result;
}

int nspare() {
  assert(initialized());
  return fenix_rt.spare_ranks;
}

std::vector<int> fail_list() {
  assert(initialized());
  if (fenix_rt.fail_world_size == 0) return {};
  return {fenix_rt.fail_world, fenix_rt.fail_world + fenix_rt.fail_world_size};
}

bool initialized() { return fenix_rt.fenix_init_flag; }

bool finalized() { return fenix_rt.finalized; }

namespace data {
const DataSubset& SUBSET_FULL      = SUBSET_FULL_IMPL;
const DataSubset& SUBSET_EMPTY     = SUBSET_EMPTY_IMPL;
const DataSubset& SUBSET_PRESTAGED = SUBSET_PRESTAGED_IMPL;
DataSubset SUBSET_IGNORE           = SUBSET_EMPTY;

int group_create(int group_id, GroupCreateArgs args) {
  FENIX_CPP_API_BEGIN
  if (args.comm == MPI_COMM_NULL) args.comm = fenix_rt.user_world;
  int ignore;
  if (args.flag == nullptr) args.flag = &ignore;
  return group_create(
    group_id, args.comm, args.start_time_stamp, args.depth, args.policy_name,
    args.policy_value, args.flag
  );
  FENIX_CPP_API_END
}

} // namespace data

} //namespace fenix

int Fenix_Data_group_create(
  int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy,
  void* policy_args, int* flag
) {
  FENIX_C_API_BEGIN
  return group_create(
    group_id, comm, start_time_stamp, depth, policy, policy_args, flag
  );
  FENIX_C_API_END
}

int Fenix_Data_group_created(int group_id) {
  FENIX_C_API_BEGIN
  return group_created(group_id);
  FENIX_C_API_END
}

int Fenix_Data_member_create(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
) {
  FENIX_C_API_BEGIN
  return member_create(group_id, member_id, buffer, count, datatype);
  FENIX_C_API_END
}

int Fenix_Data_member_fcreate(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  Fenix_Serialize_file_fn serializer, void* ctx
) {
  FENIX_C_API_BEGIN
  return member_create(
    group_id, member_id, buffer, count, datatype,
    [serializer, ctx](FILE* fp, int d, void* b, int o, int c) {
      return serializer(fp, d, b, o, c, ctx);
    }
  );
  FENIX_C_API_END
}

int Fenix_Data_member_define(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
) {
  FENIX_C_API_BEGIN
  return member_define(group_id, member_id, buffer, count, datatype);
  FENIX_C_API_END
}

int Fenix_Data_member_fdefine(
  int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype,
  Fenix_Serialize_file_fn serializer, void* ctx
) {
  FENIX_C_API_BEGIN
  int ret = member_define(group_id, member_id, buffer, count, datatype);
  if (ret == FENIX_SUCCESS) {
    fenix_rt.data_recovery->find_member(group_id, member_id)
      ->ser_func.emplace([serializer,
                          ctx](FILE* fp, int d, void* b, int o, int c) {
        return serializer(fp, d, b, o, c, ctx);
      });
  }
  return ret;
  FENIX_C_API_END
}

int Fenix_Data_member_attr_set(
  int groupid, int memberid, int name, void* value, int* flag
) {
  FENIX_C_API_BEGIN
  return member_attr_set(groupid, memberid, name, value, flag);
  FENIX_C_API_END
}

int Fenix_Data_member_created(int group_id, int member_id) {
  FENIX_C_API_BEGIN
  return member_created(group_id, member_id);
  FENIX_C_API_END
}

int Fenix_Data_member_store(
  int group_id, int member_id, const Fenix_Data_subset subset
) {
  FENIX_C_API_BEGIN
  return member_store(group_id, member_id, *(DataSubset*)subset.impl);
  FENIX_C_API_END
}

int Fenix_Data_member_storev(
  int group_id, int member_id, const Fenix_Data_subset subset
) {
  FENIX_C_API_BEGIN
  return member_storev(group_id, member_id, *(DataSubset*)subset.impl);
  FENIX_C_API_END
}

int Fenix_Data_member_istore(
  int group_id, int member_id, const Fenix_Data_subset subset,
  Fenix_Request* request
) {
  FENIX_C_API_BEGIN
  return member_istore(group_id, member_id, *(DataSubset*)subset.impl, request);
  FENIX_C_API_END
}

int Fenix_Data_member_istorev(
  int group_id, int member_id, const Fenix_Data_subset subset,
  Fenix_Request* request
) {
  FENIX_C_API_BEGIN
  return member_istorev(
    group_id, member_id, *(DataSubset*)subset.impl, request
  );
  FENIX_C_API_END
}

int Fenix_Data_commit(int group_id, int* time_stamp) {
  FENIX_C_API_BEGIN
  return commit(group_id, time_stamp);
  FENIX_C_API_END
}

int Fenix_Data_commit_barrier(int group_id, int* time_stamp) {
  FENIX_C_API_BEGIN
  return commit_barrier(group_id, time_stamp);
  FENIX_C_API_END
}

int Fenix_Data_checkpoint(
  int group_id, const Fenix_Data_subset subset, int num_storev, int* storev_ids,
  int* time_stamp
) {
  FENIX_C_API_BEGIN
  if (num_storev == FENIX_STOREV_ALL) {
    return checkpointv(group_id, *(DataSubset*)subset.impl, time_stamp);
  } else {
    std::vector<int> ids;
    for (int i = 0; i < num_storev; i++) {
      ids.push_back(storev_ids[i]);
    }
    return checkpoint(group_id, *(DataSubset*)subset.impl, ids, time_stamp);
  }
  FENIX_C_API_END
}

int Fenix_Data_member_restore(
  int group_id, int member_id, void* target_buffer, int max_count,
  int time_stamp, Fenix_Data_subset* data_found
) {
  FENIX_C_API_BEGIN
  DataSubset* s = new DataSubset();
  int ret       = member_restore(
    group_id, member_id, target_buffer, max_count, time_stamp, *s
  );
  if (data_found == nullptr) {
    delete s;
  } else {
    data_found->impl = s;
  }
  return ret;
  FENIX_C_API_END
}

int Fenix_Data_member_lrestore(
  int group_id, int member_id, void* target_buffer, int max_count,
  int time_stamp, Fenix_Data_subset* data_found
) {
  FENIX_C_API_BEGIN
  DataSubset* s = new DataSubset();
  int ret =
    member_load(group_id, member_id, target_buffer, max_count, time_stamp, *s);
  if (data_found == nullptr) {
    delete s;
  } else {
    data_found->impl = s;
  }
  return ret;
  FENIX_C_API_END
}

int Fenix_Data_subset_create(
  int num_blocks, int start_offset, int end_offset, int stride,
  Fenix_Data_subset* subset
) {
  FENIX_LOCAL_C_API_BEGIN
  subset->impl = new DataSubset({start_offset, end_offset}, num_blocks, stride);
  return FENIX_SUCCESS;
  FENIX_LOCAL_C_API_END
}

int Fenix_Data_subset_createv(
  int num_blocks, int* array_start_offsets, int* array_end_offsets,
  Fenix_Data_subset* subset
) {
  FENIX_LOCAL_C_API_BEGIN
  subset->impl =
    new DataSubset(num_blocks, array_start_offsets, array_end_offsets);
  return FENIX_SUCCESS;
  FENIX_LOCAL_C_API_END
}

int Fenix_Data_subset_delete(Fenix_Data_subset* subset) {
  FENIX_LOCAL_C_API_BEGIN
  delete (DataSubset*)subset->impl;
  subset->impl = nullptr;
  return FENIX_SUCCESS;
  FENIX_LOCAL_C_API_END
}

int Fenix_Data_snapshot_delete(int group_id, int time_stamp) {
  FENIX_C_API_BEGIN
  return snapshot_delete(group_id, time_stamp);
  FENIX_C_API_END
}

int Fenix_Data_group_delete(int group_id) {
  FENIX_C_API_BEGIN
  return group_delete(group_id);
  FENIX_C_API_END
}

int Fenix_Data_member_delete(int group_id, int member_id) {
  FENIX_C_API_BEGIN
  return member_delete(group_id, member_id);
  FENIX_C_API_END
}

int Fenix_Mlog_create(int mlog_id, MPI_Comm* comm, int depth) {
  FENIX_C_API_BEGIN
  return mlog::create(mlog_id, *comm, depth);
  FENIX_C_API_END
}

int Fenix_Mlog_activate(int mlog_id) {
  FENIX_C_API_BEGIN
  return mlog::activate(mlog_id);
  FENIX_C_API_END
}

int Fenix_Mlog_active(int* mlog_id) {
  FENIX_C_API_BEGIN
  *mlog_id = mlog::active();
  return FENIX_SUCCESS;
  FENIX_C_API_END
}

int Fenix_Mlog_begin_region(int mlog_id, int region_id) {
  FENIX_C_API_BEGIN
  return mlog::begin_region(mlog_id, region_id);
  FENIX_C_API_END
}

int Fenix_Mlog_activate_region(int mlog_id, int region_id) {
  FENIX_C_API_BEGIN
  return mlog::activate(mlog_id, region_id);
  FENIX_C_API_END
}

int Fenix_Mlog_sync(int mlog_id, int region_id) {
  FENIX_C_API_BEGIN
  return mlog::sync(mlog_id, region_id);
  FENIX_C_API_END
}

int Fenix_Mlog_create_data_member(int mlog_id, int group_id, int member_id) {
  FENIX_C_API_BEGIN
  return mlog::create_data_member(mlog_id, group_id, member_id);
  FENIX_C_API_END
}

int Fenix_Mlog_define_data_member(int mlog_id, int group_id, int member_id) {
  FENIX_C_API_BEGIN
  return mlog::define_data_member(mlog_id, group_id, member_id);
  FENIX_C_API_END
}

int Fenix_Mlog_delete(int mlog_id) {
  FENIX_C_API_BEGIN
  return mlog::mlog_delete(mlog_id);
  FENIX_C_API_END
}
