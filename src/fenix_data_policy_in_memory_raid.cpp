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
#include "fenix_data_subset.hpp"
#include "fenix_data_policy.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"
#include "fenix_data_policy_in_memory_raid.hpp"
#include "fenix/tasks/mpi.hpp"

#define __FENIX_IMR_DEFAULT_MENTRY_NUM 10
#define __FENIX_IMR_NO_MEMBERS 16000
#define __IMR_RECOVER_DATA_REGION_TAG 97854

#define STORE_PAYLOAD_TAG 2004

namespace fenix::data::imr {

Entry::Entry(int size, int max_count)
  : elm_size(size), elm_max_count(max_count) {
  if (max_count != -1) buf.reserve(size * max_count);
}

Entry::Entry(Entry&& other) { *this = std::move(other); }

Entry& Entry::operator=(Entry&& other) {
  timestamp = std::exchange(other.timestamp, -2);

  region         = std::move(other.region);
  partner_region = std::move(partner_region);
  buf            = std::move(other.buf);
  partner_buf    = std::move(other.partner_buf);
  elm_size       = other.elm_size;
  elm_max_count  = other.elm_max_count;
  return *this;
}

char* Entry::data() { return buf.data(); }
void Entry::resize(int size) { buf.resize(size); }
int Entry::size() { return buf.size(); }

char* Entry::partner_data() { return partner_buf.data(); }
void Entry::partner_resize(int size) { partner_buf.resize(size); }
int Entry::partner_size() { return partner_buf.size(); }

void Entry::add_and_fit(const DataSubset& subset) {
  fenix_assert(subset != SUBSET_PRESTAGED);
  fenix_assert(region != SUBSET_PRESTAGED);
  fenix_assert(region.max_count() > 0 || region.empty());

  region += subset;
  if (elm_max_count) region.bound(elm_max_count - 1);

  int new_count = elm_max_count;
  if (!new_count) new_count = region.max_count();
  fenix_assert(new_count || region.empty());

  int new_size = new_count * elm_size;
  if (new_size > buf.size()) buf.resize(new_size);
}

void Entry::partner_add_and_fit(const DataSubset& subset) {
  fenix_assert(subset != SUBSET_PRESTAGED);
  fenix_assert(partner_region != SUBSET_PRESTAGED);
  fenix_assert(partner_region.max_count() > 0 || partner_region.empty());

  partner_region += subset;
  if (elm_max_count) partner_region.bound(elm_max_count - 1);

  int new_count = elm_max_count;
  if (!new_count) new_count = partner_region.max_count();
  fenix_assert(new_count || partner_region.empty());

  int new_size = new_count * elm_size;
  if (new_size > partner_buf.size()) partner_buf.resize(new_size);
}

void Entry::reset() {
  timestamp = -2;

  buf.clear();
  partner_buf.clear();

  region         = {};
  partner_region = {};
}

Member::Member(fenix_member_entry_t& my_mentry, Group& my_group)
  : mentry(my_mentry), group(my_group), send_buf(group.send_buf),
    recv_buf(group.recv_buf) {
  for (int i = 0; i < group.depth + 2; i++) {
    entries.emplace_back(mentry.datatype_size, mentry.elm_count());
  }
}

BuddyMember::BuddyMember(fenix_member_entry_t& my_mentry, Group& my_group)
  : Member(my_mentry, my_group) {
  if (mentry.user_data.is_bounded()) {
    for (auto& entry : entries) entry.partner_resize(mentry.user_data.size());
  }
}

ParityMember::ParityMember(fenix_member_entry_t& my_mentry, Group& my_group)
  : Member(my_mentry, my_group) {
  int data_len   = mentry.user_data.is_bounded() ? mentry.user_data.size() : 0;
  int parity_len = data_len / (group.set_size - 1);

  int remainder = data_len % (group.set_size - 1);
  if (remainder) remainder++;
  if (remainder < group.set_rank) parity_len++;

  for (auto& entry : entries) entry.partner_resize(parity_len);
}

bool Member::snapshot_delete(int timestamp) {
  bool found = false;
  for (int i = entries.size(); i >= 0; i--) {
    if (entries[i].timestamp == timestamp) {
      assert(!found);
      found = true;
      entries[i].reset();
    }
    //Move deleted snapshot to front
    if (found && i > 0) {
      std::swap(entries[i], entries[i - 1]);
    }
  }
  return found;
}

void Member::stage(const DataSubset& subset) {
  if (subset == SUBSET_PRESTAGED) FENIX_THROW("Cannot stage SUBSET_PRESTAGED");

  Entry& e = entries.back();

  mentry.serialize(subset, e.buf);

  fenix_assert(e.buf.size() % mentry.datatype_size == 0);
  fenix_assert(e.buf.size() <= mentry.user_data.size());

  size_t max_elm = (e.buf.size() / mentry.datatype_size) - 1;
  e.region += subset.bounded(max_elm);
}

void Member::stage_inplace(void* buf, const DataSubset& subset) {
  if (subset == SUBSET_PRESTAGED) FENIX_THROW("Cannot stage SUBSET_PRESTAGED");

  Entry& e = entries.back();
  if (!mentry.user_data.is_bounded() && !subset.is_bounded()) {
    FENIX_THROW(
      "Cannot stage_inplace unbounded subset to FENIX_RESIZEABLE member"
    );
  }

  int count           = mentry.elm_count();
  size_t subset_count = subset.max_count();
  if (subset.is_bounded() && count > subset_count) count = subset_count;

  e.buf.take_ownership((char*)buf, count * e.elm_size);
  e.region = subset.bounded(count - 1);
}

void Member::stage_begin(FILE** fp) {
  mentry.stage_begin(fp, entries.back().buf);
}

void Member::stage_begin(std::iostream** strm) {
  mentry.stage_begin(strm, entries.back().buf);
}

void Member::stage_end() {
  mentry.stage_end();
  Entry& e = entries.back();
  fenix_assert(e.buf.size() % mentry.datatype_size == 0);
  e.region = DataSubset({0, (e.buf.size() / mentry.datatype_size) - 1});
}

tasks::Task<int> Member::istorev(const DataSubset& subset) {
  if (subset != SUBSET_PRESTAGED) stage(subset);

  if (subset != SUBSET_PRESTAGED && subset != SUBSET_FULL) {
    return this->istorev_impl(subset);
  } else {
    return this->istorev_impl(entries.back().region);
  }
}

tasks::Task<int> BuddyMember::exch(
  const DataSubset& subset, const DataSubset& partner_subset
) {
  const int rank  = group.set_rank;
  const int left  = rank == 0 ? group.set_size - 1 : rank - 1;
  const int right = rank == group.set_size - 1 ? 0 : rank + 1;

  Entry& e = entries.back();
  e.partner_add_and_fit(partner_subset);

  int recv_count = partner_subset.count(e.elm_max_count - 1);
  recv_buf.reset(e.elm_size * recv_count);

  subset.pack_data(e.elm_size, e.buf, send_buf);
  co_await tasks::mpi::sendrecv(
    send_buf.data(), send_buf.size(), MPI_BYTE, right, 0,
    recv_buf.data(), recv_buf.size(), MPI_BYTE,  left, 0,
    group.set_comm
  );

  partner_subset.unpack_data(e.elm_size, recv_buf, e.partner_buf);
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
    return this->istore_impl(entries.back().region);
  }
}

