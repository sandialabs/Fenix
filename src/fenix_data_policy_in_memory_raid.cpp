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
#include "fenix_data_subset.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"
#include "fenix_data_policy_in_memory_raid.hpp"
#include "fenix/tasks/mpi.hpp"

namespace fenix::data::imr {

Entry::Entry(int size, int max_count)
  : Snapshot(size, max_count), partner(size, max_count) {}

// Helper to access IMR Entry from base Snapshot
static Entry& get_entry(Snapshot& snap) { return *static_cast<Entry*>(&snap); }

static Entry& get_entry(std::unique_ptr<Snapshot>& snap) {
  return *static_cast<Entry*>(snap.get());
}

Member::Member(fenix_member_entry_t&& my_mentry, Group& my_group)
  : fenix_member_entry_t(std::move(my_mentry)), group(my_group),
    send_buf(group.send_buf), recv_buf(group.recv_buf) {}

BuddyMember::BuddyMember(fenix_member_entry_t&& my_mentry, Group& my_group)
  : Member(std::move(my_mentry), my_group) {
  // Initialize snapshots (creates Entry objects via virtual create_snapshot)
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

ParityMember::ParityMember(fenix_member_entry_t&& my_mentry, Group& my_group)
  : Member(std::move(my_mentry), my_group) {
  // Initialize snapshots (creates Entry objects via virtual create_snapshot)
  init_snapshots();

  int data_len   = user_data.is_bounded() ? user_data.size() : 0;
  int parity_len = data_len / (group.set_size - 1);

  int remainder = data_len % (group.set_size - 1);
  if (remainder) remainder++;
  if (remainder < group.set_rank) parity_len++;

  // Resize partner buffers for staging snapshot
  get_entry(*stage_snapshot_).partner.resize(parity_len);

  // Resize partner buffers for available snapshots
  for (auto& snap : avail_snapshots_) {
    get_entry(*snap).partner.resize(parity_len);
  }
}

// Staging functions now use base class default implementations

tasks::Task<int> Member::istorev(const DataSubset& subset) {
  if (subset != SUBSET_PRESTAGED) stage(subset);

  if (subset != SUBSET_PRESTAGED && subset != SUBSET_FULL) {
    return this->istorev_impl(subset);
  } else {
    return this->istorev_impl(get_entry(*stage_snapshot_).region());
  }
}

tasks::Task<int> BuddyMember::exch(
  const DataSubset& subset, const DataSubset& partner_subset
) {
  const int rank  = group.set_rank;
  const int left  = rank == 0 ? group.set_size - 1 : rank - 1;
  const int right = rank == group.set_size - 1 ? 0 : rank + 1;

  Entry& e = get_entry(*stage_snapshot_);
  e.partner.add_and_fit(partner_subset);

  int recv_count = partner_subset.count(e.elm_max_count() - 1);
  recv_buf.reset(e.elm_size() * recv_count);

  subset.pack_data(e.elm_size(), e.buf(), send_buf);
  co_await tasks::mpi::sendrecv(
    send_buf.data(), send_buf.size(), MPI_BYTE, right, 0,
    recv_buf.data(), recv_buf.size(), MPI_BYTE,  left, 0,
    group.set_comm
  );

  partner_subset.unpack_data(e.elm_size(), recv_buf, e.partner.buf());
  co_return FENIX_SUCCESS;
}

tasks::Task<int> BuddyMember::istorev_impl(const DataSubset& subset) {
  //My partner ranks (within set_comm)
  const int rank  = group.set_rank;
  const int left  = rank == 0 ? group.set_size - 1 : rank - 1;
  const int right = rank == group.set_size - 1 ? 0 : rank + 1;

  DataBuffer send_buf, recv_buf;
  subset.serialize(send_buf);

  for (int i = 0; i < group.set_size; i++) {
    if (i == rank) co_await send_buf.send(right, 0, group.set_comm);
    if (i == left) co_await recv_buf.recv_unknown(left, 0, group.set_comm);
  }

  DataSubset partner_subset(recv_buf);
  co_return co_await exch(subset, partner_subset);
}

tasks::Task<int> Member::istore(const DataSubset& subset) {
  if (subset != SUBSET_PRESTAGED) stage(subset);

  if (subset != SUBSET_PRESTAGED && subset != SUBSET_FULL) {
    return this->istore_impl(subset);
  } else {
    return this->istore_impl(get_entry(*stage_snapshot_).region());
  }
}

tasks::Task<int> BuddyMember::istore_impl(const DataSubset& subset) {
  return exch(subset, subset);
}

tasks::Task<int> ParityMember::istore_impl(const DataSubset& subset) {
  Entry& entry = get_entry(*stage_snapshot_);

  int parity_size = entry.size() / (group.set_size - 1);
  int remainder   = entry.size() % (group.set_size - 1);

  //If we have any remainder, treat as if we have one more, since a rank
  //storing a larger parity block wasn't able to store a larger data block, so
  //all such ranks need one extra larger data block.
  if (remainder) remainder++;

  int m_parity_size = parity_size;
  if (group.set_rank < remainder) m_parity_size++;
  entry.partner.resize(m_parity_size);

  //Zero out the parity data before computing, so old data doesn't contribute
  std::memset(entry.partner.data(), 0, entry.partner.size());

  int offset = 0;
  for (int i = 0; i < group.set_size; i++) {
    int len = i < remainder ? parity_size + 1 : parity_size;

    char* input;
    if (group.set_rank == i) {
      //The rank storing this parity region contributes the all zero input
      //as a way of not contributing
      input = entry.partner.data();
    } else {
      if (offset + len > entry.size()) {
        //Since we pretend to have an extra remainder if there is any
        assert(remainder);
        assert(group.set_rank >= remainder);
        assert(offset + len == entry.size() + 1);
        offset--;
      }
      input = entry.data() + offset;
      offset += len;
    }

    co_await tasks::mpi::reduce(
      MPI_IN_PLACE, input, len, MPI_BYTE, MPI_BXOR, i, group.set_comm
    );
  }

  assert(offset == entry.size());
  co_return FENIX_SUCCESS;
}

void Member::repair() {
  // Remove committed snapshots not in group's timestamp list
  for (auto it = commit_snapshots_.begin(); it != commit_snapshots_.end();) {
    auto found = std::find(
      group.timestamps.begin(), group.timestamps.end(), (*it)->timestamp()
    );
    if (found != group.timestamps.end()) {
      ++it;
      continue;
    }

    // Not in group timestamps, extract and move to available pool
    std::unique_ptr<Snapshot> snap =
      std::move(commit_snapshots_.extract(it++).value());
    snap->reset();
    avail_snapshots_.push_back(std::move(snap));
  }

  // Reset the current store buffer entry
  get_entry(*stage_snapshot_).reset();
  this->repair_impl();
}

void BuddyMember::repair_impl() {
  //My partner ranks (within set_comm)
  const int rank  = group.set_rank;
  const int left  = rank == 0 ? group.set_size - 1 : rank - 1;
  const int right = rank == group.set_size - 1 ? 0 : rank + 1;

  //Data on which partners have found each snapshot
  int found[3];
  int& found_here  = found[rank];
  int& found_left  = found[left];
  int& found_right = found[right];

  // Iterate through committed timestamps (newest to oldest)
  for (const int& ts : group.timestamps) {
    auto snap_it = commit_snapshots_.find(ts);

    found_here = snap_it != commit_snapshots_.end();
    fenix_assert(found_here || !avail_snapshots_.empty());

    Snapshot& snap = found_here ? **snap_it : *avail_snapshots_.back();
    Entry& e       = get_entry(snap);

    MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, found, 1, MPI_INT, group.set_comm);

    int n_missing = 0;
    for (int i = 0; i < group.set_size; i++) n_missing += found[i] ? 0 : 1;
    if (n_missing == 0) continue;
    if (n_missing > 1) {
      if (group.set_rank == 0) {
        debug_print(
          "WARNING Fenix_Data_member_restore: %s member %d timestamp %d "
          "unrecoverable",
          group.str().c_str(), id, ts
        );
      }
      continue;
    }

    if (!found_here) {
      //Fetch my data region from right partner
      recv_buf.recv_unknown(right, 0, group.set_comm).wait();
      e.add_and_fit({recv_buf});
      //Fetch my data
      int m_count = e.region().count(e.elm_max_count() - 1);
      recv_buf.recv(m_count * e.elm_size(), right, 0, group.set_comm).wait();
      e.region().unpack_data(e.elm_size(), recv_buf, e.buf());

      //Fetch left partner's region
      recv_buf.recv_unknown(left, 0, group.set_comm).wait();
      e.partner.add_and_fit({recv_buf});
      //Fetch data
      int p_count = e.partner.region().count(e.elm_max_count() - 1);
      recv_buf.recv(p_count * e.elm_size(), left, 0, group.set_comm).wait();
      e.partner.region().unpack_data(e.elm_size(), recv_buf, e.partner.buf());

      e.set_timestamp(ts);
      commit_snapshots_.insert(std::move(avail_snapshots_.back()));
      avail_snapshots_.pop_back();
    }
    if (!found_left) {
      //Send partner's data region
      e.partner.region().serialize(send_buf);
      send_buf.send(left, 0, group.set_comm).wait();
      //Send their data
      e.partner.region().pack_data(e.elm_size(), e.partner.buf(), send_buf);
      send_buf.send(left, 0, group.set_comm).wait();
    }
    if (!found_right) {
      //Send my data region
      e.region().serialize(send_buf);
      send_buf.send(right, 0, group.set_comm).wait();
      //Send my data
      e.region().pack_data(e.elm_size(), e.buf(), send_buf);
      send_buf.send(right, 0, group.set_comm).wait();
    }
  }
}

void ParityMember::repair_impl() {
  //Data on which partners have found each snapshot
  std::vector<int> found;
  found.resize(group.set_size);
  int found_here;

  // Iterate through committed timestamps (newest to oldest)
  for (const int& ts : group.timestamps) {
    auto snap_it = commit_snapshots_.find(ts);

    found_here = snap_it != commit_snapshots_.end();
    fenix_assert(found_here || !avail_snapshots_.empty());

    Snapshot& snap = found_here ? **snap_it : *avail_snapshots_.back();
    Entry& e       = get_entry(snap);

    MPI_Allgather(
      &found_here, 1, MPI_INT, found.data(), 1, MPI_INT, group.set_comm
    );

    int recovering = -1;
    for (int i = 0; i < group.set_size; i++) {
      if (found[i]) continue;
      if (recovering != -1) {
        if (group.set_rank == 0) {
          debug_print(
            "WARNING Fenix_Data_member_restore: %s member %d timestamp %d "
            "unrecoverable",
            group.str().c_str(), id, ts
          );
        }
        recovering = -1;
        break;
      } else {
        recovering = i;
      }
    }
    if (recovering == -1) continue;

    int sender = recovering == 0 ? 1 : 0;
    if (group.set_rank == sender) {
      e.region().serialize(send_buf);
      send_buf.send(recovering, 0, group.set_comm).wait();
    } else if (!found_here) {
      recv_buf.recv_unknown(sender, 0, group.set_comm).wait();
      e.add_and_fit({recv_buf});
    }

    //Use the same logic as store, but recovering rank is always root and
    //zeroes out the local data region before participating.
    int parity_size = e.size() / (group.set_size - 1);
    int remainder   = e.size() % (group.set_size - 1);
    if (remainder) remainder++;
    int m_parity_size = parity_size;
    if (group.set_rank < remainder) m_parity_size++;
    e.partner.resize(m_parity_size);

    if (!found_here) {
      std::memset(e.data(), 0, e.size());
      std::memset(e.partner.data(), 0, e.partner.size());
    }

    int offset = 0;
    for (int i = 0; i < group.set_size; i++) {
      int len = i < remainder ? parity_size + 1 : parity_size;
      char* input;
      if (group.set_rank == i) {
        input = e.partner.data();
      } else {
        if (offset + len > e.size()) offset--;
        input = e.data() + offset;
        offset += len;
      }
      MPI_Reduce(
        MPI_IN_PLACE, input, len, MPI_BYTE, MPI_BXOR, recovering, group.set_comm
      );
    }
    assert(offset == e.size());

    if (!found_here) {
      e.set_timestamp(ts);
      commit_snapshots_.insert(std::move(avail_snapshots_.back()));
      avail_snapshots_.pop_back();
    }
  }
}

Group::Group(
  int m_id, MPI_Comm m_comm, int m_timestart, int m_depth, int* policy_vals
)
  : fenix_group_t(m_id, m_comm, m_timestart, m_depth, FENIX_DATA_POLICY_IMR) {
  mode = policy_vals ? policy_vals[0] : 1;
  rank_separation =
    policy_vals ? policy_vals[1] : __fenix_get_world_size(m_comm) / 2;

  comm = m_comm;

  int my_rank, comm_size;
  MPI_Comm_size(comm, &comm_size);
  MPI_Comm_rank(comm, &my_rank);
  current_rank = my_rank;

  std::set<int> partner_set;
  partner_set.insert(my_rank);

  if (mode == 1) {
    //odd-sized groups take some extra handling.
    bool isOdd = ((comm_size % 2) != 0);

    int remaining_size = comm_size;
    if (isOdd) remaining_size -= 3;

    //We want to form groups of rank_separation*2 to pair within
    int n_full_groups = remaining_size / (rank_separation * 2);

    //We don't always get what we want though, one group may need to be
    //smaller.
    int mini_group_size =
      (remaining_size - n_full_groups * rank_separation * 2) / 2;

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
      if (((e_rank / rank_separation) % 2) == 0) {
        //Look forward for partner.
        my_partner = my_rank + rank_separation;
        if (isOdd && my_rank < mid_rank && my_partner >= mid_rank) ++my_partner;
      } else {
        my_partner = my_rank - rank_separation;
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
  } else if (mode == 5) {
    set_size = policy_vals[2];

    //User is responsible for giving values that "make sense" for set size and
    //rank separation given a comm size.
    int my_set_pos = (my_rank / rank_separation) % set_size;
    for (int i = 0; i < set_size; i++) {
      int partner =
        (comm_size + my_rank - rank_separation * (my_set_pos - i)) % comm_size;
      partner_set.insert(partner);
    }
  }

  partners = {partner_set.begin(), partner_set.end()};

  //Make same MPI calls as reinit
  reinit();
}

void Group::build_set_comm() {
  if (set_comm != MPI_COMM_NULL) {
    MPI_Comm_free(&set_comm);
    set_comm = MPI_COMM_NULL;
  }

  MPI_Group comm_group, set_group;
  MPI_Comm_group(comm, &comm_group);
  MPI_Group_incl(comm_group, partners.size(), partners.data(), &set_group);
  MPI_Comm_create_group(comm, set_group, 0, &(set_comm));

  MPI_Group_free(&comm_group);
  MPI_Group_free(&set_group);

  MPI_Comm_size(set_comm, &set_size);
  MPI_Comm_rank(set_comm, &set_rank);

  if (!set_comm_revoke_callback) {
    //TODO: This isn't great, and doesn't work w/ fenix restarts
    // (ie finalize then init), we need a better way to refer to callbacks
    // than just push/pop. Maybe a push/pop stack and an add/del map?
    set_comm_revoke_callback = true;
    fenix::callback_register(
      [](MPI_Comm, int) {
        auto dc = fenix_rt.data_recovery;
        if (NULL == dc) return;
        for (auto& group_ptr : dc->groups) {
          auto g = group_ptr.get();
          if (g->policy_name != FENIX_DATA_POLICY_IMR) continue;
          auto imr_g = static_cast<fenix::data::imr::Group*>(g);

          if (imr_g->set_comm == MPI_COMM_NULL) continue;
          MPIX_Comm_revoke(imr_g->set_comm);
        }
      },
      PRE_RECOVERY
    );
  }
}

std::string Group::str() {
  std::stringstream ss;
  ss << "Group " << groupid << " set ";
  ss << "[" << partners[0];
  for (int i = 1; i < partners.size(); i++) ss << ", " << partners[i];
  ss << "]";
  return ss.str();
}

void Group::emplace_member(fenix_member_entry_t&& mentry) {
  // Create and insert the policy-specific Member into members map
  std::shared_ptr<Member> m;
  if (mode == 1) {
    m = std::make_shared<BuddyMember>(std::move(mentry), *this);
  } else if (mode == 5) {
    m = std::make_shared<ParityMember>(std::move(mentry), *this);
  } else {
    assert(false);
  }
  members.insert(m);
}

void Group::commit() {
  fenix_group_t::commit();
  send_buf.clear();
  send_buf.shrink_to_fit();
  recv_buf.clear();
  recv_buf.shrink_to_fit();
}

void Group::member_repair(int member_id) {
  fenix_member_entry_t* member = search_member(member_id);

  std::vector<int> found_members(set_size);
  found_members[set_rank] = member ? 1 : 0;

  int allgather_ret = MPI_Allgather(
    MPI_IN_PLACE, 1, MPI_INT, found_members.data(), 1, MPI_INT, set_comm
  );

  int n_missing   = 0;
  int first_found = -1, missing_rank = -1;
  for (int i = 0; i < found_members.size(); i++) {
    if (!found_members[i]) {
      n_missing++;
      missing_rank = i;
    }
    if (found_members[i] && first_found == -1) first_found = i;
  }

  if (n_missing > 1) {
    if (set_rank != 0) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);

    if (n_missing == set_size) {
      debug_print(
        "ERROR %s member_id %d not found\n", this->str().c_str(), member_id
      );
    } else {
      debug_print(
        "ERROR %s member_id %d unrecoverable\n", this->str().c_str(), member_id
      );
    }
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  } else if (n_missing == 1) {
    fenix_member_entry_packet_t packet;
    if (set_rank == first_found) packet = member->to_packet();

    MPI_Bcast(&packet, sizeof(packet), MPI_BYTE, first_found, set_comm);

    if (!found_members[set_rank]) {
      fenix_group_t::member_create(
        packet.memberid, nullptr, packet.current_count, packet.datatype_size
      );
      member = find_member(member_id);
    }
  }

  member->repair();

  send_buf.clear();
  send_buf.shrink_to_fit();
  recv_buf.clear();
  recv_buf.shrink_to_fit();
}

void Group::member_restore_from_rank(
  int member_id, void* target_buffer, int max_count, int timestamp,
  int source_rank
) {
  fenix_assert(false, "restore_from_rank is not supported yet!");
}

void Group::reinit() {
  build_set_comm();
  sync_timestamps();
}

void Group::sync_timestamps() {
  int n_snapshots = timestamps.size();
  MPI_Allreduce(MPI_IN_PLACE, &n_snapshots, 1, MPI_INT, MPI_MAX, set_comm);

  // Create vector with current timestamps, pad with -1 if needed
  std::vector<int> ts(timestamps.begin(), timestamps.end());
  ts.resize(n_snapshots, -1);

  MPI_Allreduce(
    MPI_IN_PLACE, ts.data(), n_snapshots, MPI_INT, MPI_MAX, set_comm
  );

  // Rebuild set from vector (automatically reverse sorted)
  timestamps.clear();
  for (int t : ts) {
    timestamps.insert(t);
  }

  if (!timestamps.empty()) timestamp = *timestamps.begin(); // Newest first
  else timestamp = -1;
}

void Group::get_redundant_policy(int* policy_name, void* policy_value) {
  *policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID;

  int* policy_vals = (int*)policy_value;
  policy_vals[0]   = mode;
  policy_vals[1]   = rank_separation;
  if (mode == 5) policy_vals[2] = set_size;
}

} // namespace fenix::data::imr
