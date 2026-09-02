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
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/
#ifndef __FENIX_DATA_MEMBER_H__
#define __FENIX_DATA_MEMBER_H__

#include <optional>
#include <deque>
#include <memory>
#include <source_location>

#include "fenix/data/subset.hpp"
#include "fenix/data/util/buffer.hpp"
#include "fenix/data/snapshot.hpp"
#include "fenix/data/util/data_ref.hpp"
#include "fenix/data/util/serializer.hpp"
#include "fenix/tasks/task.hpp"
#include "fenix/mpixx/datatype.hpp"

namespace fenix::data {

struct DataGroup;

class DataMember {
 public:
  using Serializer = util::Serializer;
  using CommitSet =
    std::set<std::unique_ptr<DataSnapshot>, DataSnapshotTimestampComparator>;
  using CommitIter = CommitSet::iterator;

  DataMember() = delete;

  DataMember(
    DataGroup& g, int id, void* data, int count, MPI_Datatype datatype,
    int depth, std::optional<SerializeFunc> s = {}
  );
  DataMember(DataGroup& g, const util::DataBuffer& serialized, int depth);

  DataMember(DataMember&& other);

  // Serialize member metadata (memberid, count, datatype)
  util::DataBuffer serialize() const;

  int memberid = -1;
  mpixx::Datatype datatype_;
  util::DataRef user_data;
  DataGroup* group;

  int elm_count() const;

  // Create a (possibly policy-specific) snapshot with specified capacity
  virtual std::unique_ptr<DataSnapshot> create_snapshot(
    int size, int max_count
  );

  // Data operations with default local-only implementations
  virtual void stage(const DataSubset& subset);
  virtual void stage_inplace(void* buf, const DataSubset& subset);
  virtual void stage_begin(FILE** fp);
  virtual void stage_begin(std::iostream** strm);
  virtual void stage_end();
  virtual void load_begin(FILE** fp, int timestamp, DataSubset& subset);
  virtual void load_begin(
    std::iostream** strm, int timestamp, DataSubset& subset
  );
  virtual void load_end();
  virtual void load(
    void* target, int target_count, int timestamp, DataSubset& data_found
  );
  virtual int store(const DataSubset& subset);
  virtual int storev(const DataSubset& subset);
  virtual tasks::Task<int> istore(const DataSubset& subset);
  virtual tasks::Task<int> istorev(const DataSubset& subset);
  virtual tasks::Task<int> iprotect();
  virtual void repair();
  virtual void commit(int timestamp);
  virtual void snapshot_delete(int timestamp);

  void snapshot_delete(CommitIter it);

  virtual void attr_set(int attr, void* value);
  virtual void attr_get(int attr, void* value);

  // Returns true if staging snapshot contains unstored data
  virtual bool has_unstored_data();

  // Essentially an inplace allgather within the cohort - each rank sends the
  // subset at its cohort rank index.
  tasks::Task<void> exchange_subsets(std::vector<DataSubset>& subsets);

  // Broadcast subset vector from root to all ranks in cohort
  tasks::Task<void> broadcast_subsets(
    const std::vector<DataSubset>& input, std::vector<DataSubset>& output,
    int root
  );
  tasks::Task<void> broadcast_subsets(
    std::vector<DataSubset>& subsets, int root
  ) {
    return broadcast_subsets(subsets, subsets, root);
  }

  virtual ~DataMember() = default;

  // Must be called AFTER this class's constructor completes, else virtual
  // emplace_snapshot overrides won't be used.
  void init_snapshots();

  std::optional<SerializeFunc> ser_func;

  // Set iff stage_begin called with no matching stage_end
  std::optional<Serializer> open_serializer;

 protected:
  // Snapshot storage - three separate locations

  // Current uncommitted staging snapshot
  std::unique_ptr<DataSnapshot> stage_snapshot_;

  // Committed snapshots ordered by timestamp (oldest to newest)
  CommitSet commit_snapshots_;

  // Pool of unused snapshots ready for reuse
  std::vector<std::unique_ptr<DataSnapshot>> avail_snapshots_;

  // Maximum allowed snapshots (from group depth)
  int depth_ = 0;

  /**
   * @brief Get reference to the current staging snapshot.
   *
   * @return Reference to the staging snapshot
   */
  DataSnapshot& current_snapshot();

  // search for committed timestamp, returning null if not found
  // Throws if timestamp is not valid (including FENIX_DATA_SNAPSHOT_ALL)
  DataSnapshot* search_snapshot(
    int timestamp, std::source_location loc = std::source_location::current()
  );
  // As search_snapshot, but throw if not found
  DataSnapshot* find_snapshot(
    int timestamp, std::source_location loc = std::source_location::current()
  );

  // Remove committed snapshots not in the provided timestamp set
  // Used by DataGroup::sync_timestamps() to clean up after recovery
  void cleanup_timestamps(
    const std::set<int, std::greater<int>>& valid_timestamps
  );

  friend class DataGroup;

 private:
  // Note that Serializers aren't guaranteed to have written their data to the
  // buffer until their destructor is called. So these should usually only be
  // used to construct temporaries that go to a subset's copy_data call
};

struct DataMemberIdComparator {
  using is_transparent = void; // Enables heterogeneous lookup

  bool operator()(
    const std::shared_ptr<DataMember>& a, const std::shared_ptr<DataMember>& b
  ) const {
    return a->memberid < b->memberid;
  }

  bool operator()(const std::shared_ptr<DataMember>& a, int id) const {
    return a->memberid < id;
  }

  bool operator()(int id, const std::shared_ptr<DataMember>& a) const {
    return id < a->memberid;
  }
};

} // namespace fenix::data
#endif // FENIX_DATA_MEMBER_H