tasks::Task<int> BuddyMember::istore_impl(const DataSubset& subset) {
  return exch(subset, subset);
}

tasks::Task<int> ParityMember::istore_impl(const DataSubset& subset) {
  Entry& entry = entries.back();

  int parity_size = entry.size() / (group.set_size - 1);
  int remainder   = entry.size() % (group.set_size - 1);

  //If we have any remainder, treat as if we have one more, since a rank
  //storing a larger parity block wasn't able to store a larger data block, so
  //all such ranks need one extra larger data block.
  if (remainder) remainder++;

  int m_parity_size = parity_size;
  if (group.set_rank < remainder) m_parity_size++;
  entry.partner_resize(m_parity_size);

  //Zero out the parity data before computing, so old data doesn't contribute
  std::memset(entry.partner_data(), 0, entry.partner_size());

  int offset = 0;
  for (int i = 0; i < group.set_size; i++) {
    int len = i < remainder ? parity_size + 1 : parity_size;

    char* input;
    if (group.set_rank == i) {
      //The rank storing this parity region contributes the all zero input
      //as a way of not contributing
      input = entry.partner_data();
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

void Member::commit(int timestamp) {
  entries.back().timestamp = timestamp;

  Entry oldest = std::move(entries.front());
  entries.pop_front();
  oldest.reset();

  entries.push_back(std::move(oldest));
}

int Member::restore() {
  //First clear out any snapshots that we have but the group doesn't.
  auto begin     = group.timestamps.begin();
  const auto end = group.timestamps.end();
  for (int entry = 0; entry < entries.size() - 1; entry++) {
    if (entries[entry].timestamp == -2) continue;
    begin = std::lower_bound(begin, end, entries[entry].timestamp);
    if (begin == end || *begin != entries[entry].timestamp)
      entries[entry].reset();
  }

  //Now make sure snapshots align with group's timestamps
  for (int snapshot = 1; snapshot <= group.timestamps.size(); snapshot++) {
    int timestamp = group.timestamps[group.timestamps.size() - snapshot];
    int target    = entries.size() - snapshot - 1;
    int actual;
    for (actual = target; actual >= 0; actual--) {
      if (entries[actual].timestamp == timestamp) break;
    }
    if (actual == target) continue;
    if (actual != -1) {
      std::swap(entries[actual], entries[target]);
    } else {
      int free = -1;
      for (int i = 0; i <= target && free == -1; i++) {
        if (entries[i].timestamp == -2) free = i;
      }
      assert(free != -1);
      std::swap(entries[free], entries[target]);
    }
  }

  //Reset the current store buffer entry
  entries.back().reset();
  return this->restore_impl();
}

int BuddyMember::restore_impl() {
  //My partner ranks (within set_comm)
  const int rank  = group.set_rank;
  const int left  = rank == 0 ? group.set_size - 1 : rank - 1;
  const int right = rank == group.set_size - 1 ? 0 : rank + 1;

  //Data on which partners have found each snapshot
  int found[3];
  int& found_here  = found[rank];
  int& found_left  = found[left];
  int& found_right = found[right];

  auto e  = entries.rbegin() + 1;
  auto ts = group.timestamps.rbegin();
  for (; ts != group.timestamps.rend(); ts++, e++) {
    fenix_assert(e->timestamp == -2 || e->timestamp == *ts);

    found_here = e->timestamp != -2;
    MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, found, 1, MPI_INT, group.set_comm);

    int n_missing = 0;
    for (int i = 0; i < group.set_size; i++) n_missing += found[i] ? 0 : 1;
    if (n_missing == 0) continue;
    if (n_missing > 1) {
      if (group.set_rank == 0) {
        debug_print(
          "WARNING Fenix_Data_member_restore: %s member %d timestamp %d "
          "unrecoverable",
          group.str().c_str(), id, *ts
        );
      }
      continue;
    }

    if (!found_here) {
      //Fetch my data region from right partner
      recv_buf.recv_unknown(right, 0, group.set_comm).wait();
      e->add_and_fit({recv_buf});
      //Fetch my data
      int m_count = e->region.count(e->elm_max_count - 1);
      recv_buf.recv(m_count * e->elm_size, right, 0, group.set_comm).wait();
      e->region.unpack_data(e->elm_size, recv_buf, e->buf);

      //Fetch left partner's region
      recv_buf.recv_unknown(left, 0, group.set_comm).wait();
      e->partner_add_and_fit({recv_buf});
      //Fetch data
      int p_count = e->partner_region.count(e->elm_max_count - 1);
      recv_buf.recv(p_count * e->elm_size, left, 0, group.set_comm).wait();
      e->partner_region.unpack_data(e->elm_size, recv_buf, e->partner_buf);

      //Only update timestamp after all other data updated, to indicate
      //recovery of this snapshot completed
      e->timestamp = *ts;
    }
    if (!found_left) {
      //Send partner's data region
      e->partner_region.serialize(send_buf);
      send_buf.send(left, 0, group.set_comm).wait();
      //Send their data
      e->partner_region.pack_data(e->elm_size, e->partner_buf, send_buf);
      send_buf.send(left, 0, group.set_comm).wait();
    }
    if (!found_right) {
      //Send my data region
      e->region.serialize(send_buf);
      send_buf.send(right, 0, group.set_comm).wait();
      //Send my data
      e->region.pack_data(e->elm_size, e->buf, send_buf);
      send_buf.send(right, 0, group.set_comm).wait();
    }
  }
  return FENIX_SUCCESS;
}

int ParityMember::restore_impl() {
  //Data on which partners have found each snapshot
  std::vector<int> found;
  found.resize(group.set_size);
  int found_here;

  auto e  = entries.rbegin() + 1;
  auto ts = group.timestamps.rbegin();
  for (; ts != group.timestamps.rend(); ts++, e++) {
    fenix_assert(e->timestamp == -2 || e->timestamp == *ts);

    found_here = e->timestamp != -2;
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
            group.str().c_str(), id, *ts
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
      e->region.serialize(send_buf);
      send_buf.send(recovering, 0, group.set_comm).wait();
    } else if (!found_here) {
      recv_buf.recv_unknown(sender, 0, group.set_comm).wait();
      e->add_and_fit({recv_buf});
    }

    //Use the same logic as store, but recovering rank is always root and
    //zeroes out the local data region before participating.
    int parity_size = e->size() / (group.set_size - 1);
    int remainder   = e->size() % (group.set_size - 1);
    if (remainder) remainder++;
    int m_parity_size = parity_size;
    if (group.set_rank < remainder) m_parity_size++;
    e->partner_resize(m_parity_size);

    if (!found_here) {
      std::memset(e->data(), 0, e->size());
      std::memset(e->partner_data(), 0, e->partner_size());
    }

    int offset = 0;
    for (int i = 0; i < group.set_size; i++) {
      int len = i < remainder ? parity_size + 1 : parity_size;
      char* input;
      if (group.set_rank == i) {
        input = e->partner_data();
      } else {
        if (offset + len > e->size()) offset--;
        input = e->data() + offset;
        offset += len;
      }
      MPI_Reduce(
        MPI_IN_PLACE, input, len, MPI_BYTE, MPI_BXOR, recovering, group.set_comm
      );
    }
    assert(offset == e->size());

    e->timestamp = *ts;
  }

  return FENIX_SUCCESS;
}

int Member::lrestore(
  char* target, int max_restore, int timestamp, DataSubset& recovered
) {
  //Restoring always clears the commit buffer
  entries.back().reset();

  DataRef dst{target, max_restore * mentry.datatype_size};

  // Get index of entry to use
  int end = 0;
  if (timestamp == FENIX_DATA_SNAPSHOT_LATEST) {
    if (entries[entries.size() - 2].timestamp >= 0) end = entries.size() - 1;
  } else if (timestamp == FENIX_DATA_SNAPSHOT_ALL) {
    if (entries[entries.size() - 2].timestamp >= 0) end = entries.size() - 1;
  } else {
    for (int i = entries.size() - 2; i >= 0; i--) {
      if (entries[i].timestamp == timestamp) {
        end = i + 1;
        break;
      }
    }
  }
  // If entry not found, error out
  if (end == 0) FENIX_THROW(FENIX_ERROR_NODATA_FOUND);

  // Recover the data and report back what was recoverable
  for (int i = end - 1; i >= 0; i--) {
    auto& e = entries[i];
    if (e.timestamp < 0) break;

    if (max_restore > 0) {
      DataSubset s = e.region - recovered;
      mentry.deserialize(s, e.buf, dst);
    }

    recovered += e.region;

    // Only FENIX_DATA_SNAPSHOT_ALL recovers from multiple snapshots
    if (timestamp != FENIX_DATA_SNAPSHOT_ALL) break;
  }

  if (max_restore > 0 && !recovered.includes_all(max_restore - 1)) {
    return FENIX_WARNING_PARTIAL_RESTORE;
  }
  return FENIX_SUCCESS;
}

Group::Group(
  int m_id, MPI_Comm m_comm, int m_timestart, int m_depth, int* policy_vals,
  int* flag
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
  reinit(flag);
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
        auto groups = fenix_rt.data_recovery;
        if (NULL == groups) return;
        for (int i = 0; i < groups->count; i++) {
          auto g = groups->group[i];
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

Member* Group::find_member(int memberid) {
  auto iter = member_data.find(memberid);
  if (iter != member_data.end()) return iter->second.get();
  return nullptr;
}

std::string Group::str() {
  std::stringstream ss;
  ss << "Group " << groupid << " set ";
  ss << "[" << partners[0];
  for (int i = 1; i < partners.size(); i++) ss << ", " << partners[i];
  ss << "]";

  return ss.str();
}

int Group::member_create(fenix_member_entry_t* mentry) {
  auto iter = member_data.try_emplace(mentry->memberid, nullptr);
  if (!iter.second) FENIX_THROW(FENIX_ERROR_MEMBER_EXISTS);

  auto& m = iter.first->second;
  if (mode == 1) m = std::make_shared<BuddyMember>(*mentry, *this);
  else if (mode == 5) m = std::make_shared<ParityMember>(*mentry, *this);
  else assert(false);

  return FENIX_SUCCESS;
}

int Group::member_delete(fenix_member_entry_t* mentry) {
  auto iter = member_data.find(mentry->memberid);

  if (iter == member_data.end()) {
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  }

  member_data.erase(iter);
  return FENIX_SUCCESS;
}

void Group::member_stage(int member_id, const DataSubset& subset) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  iter->second->stage(subset);
}

void Group::member_stage_inplace(
  int member_id, void* buf, const DataSubset& subset
) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  iter->second->stage_inplace(buf, subset);
}

void Group::member_stage_begin(int member_id, FILE** fp) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  iter->second->stage_begin(fp);
}

