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

#include "fenix_util.hpp"
#include "fenix/data/group.hpp"
#include "fenix/data/member.hpp"
#include "fenix/mpixx/datatype.hpp"
#include "fenix/mpixx/tasks.hpp"
#include <cstring>

namespace fenix::data {

using util::ConstDataRef;
using util::DataBuffer;
using util::DataRef;

DataMember::DataMember(
  DataGroup& g, int id, void* d, int c, MPI_Datatype dt, int depth,
  std::optional<SerializeFunc> s
)
  : memberid(id), datatype_(mpixx::Datatype::dup(dt)), depth_(depth),
    group(&g) {
  int dsize = datatype_.extent();
  if (c == FENIX_RESIZEABLE) user_data = DataRef((char*)d);
  else user_data = DataRef((char*)d, c * dsize);

  ser_func = s;

  // Note: stage_snapshot_ and avail_snapshots_ are NOT created here
  // because create_snapshot() is virtual and won't dispatch to derived
  // class during base constructor. Derived classes must call init_snapshots().
}

void DataMember::init_snapshots() {
  stage_snapshot_ = this->create_snapshot(datatype_.extent(), elm_count());

  // Initialize cohort on the staging snapshot
  if (group && group->cohort_comm != MPI_COMM_NULL) {
    stage_snapshot_->init_cohort(group->cohort_comm);
  }

  for (int i = 0; i < depth_ + 1; i++) {
    avail_snapshots_.push_back(
      this->create_snapshot(datatype_.extent(), elm_count())
    );
  }
}

DataMember::DataMember(DataGroup& g, const DataBuffer& buf, int depth)
  : depth_(depth), group(&g) {
  // Deserialize: memberid (int), current_count (int), datatype_size (int),
  // datatype_data
  fenix_assert(
    buf.size() >= sizeof(int) * 3,
    "Buffer too small for DataMember deserialization"
  );

  int offset = 0;
  std::memcpy(&memberid, buf.data() + offset, sizeof(int));
  offset += sizeof(int);

  int count;
  std::memcpy(&count, buf.data() + offset, sizeof(int));
  offset += sizeof(int);

  int dt_size;
  std::memcpy(&dt_size, buf.data() + offset, sizeof(int));
  offset += sizeof(int);

  fenix_assert(
    buf.size() >= offset + dt_size, "Buffer too small for datatype data"
  );

  // Deserialize datatype (cast to uint8_t* as required)
  datatype_ = mpixx::Datatype::deserialize(
    reinterpret_cast<const uint8_t*>(buf.data() + offset), dt_size
  );

  // Initialize user_data (nullptr for now, will be set by user later)
  int dsize = datatype_.extent();
  if (count == FENIX_RESIZEABLE) {
    user_data = DataRef(nullptr);
  } else {
    user_data = DataRef(nullptr, count * dsize);
  }

  // Note: stage_snapshot_ and avail_snapshots_ are NOT created here
  // because create_snapshot() is virtual. Caller must call init_snapshots().
}

DataMember::DataMember(DataMember&& o) {
  fenix_assert(!o.open_serializer);
  memberid   = o.memberid;
  o.memberid = -1;

  datatype_ = std::move(o.datatype_);

  user_data   = o.user_data;
  o.user_data = {nullptr};

  if (o.ser_func) {
    ser_func.emplace(std::move(*o.ser_func));
    o.ser_func.reset();
  }

  stage_snapshot_   = std::move(o.stage_snapshot_);
  commit_snapshots_ = std::move(o.commit_snapshots_);
  avail_snapshots_  = std::move(o.avail_snapshots_);
  depth_            = o.depth_;
  o.depth_          = 0;

  group   = o.group;
  o.group = nullptr;
}

DataBuffer DataMember::serialize() const {
  // Serialize: memberid (int), current_count (int), datatype_size (int),
  // datatype_data
  auto dt_buf = datatype_.serialize();

  size_t total_size = sizeof(int) * 3 + dt_buf.size();
  DataBuffer result(total_size);

  int offset = 0;
  std::memcpy(result.data() + offset, &memberid, sizeof(int));
  offset += sizeof(int);

  int current_count = elm_count();
  std::memcpy(result.data() + offset, &current_count, sizeof(int));
  offset += sizeof(int);

  int dt_size = static_cast<int>(dt_buf.size());
  std::memcpy(result.data() + offset, &dt_size, sizeof(int));
  offset += sizeof(int);

  std::memcpy(result.data() + offset, dt_buf.data(), dt_buf.size());

  return result;
}

int DataMember::elm_count() const {
  if (!user_data.is_bounded()) return FENIX_RESIZEABLE;

  fenix_assert(user_data.size() % datatype_.extent() == 0);
  return user_data.size() / datatype_.extent();
}

void DataMember::stage(const DataSubset& subset) {
  if (subset == SUBSET_PRESTAGED) FENIX_THROW("Cannot stage SUBSET_PRESTAGED");

  DataSnapshot& snap = *stage_snapshot_;
  subset.copy_data(snap.create_serializer(user_data, ser_func, subset));

  fenix_assert(snap.buf().size() % datatype_.extent() == 0);
  fenix_assert(snap.buf().size() <= user_data.size());

  size_t max_elm = (snap.buf().size() / datatype_.extent()) - 1;
  snap.add_and_fit(subset.bounded(max_elm));
}

void DataMember::stage_inplace(void* buf, const DataSubset& subset) {
  if (subset == SUBSET_PRESTAGED) FENIX_THROW("Cannot stage SUBSET_PRESTAGED");

  DataSnapshot& snap = *stage_snapshot_;
  if (!user_data.is_bounded() && !subset.is_bounded()) {
    FENIX_THROW(
      "Cannot stage_inplace unbounded subset to FENIX_RESIZEABLE member"
    );
  }

  int count           = elm_count();
  size_t subset_count = subset.max_count();
  if (subset.is_bounded() && count > subset_count) count = subset_count;

  snap.buf().take_ownership((char*)buf, count * snap.elm_size());
  snap.add_and_fit(subset.bounded(count - 1));
}

void DataMember::stage_begin(FILE** fp) {
  std::optional<SerializeFunc> sf = SerializeFileFunc{};
  stage_snapshot_->buf().resize(0);
  open_serializer.emplace(
    stage_snapshot_->create_serializer(user_data, sf, SUBSET_FULL)
  );
  *fp = open_serializer->get_file();
}

void DataMember::stage_begin(std::iostream** strm) {
  std::optional<SerializeFunc> sf = SerializeStreamFunc{};
  stage_snapshot_->buf().resize(0);
  open_serializer.emplace(
    stage_snapshot_->create_serializer(user_data, sf, SUBSET_FULL)
  );
  *strm = open_serializer->get_stream();
}

void DataMember::stage_end() {
  if (!open_serializer) FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
  if (open_serializer->get_dir() != FENIX_SERIALIZE)
    FENIX_THROW(FENIX_ERROR_MEMBER_LOADING);
  open_serializer.reset();

  DataSnapshot& snap = *stage_snapshot_;
  fenix_assert(snap.buf().size() % datatype_.extent() == 0);
  snap.add_and_fit(
    DataSubset({0, (snap.buf().size() / datatype_.extent()) - 1})
  );
}

void DataMember::load_begin(FILE** fp, int timestamp, DataSubset& subset) {
  DataSnapshot* snap = find_snapshot(timestamp);
  subset             = snap->protected_subset();

  std::optional<SerializeFunc> sf = SerializeFileFunc{};
  open_serializer.emplace(snap->create_deserializer(nullptr, sf, SUBSET_FULL));
  *fp = open_serializer->get_file();
}

void DataMember::load_begin(
  std::iostream** strm, int timestamp, DataSubset& subset
) {
  DataSnapshot* snap = find_snapshot(timestamp);
  subset             = snap->protected_subset();

  std::optional<SerializeFunc> sf = SerializeStreamFunc{};
  open_serializer.emplace(snap->create_deserializer(nullptr, sf, SUBSET_FULL));
  *strm = open_serializer->get_stream();
}

void DataMember::load_end() {
  if (!open_serializer) FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
  if (open_serializer->get_dir() != FENIX_DESERIALIZE)
    FENIX_THROW(FENIX_ERROR_MEMBER_STAGING);
  open_serializer.reset();
}

void DataMember::load(
  void* target, int target_count, int timestamp, DataSubset& data_found
) {
  DataRef dst{
    (char*)target, static_cast<size_t>(target_count * datatype_.extent())
  };
  if (target == FENIX_DATA_RESTORE_INPLACE)
    dst = {user_data.data(), dst.size()};
  if (target_count == FENIX_DATA_RESTORE_FULL) dst = {dst.data()};

  if (timestamp != FENIX_DATA_SNAPSHOT_ALL) {
    DataSnapshot& snap = *find_snapshot(timestamp);
    data_found         = snap.protected_subset();
    if (target_count > 0) {
      data_found.copy_data(snap.create_deserializer(dst, ser_func, data_found));
    }
  } else {
    if (commit_snapshots_.empty()) FENIX_THROW(FENIX_ERROR_NODATA_FOUND);

    data_found = {};
    for (auto it = commit_snapshots_.rbegin(); it != commit_snapshots_.rend();
         ++it) {
      DataSnapshot& snap = **it;
      fenix_assert(snap.timestamp() >= 0);
      if (target_count > 0) {
        DataSubset partial = snap.protected_subset() - data_found;
        partial.copy_data(snap.create_deserializer(dst, ser_func, partial));
      }
      data_found += snap.protected_subset();
      if (target_count > 0 && data_found.includes_all(target_count - 1)) break;
    }
  }

  if (target_count == FENIX_DATA_RESTORE_FULL) {
    target_count = data_found.max_count();
  }
  if (target_count > 0 && !data_found.includes_all(target_count - 1)) {
    FENIX_THROW(FENIX_WARNING_PARTIAL_RESTORE);
  }
}

int DataMember::store(const DataSubset& subset) {
  // Default: call async version and wait
  return istore(subset).result();
}

int DataMember::storev(const DataSubset& subset) {
  // Default: call async version and wait
  return istorev(subset).result();
}

tasks::Task<int> DataMember::istore(const DataSubset& subset) {
  if (subset != SUBSET_PRESTAGED) this->stage(subset);
  for (int i = 0; i < stage_snapshot_->staged_subsets.size(); i++) {
    if (i == stage_snapshot_->cohort_rank) continue;
    stage_snapshot_->staged_subsets[i] = stage_snapshot_->staged_subset();
  }
  return iprotect();
}

tasks::Task<int> DataMember::istorev(const DataSubset& subset) {
  // Default: No data resilience
  if (subset != SUBSET_PRESTAGED) this->stage(subset);
  co_await exchange_subsets(stage_snapshot_->staged_subsets);
  co_return co_await iprotect();
}

tasks::Task<int> DataMember::iprotect() {
  // Default: simply move staged data to protected (no remote redundancy)
  DataSnapshot& snap = *stage_snapshot_;
  for (int i = 0; i < snap.protected_subsets.size(); i++) {
    snap.protected_subsets[i] += snap.staged_subsets[i];
    snap.staged_subsets[i] = {};
  }
  co_return FENIX_SUCCESS;
}

void DataMember::repair() {
  // Default: no-op repair
  return;
}

void DataMember::commit(int timestamp) {
  fenix_assert(stage_snapshot_->staged_subset() == SUBSET_EMPTY);

  // Set timestamp on staging snapshot
  stage_snapshot_->set_timestamp(timestamp);

  // Move staging snapshot to committed snapshots
  commit_snapshots_.insert(std::move(stage_snapshot_));

  // Trim if we exceed depth + 1
  if (commit_snapshots_.size() > depth_ + 1) {
    this->snapshot_delete((*commit_snapshots_.begin())->timestamp());
  }

  // Get new staging snapshot from available pool or create new
  if (!avail_snapshots_.empty()) {
    stage_snapshot_ = std::move(avail_snapshots_.back());
    avail_snapshots_.pop_back();
  } else {
    stage_snapshot_ = this->create_snapshot(datatype_.extent(), elm_count());
  }

  // Initialize cohort for the new stage snapshot
  stage_snapshot_->init_cohort(group->cohort_comm);
}

void DataMember::snapshot_delete(int timestamp) {
  auto it = commit_snapshots_.find(timestamp);
  if (it != commit_snapshots_.end()) {
    snapshot_delete(it);
  }
}

void DataMember::snapshot_delete(CommitIter it) {
  if (it == commit_snapshots_.end()) FENIX_THROW(FENIX_ERROR_INVALID_TIMESTAMP);
  auto node = commit_snapshots_.extract(it);
  node.value()->reset();
  avail_snapshots_.push_back(std::move(node.value()));
}

std::unique_ptr<DataSnapshot> DataMember::create_snapshot(
  int size, int max_count
) {
  // Default implementation creates a base DataSnapshot
  // Derived classes override to create policy-specific Entry types
  return std::make_unique<DataSnapshot>(size, max_count);
}

tasks::Task<void> DataMember::exchange_subsets(
  std::vector<DataSubset>& subsets
) {
  int cohort_size, cohort_rank;
  MPI_Comm_size(group->cohort_comm, &cohort_size);
  MPI_Comm_rank(group->cohort_comm, &cohort_rank);

  fenix_assert(subsets.size() == cohort_size);

  // Serialize this rank's subset (at index cohort_rank)
  DataBuffer send_buf;
  subsets[cohort_rank].serialize(send_buf);

  // Gather sizes from all cohort members
  int local_size = send_buf.size();
  std::vector<int> all_sizes(cohort_size);
  co_await mpixx::allgather(
    &local_size, 1, MPI_INT, all_sizes.data(), 1, MPI_INT, group->cohort_comm
  );

  // Prepare receive buffer and displacements for allgatherv
  std::vector<int> displs(cohort_size);
  int total_size = 0;
  for (int i = 0; i < cohort_size; i++) {
    displs[i] = total_size;
    total_size += all_sizes[i];
  }
  DataBuffer recv_buf;
  recv_buf.resize(total_size);

  // Gather all subsets - each rank contributes its own index
  co_await mpixx::allgatherv(
    send_buf.data(), local_size, MPI_BYTE, recv_buf.data(), all_sizes.data(),
    displs.data(), MPI_BYTE, group->cohort_comm
  );

  // Deserialize all received subsets back into the vector
  for (int i = 0; i < cohort_size; i++) {
    DataBuffer rank_buf;
    rank_buf.resize(all_sizes[i]);
    std::memcpy(rank_buf.data(), recv_buf.data() + displs[i], all_sizes[i]);
    subsets[i] = DataSubset(rank_buf);
  }

  co_return;
}

tasks::Task<void> DataMember::broadcast_subsets(
  const std::vector<DataSubset>& input, std::vector<DataSubset>& output,
  int root
) {
  int cohort_size, cohort_rank;
  MPI_Comm_size(group->cohort_comm, &cohort_size);
  MPI_Comm_rank(group->cohort_comm, &cohort_rank);

  // Prepare buffer on root
  DataBuffer send_buf;
  int total_size = 0;

  if (cohort_rank == root) {
    // First pack the number of subsets
    int num_subsets = input.size();
    send_buf.resize(sizeof(int));
    std::memcpy(send_buf.data(), &num_subsets, sizeof(int));
    total_size = sizeof(int);

    // Then pack each subset: size + data
    for (const auto& subset : input) {
      DataBuffer subset_buf;
      subset.serialize(subset_buf);
      int subset_size = subset_buf.size();

      // Resize and pack size
      send_buf.resize(total_size + sizeof(int) + subset_size);
      std::memcpy(send_buf.data() + total_size, &subset_size, sizeof(int));
      total_size += sizeof(int);

      // Pack data
      std::memcpy(send_buf.data() + total_size, subset_buf.data(), subset_size);
      total_size += subset_size;
    }
  }

  // Broadcast total size
  co_await mpixx::bcast(&total_size, 1, MPI_INT, root, group->cohort_comm);

  // Allocate buffer on non-root
  if (cohort_rank != root) {
    send_buf.resize(total_size);
  }

  // Broadcast data
  co_await mpixx::bcast(
    send_buf.data(), total_size, MPI_BYTE, root, group->cohort_comm
  );

  // Unpack on all ranks (handles input == output case correctly)
  int offset = 0;
  int num_subsets;
  std::memcpy(&num_subsets, send_buf.data() + offset, sizeof(int));
  offset += sizeof(int);

  output.resize(num_subsets);
  for (int i = 0; i < num_subsets; i++) {
    int subset_size;
    std::memcpy(&subset_size, send_buf.data() + offset, sizeof(int));
    offset += sizeof(int);

    DataBuffer subset_buf;
    subset_buf.resize(subset_size);
    std::memcpy(subset_buf.data(), send_buf.data() + offset, subset_size);
    offset += subset_size;

    output[i] = DataSubset(subset_buf);
  }

  co_return;
}

DataSnapshot& DataMember::current_snapshot() {
  if (!stage_snapshot_) {
    FENIX_THROW(FENIX_ERROR_NODATA_FOUND);
  }
  return *stage_snapshot_;
}

DataSnapshot* DataMember::search_snapshot(int ts, std::source_location loc) {
  DataSnapshot* ret = nullptr;
  if (ts == FENIX_DATA_SNAPSHOT_LATEST) {
    if (!commit_snapshots_.empty()) ret = commit_snapshots_.rbegin()->get();
  } else if (ts < 0) {
    FENIX_THROW_FROM(FENIX_ERROR_INVALID_TIMESTAMP, loc);
  } else {
    auto it = commit_snapshots_.find(ts);
    if (it != commit_snapshots_.end()) ret = it->get();
  }
  return ret;
}

DataSnapshot* DataMember::find_snapshot(int ts, std::source_location loc) {
  DataSnapshot* ret = search_snapshot(ts, loc);
  if (!ret) FENIX_THROW_FROM(FENIX_ERROR_NODATA_FOUND, loc);
  return ret;
}

void DataMember::attr_set(int attr, void* value) {
  switch (attr) {
  case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
    user_data = {(char*)value, user_data.size()};
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT: {
    int new_count = *((int*)value);
    if (new_count != elm_count() &&
        (!commit_snapshots_.empty() ||
         stage_snapshot_->staged_subset() != SUBSET_EMPTY ||
         stage_snapshot_->protected_subset() != SUBSET_EMPTY)) {
      FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
    }
    if (new_count == FENIX_RESIZEABLE) {
      user_data = {user_data.data()};
    } else {
      user_data = {user_data.data(), (size_t)new_count * datatype_.extent()};
    }
    break;
  }
  case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE: {
    MPI_Datatype* dtype = (MPI_Datatype*)value;
    int dtype_size;
    int err = MPI_Type_size(*dtype, &dtype_size);
    if (err) FENIX_THROW("Invalid MPI_Datatype");

    if (dtype_size != datatype_.extent() &&
        (!commit_snapshots_.empty() ||
         stage_snapshot_->staged_subset() != SUBSET_EMPTY ||
         stage_snapshot_->protected_subset() != SUBSET_EMPTY)) {
      FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
    }

    int old_count = elm_count();

    // Replace the datatype with a dup of the new one
    datatype_ = mpixx::Datatype::dup(*dtype);

    if (user_data.is_bounded()) {
      size_t new_size = old_count * dtype_size;
      user_data       = {user_data.data(), new_size};
    }

    // Update stage_snapshot_ element size to match new datatype
    stage_snapshot_->set_elm_size(dtype_size);

    // Update available snapshots element size as well
    for (auto& snap : avail_snapshots_) {
      snap->set_elm_size(dtype_size);
    }

    break;
  }
  default:
    FENIX_THROW(FENIX_ERROR_INVALID_ATTRIBUTE_NAME);
  }
}

void DataMember::attr_get(int attr, void* value) {
  switch (attr) {
  case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
    *((void**)value) = user_data.data();
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
    *((int*)value) = elm_count();
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE: {
    // Duplicated datatype that user must free
    *((MPI_Datatype*)value) = mpixx::Datatype::dup(datatype_).release();
    break;
  }
  case FENIX_DATA_MEMBER_ATTRIBUTE_SIZE:
    *((size_t*)value) = user_data.size();
    break;
  default:
    FENIX_THROW(FENIX_ERROR_INVALID_ATTRIBUTE_NAME);
  }
}

bool DataMember::has_unstored_data() {
  return stage_snapshot_ && stage_snapshot_->staged_subset() != SUBSET_EMPTY;
}

void DataMember::cleanup_timestamps(
  const std::set<int, std::greater<int>>& valid_timestamps
) {
  for (auto it = commit_snapshots_.begin(); it != commit_snapshots_.end();) {
    int snap_ts = (*it)->timestamp();
    auto found  = valid_timestamps.find(snap_ts);
    if (found != valid_timestamps.end()) {
      ++it;
      continue;
    }

    // Not in valid timestamps, extract and move to available pool
    std::unique_ptr<DataSnapshot> snap =
      std::move(commit_snapshots_.extract(it++).value());
    snap->reset();
    avail_snapshots_.push_back(std::move(snap));
  }
}

} //namespace fenix::data
