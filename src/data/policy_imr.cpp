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

#include <algorithm>
#include <vector>
#include <deque>
#include <map>
#include <sstream>
#include <set>
#include <cstring>
#include <utility>
#include <limits>

#include <mpi.h>
#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif

#include "fenix.h"
#include "fenix_ext.hpp"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"
#include "fenix_exception.hpp"
#include "fenix/data/subset.hpp"
#include "fenix/data/group.hpp"
#include "fenix/data/member.hpp"
#include "fenix/data/policy_imr.hpp"
#include "fenix/mpixx/tasks.hpp"

namespace fenix::data::imr {

using util::DataBuffer;

IMRSnapshot::IMRSnapshot(int size, int max_count)
  : DataSnapshot(size, max_count), partner(size, max_count) {}

void IMRSnapshot::init_cohort(MPI_Comm cohort_comm) {
  DataSnapshot::init_cohort(cohort_comm);
  partner.init_cohort(cohort_comm);
}

void IMRSnapshot::reinit_cohort(MPI_Comm cohort_comm) {
  DataSnapshot::reinit_cohort(cohort_comm);
  partner.reinit_cohort(cohort_comm);
}

// Helper to access IMR Entry from base DataSnapshot
static IMRSnapshot& get_entry(DataSnapshot& snap) {
  return *static_cast<IMRSnapshot*>(&snap);
}

static IMRSnapshot& get_entry(std::unique_ptr<DataSnapshot>& snap) {
  return *static_cast<IMRSnapshot*>(snap.get());
}

BuddyMember::BuddyMember(DataMember&& member, IMRGroup& my_group)
  : DataMember(std::move(member)), send_buf(my_group.send_buf),
    recv_buf(my_group.recv_buf) {
  // Initialize snapshots (creates IMRSnapshot objects via virtual
  // create_snapshot)
  init_snapshots();

  if (user_data.is_bounded()) {
    // Resize partner buffers for staging snapshot
    get_entry(*stage_snapshot_).partner.resize(user_data.size());

    // Resize partner buffers for available snapshots
    for (auto& snap : avail_snapshots_) {
      get_entry(*snap).partner.resize(user_data.size());
    }
  }
}

ParityMember::ParityMember(DataMember&& member, IMRGroup& my_group)
  : DataMember(std::move(member)), send_buf(my_group.send_buf),
    recv_buf(my_group.recv_buf) {
  // Initialize snapshots (creates IMRSnapshot objects via virtual
  // create_snapshot)
  init_snapshots();

  int data_len   = user_data.is_bounded() ? user_data.size() : 0;
  int parity_len = data_len / (group->cohort_size - 1);

  int remainder = data_len % (group->cohort_size - 1);
  if (remainder) remainder++;
  if (remainder < group->cohort_rank) parity_len++;

  // Resize partner buffers for staging snapshot (parity_len is in bytes)
  get_entry(*stage_snapshot_).partner.resize(parity_len);

  // Resize partner buffers for available snapshots
  for (auto& snap : avail_snapshots_) {
    get_entry(*snap).partner.resize(parity_len);
  }
}

tasks::Task<int> BuddyMember::iprotect() {
  const int rank  = group->cohort_rank;
  const int left  = rank == 0 ? group->cohort_size - 1 : rank - 1;
  const int right = rank == group->cohort_size - 1 ? 0 : rank + 1;

  IMRSnapshot& snap = *static_cast<IMRSnapshot*>(stage_snapshot_.get());

  const DataSubset& subset         = snap.staged_subset();
  const DataSubset& partner_subset = snap.staged_subsets[left];

  snap.partner.add_and_fit(partner_subset);

  // Create datatypes for direct send/recv without serialization
  auto send_type = subset.to_datatype(datatype_);
  auto recv_type = partner_subset.to_datatype(datatype_);

  co_await mpixx::sendrecv(
    snap.data(), 1, send_type, right, 0,
    snap.partner.data(), 1, recv_type, left, 0,
    group->cohort_comm
  );

  for (int i = 0; i < snap.staged_subsets.size(); i++) {
    snap.protected_subsets[i] += snap.staged_subsets[i];
    snap.staged_subsets[i] = {};
  }

  co_return FENIX_SUCCESS;
}

std::vector<int> ParityMember::prepare_for_parity(IMRSnapshot& snap) {
  size_t data_count = 0;
  for (const auto& subset : snap.staged_subsets) {
    data_count = std::max(data_count, subset.max_count());
  }
  for (const auto& subset : snap.protected_subsets) {
    data_count = std::max(data_count, subset.max_count());
  }

  int data_bytes = data_count * snap.elm_size();
  int n_partners = snap.staged_subsets.size() - 1;

  int parity_bytes    = data_bytes / n_partners;
  int remainder_bytes = data_bytes % n_partners;

  // If we have any remainder, we need one extra remainder, since the ranks that
  // calculate the 1-larger parity are missing one byte of protection.
  if (remainder_bytes) remainder_bytes++;

  std::vector<int> ret(n_partners + 1, parity_bytes);
  for (int i = 0; i < remainder_bytes; i++) ++ret[i];
  int local_parity = ret[group->cohort_rank];

  if (snap.size() < data_bytes) snap.resize(data_bytes);
  snap.partner.resize(local_parity);

  return ret;
}

tasks::Task<int> ParityMember::iprotect() {
  IMRSnapshot& snap = *static_cast<IMRSnapshot*>(stage_snapshot_.get());
  auto parity_bytes = prepare_for_parity(snap);

  fenix_assert(parity_bytes.size() == group->cohort_size);

  //Zero out the parity data before computing, so old data doesn't contribute
  std::memset(snap.partner.data(), 0, snap.partner.size());

  int offset = 0;
  for (int root = 0; root < parity_bytes.size(); root++) {
    int len = parity_bytes[root];

    bool local_root = group->cohort_rank == root;

    char* input;
    if (local_root) {
      input = snap.partner.data();
    } else {
      input = snap.data() + offset;
      offset += len;
      if (input + len > snap.data() + snap.size()) {
        fenix_assert(input + len == snap.data() + snap.size() + 1);
        input--;
        offset--;
      }
    }

    co_await mpixx::reduce(
      local_root ? MPI_IN_PLACE : input, input, len, MPI_BYTE, MPI_BXOR, root,
      group->cohort_comm
    );
  }

  for (int i = 0; i < snap.staged_subsets.size(); i++) {
    snap.protected_subsets[i] += snap.staged_subsets[i];
    snap.staged_subsets[i] = {};
  }
  co_return FENIX_SUCCESS;
}

void BuddyMember::repair() {
  //My partner ranks (within set_comm)
  const int rank  = group->cohort_rank;
  const int left  = rank == 0 ? group->cohort_size - 1 : rank - 1;
  const int right = rank == group->cohort_size - 1 ? 0 : rank + 1;

  //Data on which partners have found each snapshot
  int found[3];
  int& found_here  = found[rank];
  int& found_left  = found[left];
  int& found_right = found[right];

  // Reinitialize cohort in stage snapshot
  stage_snapshot_->reinit_cohort(group->cohort_comm);

  // Iterate through committed timestamps (newest to oldest)
  for (const int& ts : group->timestamps) {
    auto snap_it = commit_snapshots_.find(ts);

    found_here = snap_it != commit_snapshots_.end();
    fenix_assert(found_here || !avail_snapshots_.empty());

    DataSnapshot& snap = found_here ? **snap_it : *avail_snapshots_.back();
    IMRSnapshot& e     = get_entry(snap);
    if (!found_here) e.reset();

    MPI_Allgather(
      MPI_IN_PLACE, 1, MPI_INT, found, 1, MPI_INT, group->cohort_comm
    );

    int n_missing = 0, bcast_root = -1;
    for (int i = 0; i < group->cohort_size; i++) {
      if (found[i]) bcast_root = i;
      else n_missing++;
    }

    if (n_missing == 0) continue;
    if (n_missing > 1) {
      if (group->cohort_rank == 0) {
        debug_print(
          "WARNING Fenix_Data_member_restore: %s member %d timestamp %d "
          "unrecoverable",
          group->str().c_str(), memberid, ts
        );
      }
      continue;
    }

    e.reinit_cohort(group->cohort_comm);
    broadcast_subsets(e.protected_subsets, bcast_root).wait();

    if (!found_here) {
      //Fetch my data
      e.add_and_fit(e.protected_subset());
      int m_count = e.protected_subset().count(e.elm_max_count() - 1);
      recv_buf.recv(m_count * e.elm_size(), right, 0, group->cohort_comm)
        .wait();
      e.protected_subset().unpack_data(e.elm_size(), recv_buf, e.buf());

      //Fetch partner's data
      e.partner.add_and_fit(e.protected_subsets[left]);
      int p_count = e.partner.staged_subset().count(e.elm_max_count() - 1);
      recv_buf.recv(p_count * e.elm_size(), left, 0, group->cohort_comm).wait();
      e.partner.staged_subset().unpack_data(
        e.elm_size(), recv_buf, e.partner.buf()
      );

      e.set_timestamp(ts);
      commit_snapshots_.insert(std::move(avail_snapshots_.back()));
      avail_snapshots_.pop_back();
    }
    if (!found_left) {
      //Send their data
      e.partner.staged_subset().pack_data(
        e.elm_size(), e.partner.buf(), send_buf
      );
      send_buf.send(left, 0, group->cohort_comm).wait();
    }
    if (!found_right) {
      //Send my data
      e.protected_subset().pack_data(e.elm_size(), e.buf(), send_buf);
      send_buf.send(right, 0, group->cohort_comm).wait();
    }
  }
}

void ParityMember::repair() {
  //Data on which partners have found each snapshot
  std::vector<int> found;
  found.resize(group->cohort_size);
  int found_here;

  // Reinitialize cohort in stage snapshot
  stage_snapshot_->reinit_cohort(group->cohort_comm);

  // Iterate through committed timestamps (newest to oldest)
  for (const int& ts : group->timestamps) {
    auto snap_it = commit_snapshots_.find(ts);

    found_here = snap_it != commit_snapshots_.end();
    fenix_assert(found_here || !avail_snapshots_.empty());

    DataSnapshot& snap = found_here ? **snap_it : *avail_snapshots_.back();
    IMRSnapshot& e     = get_entry(snap);
    if (!found_here) e.reset();

    MPI_Allgather(
      &found_here, 1, MPI_INT, found.data(), 1, MPI_INT, group->cohort_comm
    );

    int recovering = -1;
    for (int i = 0; i < group->cohort_size; i++) {
      if (found[i]) continue;
      if (recovering != -1) {
        if (group->cohort_rank == 0) {
          debug_print(
            "WARNING Fenix_Data_member_restore: %s member %d timestamp %d "
            "unrecoverable",
            group->str().c_str(), memberid, ts
          );
        }
        recovering = -1;
        break;
      } else {
        recovering = i;
      }
    }
    if (recovering == -1) continue;
    e.reinit_cohort(group->cohort_comm);

    // Broadcast protected_subsets from a sender who has the data
    int sender = recovering == 0 ? 1 : 0;
    broadcast_subsets(e.protected_subsets, sender).wait();

    auto parity_bytes = prepare_for_parity(e);

    if (!found_here) {
      std::memset(e.data(), 0, e.size());
      std::memset(e.partner.data(), 0, e.partner.size());
    }

    int offset = 0;
    for (int i = 0; i < parity_bytes.size(); i++) {
      int len = parity_bytes[i];

      char* input;
      if (group->cohort_rank == i) {
        input = e.partner.data();
      } else {
        input = e.data() + offset;
        offset += len;
        if (input + len > snap.data() + snap.size()) {
          fenix_require(input + len == snap.data() + snap.size() + 1);
          input--;
          offset--;
        }
      }
      MPI_Reduce(
        found_here ? input : MPI_IN_PLACE, input, len, MPI_BYTE, MPI_BXOR,
        recovering, group->cohort_comm
      );
    }

    if (!found_here) {
      e.set_timestamp(ts);
      commit_snapshots_.insert(std::move(avail_snapshots_.back()));
      avail_snapshots_.pop_back();
    }
  }
}

int IMRGroup::get_mode(int* policy_vals) {
  return policy_vals ? policy_vals[0] : 1;
}

int IMRGroup::get_rank_sep(int* policy_vals, MPI_Comm comm) {
  return policy_vals ? policy_vals[1] : __fenix_get_world_size(comm) / 2;
}

MPI_Group IMRGroup::create_cohort() {
  int mode_val     = mode;
  int rank_sep_val = rank_separation;

  int my_rank, comm_size;
  MPI_Comm_size(comm, &comm_size);
  MPI_Comm_rank(comm, &my_rank);

  std::set<int> partner_set;
  partner_set.insert(my_rank);

  if (mode_val == 1) {
    //odd-sized groups take some extra handling.
    bool isOdd = ((comm_size % 2) != 0);

    int remaining_size = comm_size;
    if (isOdd) remaining_size -= 3;

    //We want to form groups of rank_sep_val*2 to pair within
    int n_full_groups = remaining_size / (rank_sep_val * 2);

    //We don't always get what we want though, one group may need to be
    //smaller.
    int mini_group_size =
      (remaining_size - n_full_groups * rank_sep_val * 2) / 2;

    int start_rank = mini_group_size + (isOdd ? 1 : 0);
    int mid_rank   = comm_size / 2; //Only used when isOdd

    int end_mini_group_start   = comm_size - mini_group_size - (isOdd ? 1 : 0);
    int start_mini_group_start = (isOdd ? 1 : 0);
    bool in_start_mini         = my_rank >= start_mini_group_start &&
      my_rank < start_mini_group_start + mini_group_size;
    bool in_end_mini =
      my_rank >= end_mini_group_start && my_rank < comm_size - (isOdd ? 1 : 0);

    //Allocate the "normal" ranks
    if (my_rank >= start_rank && my_rank < end_mini_group_start &&
        (!isOdd || my_rank != mid_rank)) {
      //"effective" rank for determining which group I'm in and if I look
      //forward or backward for a partner.
      int e_rank = my_rank - start_rank;
      if (isOdd && my_rank > mid_rank) --e_rank; //Skip middle rank when isOdd

      int my_partner;
      if (((e_rank / rank_sep_val) % 2) == 0) {
        //Look forward for partner.
        my_partner = my_rank + rank_sep_val;
        if (isOdd && my_rank < mid_rank && my_partner >= mid_rank) ++my_partner;
      } else {
        my_partner = my_rank - rank_sep_val;
        if (isOdd && my_rank > mid_rank && my_partner <= mid_rank) --my_partner;
      }

      partner_set.insert(my_partner);
    } else if (in_start_mini) {
      int e_rank  = my_rank - start_mini_group_start;
      int partner = end_mini_group_start + e_rank;
      partner_set.insert(partner);
    } else if (in_end_mini) {
      int e_rank  = my_rank - end_mini_group_start;
      int partner = start_mini_group_start + e_rank;
      partner_set.insert(partner);
    } else {
      //Only things left are the three ranks that must be paired to handle
      //odd-sized comms
      partner_set.insert({0, mid_rank, comm_size - 1});

      //my_rank should be one of the inserted ranks, or something in the
      //logic here is broken.
      fenix_assert(
        (partner_set.size() == 3 || (comm_size == 1 && partner_set.size() == 1))
      );
    }
  } else if (mode_val == 5) {
    int set_size_val = set_size_policy;

    //User is responsible for giving values that "make sense" for set size and
    //rank separation given a comm size.
    int my_set_pos = (my_rank / rank_sep_val) % set_size_val;
    for (int i = 0; i < set_size_val; i++) {
      int partner =
        (comm_size + my_rank - rank_sep_val * (my_set_pos - i)) % comm_size;
      partner_set.insert(partner);
    }
  }

  // Create cohort group from computed partners
  std::vector<int> partner_vec(partner_set.begin(), partner_set.end());
  MPI_Group comm_group, cohort_group;
  MPI_Comm_group(comm, &comm_group);
  MPI_Group_incl(
    comm_group, partner_vec.size(), partner_vec.data(), &cohort_group
  );
  MPI_Group_free(&comm_group);
  return cohort_group;
}

IMRGroup::IMRGroup(
  int m_id, MPI_Comm m_comm, int m_timestart, int m_depth, int* policy_vals
)
  : DataGroup(m_id, m_comm, m_timestart, m_depth, FENIX_DATA_POLICY_IMR),
    mode(get_mode(policy_vals)),
    rank_separation(get_rank_sep(policy_vals, m_comm)),
    set_size_policy(policy_vals && mode == 5 ? policy_vals[2] : 0) {}

void IMRGroup::emplace_member(DataMember&& member) {
  // Create and insert the policy-specific Member into members map
  std::shared_ptr<DataMember> m;
  if (mode == 1) {
    m = std::make_shared<BuddyMember>(std::move(member), *this);
  } else if (mode == 5) {
    m = std::make_shared<ParityMember>(std::move(member), *this);
  } else {
    assert(false);
  }
  members.insert(m);
}

void IMRGroup::commit() {
  DataGroup::commit();
  send_buf.clear();
  send_buf.shrink_to_fit();
  recv_buf.clear();
  recv_buf.shrink_to_fit();
}

void IMRGroup::member_repair(int member_id) {
  // Call parent to handle member repair across cohort
  DataGroup::member_repair(member_id);

  // Clear IMR-specific buffers
  send_buf.clear();
  send_buf.shrink_to_fit();
  recv_buf.clear();
  recv_buf.shrink_to_fit();
}

void IMRGroup::member_restore_from_rank(
  int member_id, void* target_buffer, int max_count, int timestamp,
  int source_rank
) {
  fenix_assert(false, "restore_from_rank is not supported yet!");
}

void IMRGroup::init() {
  // Call parent to create cohort and cohort_comm (includes sync_timestamps)
  DataGroup::init();

  // Validate parity mode requirements
  if (mode == 5 && cohort_size < 3) {
    if (cohort_rank == 0) {
      debug_print(
        "ERROR: Parity mode (mode 5) requires cohort_size >= 3, but got %d",
        cohort_size
      );
    }
    FENIX_THROW(FENIX_ERROR_INVALID_POLICY_VALUE);
  }
}

void IMRGroup::get_redundant_policy(int* policy_name, void* policy_value) {
  *policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID;

  int* policy_vals = (int*)policy_value;
  policy_vals[0]   = mode;
  policy_vals[1]   = rank_separation;
  if (mode == 5) policy_vals[2] = set_size_policy;
}

} // namespace fenix::data::imr
