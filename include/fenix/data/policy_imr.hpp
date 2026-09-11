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

  // Override to also initialize partner's cohort
  void init_cohort(MPI_Comm cohort_comm) override;
  void reinit_cohort(MPI_Comm cohort_comm) override;
};

struct IMRGroup;

struct BuddyMember : public DataMember {
  BuddyMember(DataMember&& member, IMRGroup& group);
  void repair() override;
  tasks::Task<int> iprotect() override;

  std::unique_ptr<DataSnapshot> create_snapshot(int size, int count) override {
    return std::make_unique<imr::IMRSnapshot>(size, count);
  }

  util::DataBuffer& send_buf;
  util::DataBuffer& recv_buf;
};

struct ParityMember : public DataMember {
  ParityMember(DataMember&& member, IMRGroup& group);
  void repair() override;
  tasks::Task<int> iprotect() override;

  // Ensures snapshot sized appropriately.
  // Returns vector of parity bytes for each cohort member, including self.
  std::vector<int> prepare_for_parity(IMRSnapshot& snap);

  std::unique_ptr<DataSnapshot> create_snapshot(int size, int count) override {
    return std::make_unique<imr::IMRSnapshot>(size, count);
  }

  util::DataBuffer& send_buf;
  util::DataBuffer& recv_buf;
};

struct IMRGroup : public DataGroup {
  IMRGroup(int id, MPI_Comm comm, int timestart, int depth, int* policy);

  // Helpers to extract mode and rank_separation from policy_vals
  static int get_mode(int* policy_vals);
  static int get_rank_sep(int* policy_vals, MPI_Comm comm);

  mpixx::Group create_cohort() override;
  void init() override;

  int mode;
  int rank_separation;
  int set_size_policy; // For mode 5, the set_size from policy_vals

  util::DataBuffer send_buf, recv_buf;

  void emplace_member(DataMember&& member) override;
  void get_redundant_policy(int* name, void* value) override;

  void commit() override;

  void member_repair(int member_id) override;
  void member_restore_from_rank(
    int member_id, void* buffer, int max, int timestamp, int source_rank
  ) override;
};

} // namespace fenix::data::imr

#endif //__FENIX_DATA_POLICY_IN_MEMORY_RAID_H__
