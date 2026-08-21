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

#include "fenix.hpp"
#include "fenix_data_policy.hpp"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"
#include "fenix_ext.hpp"
#include "fenix_data_subset.hpp"
#include "fenix/data/util/mstream.hpp"

#include <cassert>

#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif

using namespace fenix::data;

namespace fenix::data {

int group_create(
  int groupid, MPI_Comm comm, int timestart, int depth, int policy_name,
  void* policy_value, int* flag
) {
  FENIX_CPP_API_BEGIN
  fenix::util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  if (timestart < 0) FENIX_THROW(FENIX_ERROR_INVALID_TIMESTART);
  if (depth < -1) FENIX_THROW(FENIX_ERROR_INVALID_DEPTH);

  auto group                = search_group(groupid);
  fenix_data_recovery_t* dr = fenix_rt.data_recovery;

  if (!group) {
    // Initialize Group
    group = new_group(
      groupid, comm, timestart, depth, policy_name, policy_value, flag
    );

    // Place in an available group slot
    dr->group[__fenix_find_next_group_position(dr)] = group;
    // Update the count AFTER finding next group position.
    dr->count++;
  } else {
    // Already created. Renew the MPI communicator
    group->comm = comm;
    MPI_Comm_rank(comm, &(group->current_rank));

    // Reinit group metadata as needed w/ new communicator.
    group->reinit(flag);
  }

  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

bool group_created(int groupid) {
  FENIX_CPP_API_BEGIN
  return search_group(groupid) != nullptr;
  FENIX_CPP_API_END
}

int member_create(
  int groupid, int memberid, void* data, int count, MPI_Datatype datatype
) {
  FENIX_CPP_API_BEGIN
  return find_group(groupid)->member_create(memberid, data, count, datatype);
  FENIX_CPP_API_END
}

int member_create(
  int groupid, int memberid, void* data, int count, MPI_Datatype datatype,
  SerializeFunc serializer
) {
  FENIX_CPP_API_BEGIN
  return find_group(groupid)->member_create(
    memberid, data, count, datatype, serializer
  );
  FENIX_CPP_API_END
}

int member_define(
  int groupid, int memberid, void* data, int count, MPI_Datatype datatype
) {
  FENIX_CPP_API_BEGIN
  auto group  = find_group(groupid);
  auto member = group->search_member(memberid);
  if (!member) {
    return member_create(groupid, memberid, data, count, datatype);
  } else {
    int set;
    int ret = member_attr_set(
      groupid, memberid, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data, &set
    );
    if (ret == FENIX_SUCCESS)
      ret = member_attr_set(
        groupid, memberid, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, &count, &set
      );
    if (ret == FENIX_SUCCESS)
      ret = member_attr_set(
        groupid, memberid, FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE, &datatype, &set
      );
    member->ser_func.reset();
    return ret;
  }
  FENIX_CPP_API_END
}

int member_define(
  int groupid, int memberid, void* data, int count, MPI_Datatype datatype,
  SerializeFunc serializer
) {
  FENIX_CPP_API_BEGIN
  int ret = member_define(groupid, memberid, data, count, datatype);
  if (ret == FENIX_SUCCESS)
    find_group(groupid)->find_member(memberid)->ser_func.emplace(serializer);
  return ret;
  FENIX_CPP_API_END
}

int member_attr_set(
  int groupid, int memberid, int attr, void* value, int* flag
) {
  FENIX_CPP_API_BEGIN
  auto group  = find_group(groupid);
  auto mentry = group->find_member(memberid);

  //Always pass attribute changes along to group - they might have unknown
  //attributes or side-effects to handle from changes. They get change info
  //before changes are made, in case they need prior state.
  int retval = group->member_set_attribute(mentry, attr, value, flag);
  if (retval != FENIX_SUCCESS) return retval;

  switch (attr) {
  case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
    mentry->user_data = {(char*)value, mentry->user_data.size()};
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT: {
    int new_count = *((int*)value);
    if (new_count == FENIX_RESIZEABLE) {
      mentry->user_data = {mentry->user_data.data()};
    } else {
      mentry->user_data = {
        mentry->user_data.data(), new_count * mentry->datatype_size
      };
    }
    break;
  }
  case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE: {
    MPI_Datatype* dtype = (MPI_Datatype*)value;
    int dtype_size;
    int err = MPI_Type_size(*dtype, &dtype_size);
    if (err) throw RuntimeException("Invalid MPI_Datatype");

    // Update datatype size and recalculate user_data size for fixed-size members
    int old_count = mentry->elm_count();
    mentry->datatype_size = dtype_size;

    // If this is a fixed-size member, update user_data to reflect new size
    // based on the element count (which stays the same)
    if (old_count != FENIX_RESIZEABLE) {
      size_t new_size = old_count * dtype_size;
      mentry->user_data = {mentry->user_data.data(), new_size};
    }
    break;
  }
  default:
    // No problem, since the group policy successfully handled this
    break;
  }

  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

bool member_created(int group_id, int member_id) {
  FENIX_CPP_API_BEGIN
  auto group = search_group(group_id);
  return group && group->search_member(member_id);
  FENIX_CPP_API_END
}

int member_stage(int groupid, int memberid, const DataSubset& specifier) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_stage(memberid, specifier);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_stage_inplace(
  int groupid, int memberid, void* buf, const DataSubset& specifier
) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_stage_inplace(memberid, buf, specifier);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_stage_begin(int groupid, int memberid, FILE** fp) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_stage_begin(memberid, fp);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_stage_begin(int groupid, int memberid, std::iostream** stream) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_stage_begin(memberid, stream);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_stage_end(int groupid, int memberid) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_stage_end(memberid);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

// Quick little helper function for the 4 kinds of stores
template <auto Func, typename... Args>
static int store(int groupid, int memberid, Args&&... args) {
  fenix::util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  auto group = find_group(groupid);
  if (memberid == FENIX_DATA_MEMBER_ALL) {
    for (auto& [id, member] : group->members) {
      int ret = (group->*Func)(id, std::forward<Args>(args)...);
      if (ret != FENIX_SUCCESS) return ret;
    }
    return FENIX_SUCCESS;
  } else {
    return (group->*Func)(memberid, std::forward<Args>(args)...);
  }
}

int member_store(int groupid, int memberid, const DataSubset& specifier) {
  FENIX_CPP_API_BEGIN
  return store<&fenix_group_t::member_store>(groupid, memberid, specifier);
  FENIX_CPP_API_END
}

int member_storev(int groupid, int memberid, const DataSubset& specifier) {
  FENIX_CPP_API_BEGIN
  return store<&fenix_group_t::member_storev>(groupid, memberid, specifier);
  FENIX_CPP_API_END
}

int member_istore(
  int groupid, int memberid, const DataSubset& specifier, Fenix_Request* request
) {
  FENIX_CPP_API_BEGIN
  fatal_print("unimplemented");
  return store<&fenix_group_t::member_istore>(
    groupid, memberid, specifier, request
  );
  FENIX_CPP_API_END
}

int member_istorev(
  int groupid, int memberid, const DataSubset& specifier, Fenix_Request* request
) {
  FENIX_CPP_API_BEGIN
  fatal_print("unimplemented");
  return store<&fenix_group_t::member_istorev>(
    groupid, memberid, specifier, request
  );
  FENIX_CPP_API_END
}

int commit(int groupid, int* timestamp) {
  FENIX_CPP_API_BEGIN
  auto g       = find_group(groupid);
  g->timestamp = g->timestamp == -1 ? g->timestart : g->timestamp + 1;
  if (timestamp) *timestamp = g->timestamp;
  return g->commit();
  FENIX_CPP_API_END
}

int commit_barrier(int groupid, int* timestamp) {
  FENIX_CPP_API_BEGIN
  fenix::util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  //We want to make sure there aren't any failed MPI operations (IE unfinished
  //stores) But we don't want to fail to commit if a failure has happened after
  //all stores.
  auto g = find_group(groupid);

  //Our error handler also enters an agree, with a unique location bit set.
  //So if we aren't all here, we've hit an error already.
  int location      = FENIX_DATA_COMMIT_BARRIER_LOC;
  int err           = MPIX_Comm_agree(*fenix_rt.user_world, &location);
  bool can_commit   = location == FENIX_DATA_COMMIT_BARRIER_LOC;
  bool must_recover = !can_commit || err != MPI_SUCCESS;

  int ret = FENIX_ERROR_COMMIT_BARRIER;
  if (can_commit) {
    g->timestamp = g->timestamp == -1 ? g->timestart : g->timestamp + 1;
    if (timestamp) *timestamp = g->timestamp;
    ret = g->commit();
  }
  if (must_recover) {
    MPI_Comm_call_errhandler(*fenix_rt.user_world, MPI_ERR_PROC_FAILED);
  }
  return ret;
  FENIX_CPP_API_END
}

int checkpoint(
  int group_id, const DataSubset& subset, const std::vector<int>& storev_ids,
  int* timestamp
) {
  FENIX_CPP_API_BEGIN
  fenix::util::ScopedActiveMlog scoped_mlog(FENIX_MLOG_NONE);
  bool inline_recovery = scoped_mlog.old_inline_recovery;
  auto g               = find_group(group_id);

  int old_timestamp = g->timestamp;
  while (old_timestamp == g->timestamp) {
    try {
      for (int id : g->member_order) {
        bool must_storev = false;
        for (int i = 0; i < storev_ids.size() && !must_storev; i++) {
          if (storev_ids[i] == id) must_storev = true;
        }

        int ret;
        if (must_storev) {
          ret = g->member_storev(id, subset);
        } else {
          ret = g->member_store(id, subset);
        }
        if (ret != FENIX_SUCCESS) {
          fenix_assert(ret != FENIX_ERROR_CANCELLED);
          throw RuntimeException(
            ret, "Fenix_Data_checkpoint failed to store member"
          );
        }
      }

      g->timestamp = g->timestamp == -1 ? g->timestart : g->timestamp + 1;
      int ret      = g->commit();
      if (ret != FENIX_SUCCESS) {
        fenix_assert(ret != FENIX_ERROR_CANCELLED);
        throw RuntimeException(ret, "Fenix_Data_checkpoint failed to commit");
      }
    } catch (const CommException& e) {
      if (!inline_recovery) throw;
    }
  }

  if (timestamp) *timestamp = g->timestamp;
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int checkpointv(int group_id, const DataSubset& subset, int* time_stamp) {
  FENIX_CPP_API_BEGIN
  return checkpoint(
    group_id, subset, find_group(group_id)->member_order, time_stamp
  );
  FENIX_CPP_API_END
}

int member_repair(int groupid, int memberid) {
  FENIX_CPP_API_BEGIN
  // TODO: This should be the base function which member_restore invokes during
  // its execution
  return member_restore(
    groupid, memberid, nullptr, 0, FENIX_DATA_SNAPSHOT_ALL, SUBSET_IGNORE
  );
  FENIX_CPP_API_END
}

int member_load(
  int groupid, int memberid, int timestamp, DataSubset& data_found
) {
  FENIX_CPP_API_BEGIN
  return member_load(
    groupid, memberid, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    timestamp, data_found
  );
  FENIX_CPP_API_END
}

int member_load(
  int groupid, int memberid, void* target, int target_count, int timestamp,
  DataSubset& data_found
) {
  FENIX_CPP_API_BEGIN
  // TODO: This shold be the base function, not member_lrestore
  return member_lrestore(
    groupid, memberid, target, target_count, timestamp, data_found
  );
  FENIX_CPP_API_END
}

int member_load_begin(
  int groupid, int memberid, FILE** fp, int timestamp, DataSubset& data_found
) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_load_begin(memberid, fp, timestamp, data_found);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_load_begin(
  int groupid, int memberid, std::iostream** strm, int timestamp,
  DataSubset& data_found
) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_load_begin(memberid, strm, timestamp, data_found);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_load_end(int groupid, int memberid) {
  FENIX_CPP_API_BEGIN
  find_group(groupid)->member_load_end(memberid);
  return FENIX_SUCCESS;
  FENIX_CPP_API_END
}

int member_restore(
  int groupid, int memberid, void* data, int maxcount, int timestamp,
  DataSubset& data_found
) {
  FENIX_CPP_API_BEGIN
  auto g     = find_group(groupid);
  data_found = {};
  return find_group(groupid)->member_restore(
    memberid, data, maxcount, timestamp, data_found
  );
  FENIX_CPP_API_END
}

int member_lrestore(
  int groupid, int memberid, void* data, int maxcount, int timestamp,
  DataSubset& data_found
) {
  FENIX_CPP_API_BEGIN
  data_found = {};
  return find_group(groupid)->member_lrestore(
    memberid, data, maxcount, timestamp, data_found
  );
  FENIX_CPP_API_END
}

std::optional<std::vector<int>> group_members(int group_id) {
  assert(initialized());
  auto group = search_group(group_id);
  if (group) return group->get_member_ids();
  return {};
}

std::optional<std::vector<int>> group_snapshots(int group_id) {
  assert(initialized());
  auto group = search_group(group_id);
  if (group) return group->get_snapshots();
  return {};
}

int snapshot_delete(int group_id, int time_stamp) {
  FENIX_CPP_API_BEGIN
  if (time_stamp < 0) FENIX_THROW(FENIX_ERROR_INVALID_TIMESTAMP);
  return find_group(group_id)->snapshot_delete(time_stamp);
  FENIX_CPP_API_END
}

int member_delete(int groupid, int memberid) {
  FENIX_CPP_API_BEGIN
  return find_group(groupid)->member_delete(memberid);
  FENIX_CPP_API_END
}

int group_delete(int groupid) {
  FENIX_CPP_API_BEGIN
  int ret = find_group(groupid)->group_delete();
  if (ret == FENIX_SUCCESS) {
    ret = __fenix_data_recovery_remove_group(groupid);
  }
  return ret;
  FENIX_CPP_API_END
}

} // namespace fenix::data

using namespace fenix;

int Fenix_Data_group_get_number_of_snapshots(int group_id, int* num_snapshots) {
  FENIX_C_API_BEGIN
  return find_group(group_id)->get_number_of_snapshots(num_snapshots);
  FENIX_C_API_END
}

int Fenix_Data_group_get_snapshot_at_position(
  int groupid, int position, int* timestamp
) {
  FENIX_C_API_BEGIN
  return find_group(groupid)->get_snapshot_at_position(position, timestamp);
  FENIX_C_API_END
}

int Fenix_Data_member_attr_get(
  int groupid, int memberid, int attributename, void* attributevalue, int* flag,
  int sourcerank
) {
  FENIX_C_API_BEGIN
  auto g = find_group(groupid);
  return g->member_get_attribute(
    g->find_member(memberid), attributename, attributevalue, flag, sourcerank
  );
  FENIX_C_API_END
}

int Fenix_Data_group_get_redundancy_policy(
  int groupid, int* policy_name, int* policy_value, int* flag
) {
  FENIX_C_API_BEGIN
  return find_group(groupid)->get_redundant_policy(
    policy_name, policy_value, flag
  );
  FENIX_C_API_END
}

int Fenix_Data_member_restore_from_rank(
  int groupid, int memberid, void* target_buffer, int max_count, int time_stamp,
  int source_rank
) {
  FENIX_C_API_BEGIN
  fenix::util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  fatal_print("unimplemented");
  return find_group(groupid)->member_restore_from_rank(
    memberid, target_buffer, max_count, time_stamp, source_rank
  );
  FENIX_C_API_END
}

int Fenix_Data_group_get_number_of_members(int group_id, int* num_members) {
  FENIX_C_API_BEGIN
  *num_members = find_group(group_id)->members.size();
  return FENIX_SUCCESS;
  FENIX_C_API_END
}

int Fenix_Data_group_get_member_at_position(
  int group_id, int* member_id, int position
) {
  FENIX_C_API_BEGIN
  auto group = find_group(group_id);
  if (position < 0 || position >= group->members.size()) {
    FENIX_THROW(FENIX_ERROR_INVALID_POSITION);
  }
  auto iter = group->members.begin();
  std::advance(iter, position);
  *member_id = iter->first;
  return FENIX_SUCCESS;
  FENIX_C_API_END
}

int Fenix_Data_wait(Fenix_Request request) {
  FENIX_C_API_BEGIN
  fenix::util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  fatal_print("unimplemented");
  return FENIX_SUCCESS;
  FENIX_C_API_END
}

int Fenix_Data_test(Fenix_Request request, int* flag) {
  FENIX_C_API_BEGIN
  fenix::util::ScopedActiveMlog active_mlog(FENIX_MLOG_NONE);
  fatal_print("unimplemented");
  return FENIX_SUCCESS;
  FENIX_C_API_END
}
