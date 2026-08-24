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
#include <algorithm>

#include "mpi.h"
#include "fenix-config.h"
#include "fenix_ext.hpp"
#include "fenix_util.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"
#include "fenix/mpi_util.hpp"

namespace fenix::data {

DataGroup::DataGroup(
  int m_groupid, MPI_Comm m_comm, int m_timestart, int m_depth, int m_policy
) {
  groupid      = m_groupid;
  comm         = m_comm;
  comm_size    = fenix::util::comm_size(comm);
  current_rank = fenix::util::comm_rank(comm);
  timestart    = m_timestart;
  timestamp    = -1;
  depth        = m_depth;
  policy_name  = m_policy;
}

DataMember* DataGroup::search_member(int id) {
  auto iter = members.find(id);
  if (iter == members.end()) return nullptr;
  assert((*iter)->memberid == id);
  return iter->get();
}

DataMember* DataGroup::find_member(int id, std::source_location loc) {
  auto member = search_member(id);
  if (!member) FENIX_THROW_FROM(FENIX_ERROR_INVALID_MEMBERID, loc);
  return member;
}

// Create a member with MPI_Datatype
// Creates temporary base entry, calls emplace_member to replace with policy
// type, then calls virtual member_create for initialization
void DataGroup::member_create(
  int id, void* data, int count, MPI_Datatype datatype
) {
  if (members.find(id) != members.end()) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);

  // Let policy replace with its specific Member type
  this->emplace_member({id, data, count, datatype, depth});

  auto iter = members.find(id);
  fenix_assert(iter != members.end(), "emplace_member failed");

  member_order.push_back(id);
  (*iter)->init_snapshots();
}

// Create a member with MPI_Datatype and custom serialization function
void DataGroup::member_create(
  int id, void* data, int count, MPI_Datatype datatype, SerializeFunc& s
) {
  if (members.find(id) != members.end()) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);

  this->emplace_member({id, data, count, datatype, depth, s});

  auto iter = members.find(id);
  fenix_assert(iter != members.end(), "emplace_member failed");

  member_order.push_back(id);
  (*iter)->init_snapshots();
}

// Create a member with explicit datatype size
void DataGroup::member_create(
  int id, void* data, int count, int datatype_size
) {
  if (members.find(id) != members.end()) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);

  // Let policy replace with its specific Member type
  this->emplace_member({id, data, count, datatype_size, depth});

  auto iter = members.find(id);
  fenix_assert(iter != members.end(), "emplace_member failed");

  member_order.push_back(id);
  (*iter)->init_snapshots();
}

void DataGroup::emplace_member(DataMember&& member) {
  members.insert(std::make_shared<DataMember>(std::move(member)));
}

void DataGroup::member_delete(int id) {
  auto iter = members.find(id);
  if (iter == members.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  members.erase(iter);
  for (int i = 0; i < member_order.size(); i++) {
    if (member_order[i] == id) {
      member_order.erase(member_order.begin() + i);
      break;
    }
  }
}

std::vector<int> DataGroup::get_member_ids() { return member_order; }

void DataGroup::commit() {
  if (timestamps.size() == depth + 1) {
    // Remove oldest timestamp (last in reverse-sorted set)
    timestamps.erase(--timestamps.end());
  }
  timestamps.insert(timestamp);
  for (auto& member : members) member->commit(timestamp);
}

void DataGroup::snapshot_delete(int ts) {
  if (timestamps.empty()) {
    FENIX_THROW(FENIX_ERROR_INVALID_TIMESTAMP);
  } else if (ts == FENIX_DATA_SNAPSHOT_ALL) {
    while (!timestamps.empty()) this->snapshot_delete(*timestamps.begin());
  } else {
    if (ts == FENIX_DATA_SNAPSHOT_LATEST) ts = *timestamps.begin();

    auto it = timestamps.find(ts);
    if (it == timestamps.end()) FENIX_THROW(FENIX_ERROR_INVALID_TIMESTAMP);
    timestamps.erase(it);

    for (auto& member : members) member->snapshot_delete(ts);
  }
}

int DataGroup::get_number_of_snapshots() { return timestamps.size(); }

int DataGroup::get_snapshot_at_position(int position) {
  if (position < 0 || position >= timestamps.size())
    FENIX_THROW(FENIX_ERROR_INVALID_POSITION);
  auto it = timestamps.begin();
  std::advance(it, position);
  return *it;
}

std::vector<int> DataGroup::get_snapshots() {
  return {timestamps.begin(), timestamps.end()};
}

} //end namespace fenix::data
