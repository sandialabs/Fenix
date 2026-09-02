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
#include <sstream>

#include "mpi.h"
#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif
#include "fenix-config.h"
#include "fenix_ext.hpp"
#include "fenix_util.hpp"
#include "fenix/data/group.hpp"
#include "fenix/data/member.hpp"
#include "fenix/mpixx/datatype.hpp"
#include "fenix/mpixx/util.hpp"

namespace fenix::data {

DataGroup::DataGroup(
  int m_groupid, MPI_Comm m_comm, int m_timestart, int m_depth, int m_policy
) {
  groupid      = m_groupid;
  comm         = m_comm;
  comm_size    = comm.size();
  current_rank = comm.rank();
  timestart    = m_timestart;
  timestamp    = -1;
  depth        = m_depth;
  policy_name  = m_policy;
}

DataGroup::~DataGroup() {
  if (cohort != MPI_GROUP_NULL) {
    MPI_Group_free(&cohort);
  }
  // cohort_comm is automatically freed by mpixx::Comm destructor
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
  this->emplace_member({*this, id, data, count, datatype, depth});

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

  this->emplace_member({*this, id, data, count, datatype, depth, s});

  auto iter = members.find(id);
  fenix_assert(iter != members.end(), "emplace_member failed");

  member_order.push_back(id);
  (*iter)->init_snapshots();
}

// Create a member from serialized data
void DataGroup::member_create(const util::DataBuffer& serialized) {
  // Create the member first to get its ID
  DataMember member{*this, serialized, depth};
  int id = member.memberid;

  if (members.find(id) != members.end()) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);

  // Let policy replace with its specific Member type
  this->emplace_member(std::move(member));

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
  // Check that all members have stored their data
  for (auto& member : members) {
    if (member->has_unstored_data()) {
      FENIX_THROW(FENIX_ERROR_MEMBER_UNSTORED);
    }
  }

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

void DataGroup::revoke() {
  if (cohort_comm) {
    cohort_comm.revoke();
    cohort_comm.free();
  }
}

std::string DataGroup::str() {
  // Extract partners from cohort group
  int cohort_size;
  MPI_Group_size(cohort, &cohort_size);

  std::vector<int> cohort_ranks(cohort_size);
  for (int i = 0; i < cohort_size; i++) cohort_ranks[i] = i;

  std::vector<int> partners(cohort_size);
  MPI_Group comm_group;
  MPI_Comm_group(comm, &comm_group);
  MPI_Group_translate_ranks(
    cohort, cohort_size, cohort_ranks.data(), comm_group, partners.data()
  );
  MPI_Group_free(&comm_group);

  std::stringstream ss;
  ss << "Group " << groupid << " set ";
  ss << "[" << partners[0];
  for (int i = 1; i < partners.size(); i++) ss << ", " << partners[i];
  ss << "]";
  return ss.str();
}

void DataGroup::sync_timestamps() {
  fenix_assert(cohort_comm, "sync_timestamps called with invalid cohort_comm");

  int n_snapshots = timestamps.size();
  MPI_Allreduce(MPI_IN_PLACE, &n_snapshots, 1, MPI_INT, MPI_MAX, cohort_comm);

  // Create vector with current timestamps, pad with -1 if needed
  std::vector<int> ts(timestamps.begin(), timestamps.end());
  ts.resize(n_snapshots, -1);

  MPI_Allreduce(
    MPI_IN_PLACE, ts.data(), n_snapshots, MPI_INT, MPI_MAX, cohort_comm
  );

  // Rebuild set from vector (automatically reverse sorted)
  timestamps.clear();
  for (int t : ts) {
    timestamps.insert(t);
  }

  if (!timestamps.empty()) timestamp = *timestamps.begin(); // Newest first
  else timestamp = -1;

  // Remove committed snapshots from members that are not in synced timestamps
  for (auto& member : members) {
    member->cleanup_timestamps(timestamps);
  }
}

void DataGroup::member_repair(int member_id) {
  fenix_assert(cohort_comm, "member_repair called with invalid cohort_comm");

  DataMember* member = search_member(member_id);

  // Query cohort size and rank (using cached values)
  int cohort_size = this->cohort_size;
  int cohort_rank = this->cohort_rank;

  // Gather which cohort members have this member
  std::vector<int> found_members(cohort_size);
  found_members[cohort_rank] = member ? 1 : 0;

  MPI_Allgather(
    MPI_IN_PLACE, 1, MPI_INT, found_members.data(), 1, MPI_INT, cohort_comm
  );

  // Count missing and find first rank that has it
  int n_missing   = 0;
  int first_found = -1;
  for (int i = 0; i < cohort_size; i++) {
    if (!found_members[i]) {
      n_missing++;
    }
    if (found_members[i] && first_found == -1) {
      first_found = i;
    }
  }

  // Only throw if NO cohort member knows about this member
  if (n_missing == cohort_size) {
    if (cohort_rank == 0) {
      debug_print(
        "ERROR Group %d member_id %d not found in any cohort member\n", groupid,
        member_id
      );
    }
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  }

  // If any rank is missing the member, broadcast the member metadata
  if (n_missing > 0) {
    util::DataBuffer metadata_buf;

    if (cohort_rank == first_found) {
      metadata_buf = member->serialize();
    }

    // Broadcast metadata buffer size
    int metadata_size = static_cast<int>(metadata_buf.size());
    MPI_Bcast(&metadata_size, 1, MPI_INT, first_found, cohort_comm);

    // Broadcast metadata
    if (cohort_rank != first_found) {
      metadata_buf.resize(metadata_size);
    }
    MPI_Bcast(metadata_buf.data(), metadata_size, MPI_BYTE, first_found, cohort_comm);

    // If I'm missing it, create it now
    if (!found_members[cohort_rank]) {
      member_create(metadata_buf);
      member = find_member(member_id);
    }
  }

  // Let the member handle policy-specific repair
  member->repair();
}

} //end namespace fenix::data