void Group::member_stage_begin(int member_id, std::iostream** strm) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  iter->second->stage_begin(strm);
}

void Group::member_stage_end(int member_id) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  iter->second->stage_end();
}

int Group::member_store(int member_id, const DataSubset& subset) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) {
    debug_print(
      "ERROR Fenix_Data_member_store: %s unknown member_id %d on rank %d\n",
      this->str().c_str(), member_id, current_rank
    );
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  }
  return iter->second->store(subset);
}

int Group::member_storev(int member_id, const DataSubset& subset) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) {
    debug_print(
      "ERROR Fenix_Data_member_storev: %s unknown member_id %d on rank %d\n",
      this->str().c_str(), member_id, current_rank
    );
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  }
  return iter->second->storev(subset);
}

int Group::member_istore(
  int member_id, const DataSubset& subset, Fenix_Request* request
) {
  return 0;
}

int Group::member_istorev(
  int member_id, const DataSubset& subset_specifier, Fenix_Request* request
) {
  return 0;
}

int Group::commit() {
  if (timestamps.size() == depth + 1) {
    //Full of timestamps, remove the oldest and proceed as normal.
    timestamps.pop_front();
  }
  timestamps.push_back(timestamp);

  for (auto& iter : member_data) {
    iter.second->commit(timestamp);
  }

  send_buf.clear();
  send_buf.shrink_to_fit();
  recv_buf.clear();
  recv_buf.shrink_to_fit();

  return FENIX_SUCCESS;
}

