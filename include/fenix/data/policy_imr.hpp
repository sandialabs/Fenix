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
#include "fenix/data/group.hpp"
#include "fenix/data/util/buffer.hpp"
#include "fenix/data/subset.hpp"
#include "fenix/data/snapshot.hpp"
#include "fenix/tasks/task.hpp"

namespace fenix::data::imr {

struct IMRSnapshot : public fenix::data::DataSnapshot {
  IMRSnapshot(int size, int max_count);

  //IMR-specific partner data as a second DataSnapshot
  DataSnapshot partner;

  //Accessor for partner snapshot
  DataSnapshot& partner_snapshot() { return partner; }
};

struct IMRGroup;

struct IMRMember : public DataMember {
  IMRMember(DataMember&& member, IMRGroup& group);

  // Override create_snapshot to return IMR IMRSnapshot type
  std::unique_ptr<DataSnapshot> create_snapshot(
    int size, int max_count
  ) override {
    return std::make_unique<imr::IMRSnapshot>(size, max_count);
  }

  // Staging and loading functions use base class default implementations

  // IMRMember::istore(v) handle local data and region, while istore(v)_impl
  // handle partner data and region
  tasks::Task<int> istore(const DataSubset& subset) override;
  virtual tasks::Task<int> istore_impl(const DataSubset& subset) = 0;

  tasks::Task<int> istorev(const DataSubset& subset) override;
  virtual tasks::Task<int> istorev_impl(const DataSubset& subset) = 0;

  //Restore all internal snapshot data
  //Moves snapshots to align with the group's list of timestamps.
  //Impl must handle actually restoring snapshot data
  void repair();
  virtual void repair_impl() = 0;

  IMRGroup& group;
  int id = memberid;

  util::DataBuffer& send_buf;
  util::DataBuffer& recv_buf;
};

struct BuddyMember : public IMRMember {
  BuddyMember(DataMember&& member, IMRGroup& group);
  void repair_impl() override;
  tasks::Task<int> istore_impl(const DataSubset& subset) override;
  tasks::Task<int> istorev_impl(const DataSubset& subset) override;
  tasks::Task<int> exch(
    const DataSubset& subset, const DataSubset& partner_subset
  );
};

struct ParityMember : public IMRMember {
  ParityMember(DataMember&& member, IMRGroup& group);
  void repair_impl() override;
  tasks::Task<int> istore_impl(const DataSubset& subset) override;

  tasks::Task<int> istorev_impl(const DataSubset& subset) override {
    fatal_print("IMR mode 5 cannot storev");
    co_return 0;
  }
};

struct IMRGroup : public DataGroup {
  IMRGroup(int id, MPI_Comm comm, int timestart, int depth, int* policy);

  int mode;
  int rank_separation;
  std::vector<int> partners;

  MPI_Comm set_comm = MPI_COMM_NULL;
  int set_size, set_rank;
  static inline bool set_comm_revoke_callback = false;

  util::DataBuffer send_buf, recv_buf;

  void sync_timestamps();
  void build_set_comm();

  std::string str();

  void emplace_member(DataMember&& member) override;
  void get_redundant_policy(int* name, void* value) override;

  void commit() override;

  void member_repair(int member_id) override;
  void member_restore_from_rank(
    int member_id, void* buffer, int max, int timestamp, int source_rank
  ) override;

  void reinit() override;
};

} // namespace fenix::data::imr

#endif //__FENIX_DATA_POLICY_IN_MEMORY_RAID_H__
