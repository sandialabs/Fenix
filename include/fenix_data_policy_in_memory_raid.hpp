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

#ifndef __FENIX_DATA_POLICY_IN_MEMORY_RAID_H__
#define __FENIX_DATA_POLICY_IN_MEMORY_RAID_H__

#include <mpi.h>
#include <map>
#include <memory>
#include <deque>
#include <cassert>
#include <string>
#include "fenix_data_group.hpp"
#include "fenix_data_buffer.hpp"
#include "fenix_data_subset.hpp"

namespace Fenix::Data::IMR {

void __fenix_policy_in_memory_raid_get_group(fenix_group_t** group, MPI_Comm comm,
      int timestart, int depth, void* policy_value, int* flag);

struct Entry {
  //No copying, must be moved
  Entry(const Entry&) = delete;
  Entry(Entry&&);
  Entry& operator=(Entry&&);

  Entry(int size, int max_count);
  
  //Re-initializes
  void reset();

  //Get raw buffer pointer
  char* data();
  //Get buffer size
  int   size();
  //Resize buffer
  void  resize(int size);
  //Add subset to region and ensure buf is large enough.
  void add_and_fit(const DataSubset& subset);
  
  DataBuffer buf;
  DataSubset region;
  
  char* partner_data();
  int   partner_size();
  void  partner_resize(int size);
  void partner_add_and_fit(const DataSubset& subset);

  DataBuffer partner_buf;
  DataSubset partner_region;
  
  int timestamp = -2;
  int elm_size;
  int elm_max_count;
};

struct Group;

struct Member {
  Member(fenix_member_entry_t& mentry, Group& group);

  //Returns true if snapshot was found.
  bool snapshot_delete(int timestamp);

  //Member::store(v) copies local data and region.
  int store(const DataSubset& subset);
  //Handles partner data and region
  virtual int store_impl(const DataSubset& subset) = 0;

  //As store(_impl)
  int storev(const DataSubset& subset);
  virtual int storev_impl(const DataSubset& subset) = 0;

  //Restore all internal snapshot data
  //Moves entries to align with the group's list of timestamps.
  //Impl must actually restoring entry data
  int restore();
  virtual int restore_impl() = 0;

  int lrestore(char* target, int max, int timestamp, DataSubset& subset);

  void commit(int timestamp);

  fenix_member_entry_t& mentry;
  Group& group;
  int id = mentry.memberid;
  // entries to be initialized by inheritors
  std::deque<Entry> entries;

  DataBuffer& send_buf;
  DataBuffer& recv_buf;
};

struct BuddyMember : public Member {
  BuddyMember(fenix_member_entry_t& mentry, Group& group);
  int restore_impl() override;
  int store_impl(const DataSubset& subset) override;
  int storev_impl(const DataSubset& subset) override;
  int exch(const DataSubset& subset, const DataSubset& partner_subset);
};

struct ParityMember : public Member {
  ParityMember(fenix_member_entry_t& mentry, Group& group);
  int restore_impl() override;
  int store_impl(const DataSubset& subset) override;

  int storev_impl(const DataSubset& subset) override {
    fatal_print("IMR mode 5 cannot storev");
    return 0;
  };
};

struct Group : public fenix_group_t {
  Group(MPI_Comm comm, int timestart, int depth, int* policy, int* flag);

  int mode;
  int rank_separation;
  std::vector<int> partners;

  MPI_Comm set_comm = MPI_COMM_NULL;
  int set_size, set_rank;

  std::map<int, std::shared_ptr<Member>> member_data;
  std::deque<int> timestamps;

  DataBuffer send_buf, recv_buf;

  void sync_timestamps();
  void build_set_comm();

  //nullptr if member not found
  Member* find_member(int member_id);
  
  std::string str();

  int group_delete() override;
  int member_create(fenix_member_entry_t* mentry) override;
  int member_delete(int member_id) override;
  int get_redundant_policy(int* name, void* value, int* flag) override;

  int member_store(int member_id, const DataSubset& subset) override;
  int member_storev(int member_id, const DataSubset& subset) override;
  int member_istore(
    int member_id, const DataSubset& subset, Fenix_Request *request
  ) override;
  int member_istorev(
    int member_id, const DataSubset& subset, Fenix_Request *request
  ) override;

  int commit() override;

  int snapshot_delete(int timestamp) override;
  int barrier() override;

  int member_restore(
    int member_id, void* buffer, int max, int timestamp, DataSubset& data_found
  ) override;
  int member_lrestore(
    int member_id, void* buffer, int max, int timestamp, DataSubset& data_found
  ) override;
  int member_restore_from_rank(
    int member_id, void* buffer, int max, int timestamp, int source_rank
  ) override;

  int member_get_attribute(
    fenix_member_entry_t* member, int name, void* value, int* flag,
    int sourcerank
  ) override;
  int member_set_attribute(
    fenix_member_entry_t* member, int name, void* value, int* flag
  ) override;

  int get_number_of_snapshots(int* number_of_snapshots) override;
  int get_snapshot_at_position(int position, int* timestamp) override;

  int reinit(int* flag) override;
};

}

#endif //__FENIX_DATA_POLICY_IN_MEMORY_RAID_H__