int Group::snapshot_delete(int to_delete) {
  int retval = FENIX_SUCCESS;

  bool found = false;
  for (auto it = timestamps.begin(); it != timestamps.end(); it++) {
    if (*it == to_delete) {
      timestamps.erase(it);
      found = true;
      break;
    }
  }
  for (auto& iter : member_data) {
    found |= iter.second->snapshot_delete(to_delete);
  }
  if (!found) FENIX_THROW(FENIX_ERROR_INVALID_TIMESTAMP);
  return FENIX_SUCCESS;
}

int Group::barrier() { return 0; }

int Group::get_number_of_snapshots(int* num) {
  *num = timestamps.size();
  return FENIX_SUCCESS;
}

int Group::get_snapshot_at_position(int idx, int* snapshot) {
  if (idx >= timestamps.size() || idx < 0)
    FENIX_THROW(FENIX_ERROR_INVALID_POSITION);

  *snapshot = timestamps[idx];
  return FENIX_SUCCESS;
}

std::vector<int> Group::get_snapshots() {
  return {timestamps.begin(), timestamps.end()};
}

int Group::member_restore(
  int member_id, void* target_buffer, int max_count, int ts,
  DataSubset& data_found
) {
  //TODO: Is this fix needed anymore?
  //One-time fix after a reinit.
  if (timestamp == -1 && !timestamps.empty()) timestamp = timestamps.back();

  Member* member = find_member(member_id);

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
        "ERROR Fenix_Data_member_restore: %s member_id %d not found\n",
        this->str().c_str(), member_id
      );
    } else {
      debug_print(
        "ERROR Fenix_Data_member_restore: %s member_id %d unrecoverable\n",
        this->str().c_str(), member_id
      );
    }
    FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  } else if (n_missing == 1) {
    fenix_member_entry_packet_t packet;
    if (set_rank == first_found) packet = member->mentry.to_packet();

    MPI_Bcast(&packet, sizeof(packet), MPI_BYTE, first_found, set_comm);

    if (!found_members[set_rank]) {
      fenix_group_t::member_create(
        packet.memberid, target_buffer, packet.current_count,
        packet.datatype_size
      );
      member = find_member(member_id);
    }
  }

  member->restore();

  send_buf.clear();
  send_buf.shrink_to_fit();
  recv_buf.clear();
  recv_buf.shrink_to_fit();

  return member->lrestore((char*)target_buffer, max_count, ts, data_found);
}

