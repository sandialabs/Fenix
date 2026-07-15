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

#include <cassert>

#include "mpi.h"
#include "fenix-config.h"
#include "fenix_ext.hpp"
#include "fenix_util.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"
#include "fenix/mpi_util.hpp"

namespace fenix::data {

fenix_group_t* search_group(int id) {
  auto dr = fenix_rt.data_recovery;
  for (int i = 0; i < dr->count; i++) {
    auto group = dr->group[i];
    if (dr->group[i]->groupid == id) {
      return dr->group[i];
    }
  }
  return nullptr;
}

fenix_group_t* find_group(int id, std::source_location loc) {
  auto group = search_group(id);
  if (!group) FENIX_THROW_FROM(FENIX_ERROR_INVALID_GROUPID, loc);
  return group;
}

fenix_group_t::fenix_group_t(
  int m_groupid, MPI_Comm m_comm, int m_timestart, int m_depth, int m_policy
) {
  groupid      = m_groupid;
  comm         = m_comm;
  comm_size    = util::comm_size(comm);
  current_rank = util::comm_rank(comm);
  timestart    = m_timestart;
  timestamp    = -1;
  depth        = m_depth;
  policy_name  = m_policy;
}

fenix_member_entry_t* fenix_group_t::search_member(int id) {
  auto iter = members.find(id);
  if (iter == members.end()) return nullptr;
  assert(iter->first == iter->second.memberid);
  return &(iter->second);
}

fenix_member_entry_t* fenix_group_t::find_member(
  int id, std::source_location loc
) {
  auto member = search_member(id);
  if (!member) FENIX_THROW_FROM(FENIX_ERROR_INVALID_MEMBERID, loc);
  return member;
}

int fenix_group_t::member_create(
  int id, void* data, int count, MPI_Datatype datatype
) {
  auto [iter, emplaced] = members.try_emplace(id, id, data, count, datatype);
  if (!emplaced) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);
  member_order.push_back(id);
  return this->member_create(&iter->second);
}

int fenix_group_t::member_create(
  int id, void* data, int count, MPI_Datatype datatype, SerializeFileFunc& s
) {
  auto [iter, emplaced] = members.try_emplace(id, id, data, count, datatype, s);
  if (!emplaced) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);
  member_order.push_back(id);
  return this->member_create(&iter->second);
}

int fenix_group_t::member_create(
  int id, void* data, int count, int datatype_size
) {
  auto [iter, emplaced] =
    members.try_emplace(id, id, data, count, datatype_size);
  if (!emplaced) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);
  member_order.push_back(id);
  return this->member_create(&iter->second);
}

int fenix_group_t::member_delete(int id) {
  auto member = find_member(id);
  int ret     = this->member_delete(member);
  if (ret == FENIX_SUCCESS) {
    members.erase(id);
    for (int i = 0; i < member_order.size(); i++) {
      if (member_order[i] == id) {
        member_order.erase(member_order.begin() + i);
        break;
      }
    }
  }
  return ret;
}

std::vector<int> fenix_group_t::get_member_ids() { return member_order; }

fenix_data_recovery_t* __fenix_data_recovery_init() {
  fenix_data_recovery_t* data_recovery =
    (fenix_data_recovery_t*)s_calloc(1, sizeof(fenix_data_recovery_t));

  data_recovery->count      = 0;
  data_recovery->total_size = __FENIX_DEFAULT_GROUP_SIZE;

  data_recovery->group = (fenix_group_t**)s_malloc(
    __FENIX_DEFAULT_GROUP_SIZE * sizeof(fenix_group_t*)
  );

  if (fenix_rt.options.verbose == 41) {
    verbose_print(
      "c-rank: %d, role: %d, g-count: %zu, g-size: %zu\n",
      __fenix_get_current_rank(fenix_rt.new_world), fenix_rt.role,
      data_recovery->count, data_recovery->total_size
    );
  }

  return data_recovery;
}

int __fenix_group_delete_direct(fenix_group_t* group) {
  //We need this function to remove any allocated pointers
  //that the fenix core adds before called the policy-specific
  //group delete. This lets us update fenix core's group struct
  //without having to potentially update the deletion process
  //for each and every policy.
  //
  //We'll leave responsibility for calling
  //__fenix_data_member_destroy()
  //to the policy, as it may be using that as a reference for
  //knowing how many members there are during its own deletion
  //process.

  return group->group_delete();
}

int __fenix_data_recovery_remove_group(int groupid) {
  auto dr = fenix_rt.data_recovery;

  bool found = false;
  for (int i = 0; i < dr->count; i++) {
    if (dr->group[i] && dr->group[i]->groupid == groupid) {
      found = true;
    }
    if (found) {
      if (i < dr->count - 1) dr->group[i] = dr->group[i + 1];
      else dr->group[i] = nullptr;
    }
  }
  if (!found) FENIX_THROW(FENIX_ERROR_INVALID_GROUPID);
  dr->count--;

  return FENIX_SUCCESS;
}

void __fenix_data_recovery_destroy(fenix_data_recovery_t* data_recovery) {
  int group_index;
  for (group_index = 0; group_index < data_recovery->count; group_index++) {
    fenix_group_t* group = (data_recovery->group[group_index]);

    //Specific data policy function frees any data policy constructs
    __fenix_group_delete_direct(group);
  }
  free(data_recovery->group);
  free(data_recovery);
}

void __fenix_ensure_data_recovery_capacity(
  fenix_data_recovery_t* data_recovery
) {
  //If we're ensuring there is space for a new group, we need to check that
  //count+1 is < size
  if (data_recovery->count + 1 >= data_recovery->total_size) {
    int start_index      = data_recovery->total_size;
    data_recovery->group = (fenix_group_t**)s_realloc(
      data_recovery->group,
      (data_recovery->total_size * 2) * sizeof(fenix_group_t*)
    );
    data_recovery->total_size = data_recovery->total_size * 2;

    if (fenix_rt.options.verbose == 51) {
      verbose_print(
        "g-count: %zu, g-size: %zu\n", data_recovery->count,
        data_recovery->total_size
      );
    }
  }
}

int __fenix_search_groupid(int key, fenix_data_recovery_t* data_recovery) {
  int group_index, found = -1, index = -1;
  for (group_index = 0; (found != 1) && (group_index < data_recovery->count);
       group_index++) {
    fenix_group_t* group = (data_recovery->group[group_index]);
    if (key == group->groupid) {
      index = group_index;
      found = 1;
    }
  }
  return index;
}

int __fenix_find_next_group_position(fenix_data_recovery_t* data_recovery) {
  //Ensure that we have space.
  __fenix_ensure_data_recovery_capacity(data_recovery);
  return data_recovery->count;
}

} //end namespace fenix::data