int Group::member_lrestore(
  int member_id, void* target_buffer, int max_count, int ts,
  DataSubset& data_found
) {
  auto iter = member_data.find(member_id);
  if (iter == member_data.end()) FENIX_THROW(FENIX_ERROR_INVALID_MEMBERID);
  return iter->second->lrestore(
    (char*)target_buffer, max_count, ts, data_found
  );
}

int Group::member_restore_from_rank(
  int member_id, void* target_buffer, int max_count, int timestamp,
  int source_rank
) {
  fenix_assert(false, "restore_from_rank is not supported yet!");
  return FENIX_ERROR_NOCATEGORY;
}

int Group::member_get_attribute(
  fenix_member_entry_t* member, int attributename, void* attributevalue,
  int* flag, int sourcerank
) {
  return FENIX_SUCCESS;
}

int Group::member_set_attribute(
  fenix_member_entry_t* member, int attributename, void* attributevalue,
  int* flag
) {
  //No mutable attributes (as of now) require any changes to this policy's info
  return FENIX_SUCCESS;
}

int Group::reinit(int* flag) {
  build_set_comm();
  sync_timestamps();

  *flag = FENIX_SUCCESS;
  return *flag;
}

void Group::sync_timestamps() {
  int n_snapshots = timestamps.size();
  MPI_Allreduce(MPI_IN_PLACE, &n_snapshots, 1, MPI_INT, MPI_MAX, set_comm);

  for (int i = timestamps.size(); i < n_snapshots; i++) {
    timestamps.push_front(-1);
  }

  std::vector<int> ts = {timestamps.begin(), timestamps.end()};
  MPI_Allreduce(
    MPI_IN_PLACE, ts.data(), n_snapshots, MPI_INT, MPI_MAX, set_comm
  );
  timestamps = {ts.begin(), ts.end()};

  if (!timestamps.empty()) timestamp = timestamps.back();
  else timestamp = -1;
}

int Group::get_redundant_policy(
  int* policy_name, void* policy_value, int* flag
) {
  *policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID;

  int* policy_vals = (int*)policy_value;
  policy_vals[0]   = mode;
  policy_vals[1]   = rank_separation;
  if (mode == 5) policy_vals[2] = set_size;

  *flag = FENIX_SUCCESS;
  return *flag;
}

int Group::group_delete() {
  delete this;
  return FENIX_SUCCESS;
}
} // namespace fenix::data::imr
