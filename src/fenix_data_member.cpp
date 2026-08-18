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
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"

namespace fenix::data {

fenix_member_entry_packet_t fenix_member_entry_t::to_packet() {
  fenix_member_entry_packet_t to_ret;
  to_ret.memberid      = memberid;
  to_ret.datatype_size = datatype_size;
  to_ret.current_count = elm_count();
  return to_ret;
}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* d, int c, MPI_Datatype dt, int depth,
  std::optional<SerializeFunc> s
)
  : fenix_member_entry_t(id, d, c, __fenix_get_size(dt), depth, s) {}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* data, int count, int dsize, int depth,
  std::optional<SerializeFunc> s
)
  : memberid(id), datatype_size(dsize), depth_(depth) {
  if (count == FENIX_RESIZEABLE) user_data = DataRef((char*)data);
  else user_data = DataRef((char*)data, count * dsize);

  ser_func = s;

  // Note: stage_snapshot_ and avail_snapshots_ are NOT created here
  // because create_snapshot() is virtual and won't dispatch to derived
  // class during base constructor. Derived classes must call init_snapshots().
};

void fenix_member_entry_t::init_snapshots() {
  stage_snapshot_ = this->create_snapshot(datatype_size, elm_count());
  for (int i = 0; i < depth_ + 1; i++) {
    avail_snapshots_.push_back(
      this->create_snapshot(datatype_size, elm_count())
    );
  }
}

fenix_member_entry_t::fenix_member_entry_t(fenix_member_entry_t&& o) {
  fenix_assert(!o.open_serializer);
  memberid   = o.memberid;
  o.memberid = -1;

  datatype_size   = o.datatype_size;
  o.datatype_size = 0;

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
}

int fenix_member_entry_t::elm_count() {
  if (!user_data.is_bounded()) return FENIX_RESIZEABLE;

  fenix_assert(user_data.size() % datatype_size == 0);
  return user_data.size() / datatype_size;
}

void fenix_member_entry_t::serialize(
  const DataSubset& subset, DataBuffer& buf
) {
  subset.copy_data(create_serializer(ser_func, subset, buf));
}

void fenix_member_entry_t::deserialize(
  const DataSubset& subset, DataBuffer& buf, const DataRef& dst
) {
  subset.copy_data(create_deserializer(ser_func, subset, buf, dst));
}

fenix::data::util::Serializer fenix_member_entry_t::create_serializer(
  std::optional<SerializeFunc>& sf, const DataSubset& subset, DataBuffer& buf
) {
  if (open_serializer) {
    if (open_serializer->get_dir() == FENIX_SERIALIZE)
      FENIX_THROW(FENIX_ERROR_MEMBER_STAGING);
    FENIX_THROW(FENIX_ERROR_MEMBER_LOADING);
  }

  DataRef output = user_data;
  if (subset.is_bounded()) {
    output = output.bounded(subset.max_count() * datatype_size);
  }

  if (output.is_bounded()) {
    if (buf.size() < output.size()) buf.resize(output.size());
  } else if (!sf) {
    FENIX_THROW(FENIX_ERROR_INVALID_SUBSET);
  }

  return Serializer(buf, sf, output, FENIX_SERIALIZE, datatype_size);
}

fenix::data::util::Serializer fenix_member_entry_t::create_deserializer(
  std::optional<SerializeFunc>& sf, const DataSubset& subset, DataBuffer& buf,
  const DataRef& dst
) {
  if (open_serializer) {
    if (open_serializer->get_dir() == FENIX_SERIALIZE)
      FENIX_THROW(FENIX_ERROR_MEMBER_STAGING);
    FENIX_THROW(FENIX_ERROR_MEMBER_LOADING);
  }
  return Serializer(buf, sf, dst, FENIX_DESERIALIZE, datatype_size);
}

void fenix_member_entry_t::stage(const DataSubset& subset) {
  if (subset == SUBSET_PRESTAGED) FENIX_THROW("Cannot stage SUBSET_PRESTAGED");

  Snapshot& snap = *stage_snapshot_;
  this->serialize(subset, snap.buf());

  fenix_assert(snap.buf().size() % datatype_size == 0);
  fenix_assert(snap.buf().size() <= user_data.size());

  size_t max_elm = (snap.buf().size() / datatype_size) - 1;
  snap.add_and_fit(subset.bounded(max_elm));
}

void fenix_member_entry_t::stage_inplace(void* buf, const DataSubset& subset) {
  if (subset == SUBSET_PRESTAGED) FENIX_THROW("Cannot stage SUBSET_PRESTAGED");

  Snapshot& snap = *stage_snapshot_;
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

void fenix_member_entry_t::stage_begin(FILE** fp) {
  std::optional<SerializeFunc> sf = SerializeFileFunc{};
  DataBuffer& buf                 = stage_snapshot_->buf();
  buf.resize(0);
  open_serializer.emplace(create_serializer(sf, SUBSET_FULL, buf));
  *fp = open_serializer->get_file();
}

void fenix_member_entry_t::stage_begin(std::iostream** strm) {
  std::optional<SerializeFunc> sf = SerializeStreamFunc{};
  DataBuffer& buf                 = stage_snapshot_->buf();
  buf.resize(0);
  open_serializer.emplace(create_serializer(sf, SUBSET_FULL, buf));
  *strm = open_serializer->get_stream();
}

void fenix_member_entry_t::stage_end() {
  if (!open_serializer) FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
  if (open_serializer->get_dir() != FENIX_SERIALIZE)
    FENIX_THROW(FENIX_ERROR_MEMBER_LOADING);
  open_serializer.reset();

  Snapshot& snap = *stage_snapshot_;
  fenix_assert(snap.buf().size() % datatype_size == 0);
  snap.add_and_fit(DataSubset({0, (snap.buf().size() / datatype_size) - 1}));
}

void fenix_member_entry_t::load_begin(
  FILE** fp, int timestamp, DataSubset& subset
) {
  Snapshot* snap = find_snapshot(timestamp);
  subset         = snap->region();

  std::optional<SerializeFunc> sf = SerializeFileFunc{};
  DataBuffer& buf                 = snap->buf();
  open_serializer.emplace(create_deserializer(sf, SUBSET_FULL, buf, nullptr));
  *fp = open_serializer->get_file();
}

void fenix_member_entry_t::load_begin(
  std::iostream** strm, int timestamp, DataSubset& subset
) {
  Snapshot* snap = find_snapshot(timestamp);
  subset         = snap->region();

  std::optional<SerializeFunc> sf = SerializeStreamFunc{};
  DataBuffer& buf                 = snap->buf();
  open_serializer.emplace(create_deserializer(sf, SUBSET_FULL, buf, nullptr));
  *strm = open_serializer->get_stream();
}

void fenix_member_entry_t::load_end() {
  if (!open_serializer) FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
  if (open_serializer->get_dir() != FENIX_DESERIALIZE)
    FENIX_THROW(FENIX_ERROR_MEMBER_STAGING);
  open_serializer.reset();
}

void fenix_member_entry_t::load(
  void* target, int target_count, int timestamp, DataSubset& data_found
) {
  DataRef dst{(char*)target, static_cast<size_t>(target_count * datatype_size)};
  if (target == FENIX_DATA_RESTORE_INPLACE)
    dst = {user_data.data(), dst.size()};
  if (target_count == FENIX_DATA_RESTORE_FULL) dst = {dst.data()};

  if (timestamp != FENIX_DATA_SNAPSHOT_ALL) {
    Snapshot& snap = *find_snapshot(timestamp);
    data_found     = snap.region();
    if (target_count > 0) deserialize(data_found, snap.buf(), dst);
  } else {
    if (commit_snapshots_.empty()) FENIX_THROW(FENIX_ERROR_NODATA_FOUND);

    data_found = {};
    for (auto it = commit_snapshots_.rbegin(); it != commit_snapshots_.rend();
         ++it) {
      Snapshot& snap = **it;
      fenix_assert(snap.timestamp() >= 0);
      if (target_count > 0) {
        this->deserialize(snap.region() - data_found, snap.buf(), dst);
      }
      data_found += snap.region();
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

int fenix_member_entry_t::store(const DataSubset& subset) {
  // Default: call async version and wait
  return istore(subset).result();
}

int fenix_member_entry_t::storev(const DataSubset& subset) {
  // Default: call async version and wait
  return istorev(subset).result();
}

tasks::Task<int> fenix_member_entry_t::istore(const DataSubset& subset) {
  // Default: No data resilience
  if (subset != SUBSET_PRESTAGED) this->stage(subset);
  co_return FENIX_SUCCESS;
}

tasks::Task<int> fenix_member_entry_t::istorev(const DataSubset& subset) {
  // Default: No data resilience
  if (subset != SUBSET_PRESTAGED) this->stage(subset);
  co_return FENIX_SUCCESS;
}

void fenix_member_entry_t::repair() {
  // Default: no-op repair
  return;
}

void fenix_member_entry_t::commit(int timestamp) {
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
    stage_snapshot_ = this->create_snapshot(datatype_size, elm_count());
  }
}

void fenix_member_entry_t::snapshot_delete(int timestamp) {
  auto it = commit_snapshots_.find(timestamp);
  if (it != commit_snapshots_.end()) {
    snapshot_delete(it);
  }
}

void fenix_member_entry_t::snapshot_delete(CommitIter it) {
  if (it == commit_snapshots_.end()) FENIX_THROW(FENIX_ERROR_INVALID_TIMESTAMP);
  auto node = commit_snapshots_.extract(it);
  node.value()->reset();
  avail_snapshots_.push_back(std::move(node.value()));
}

std::unique_ptr<Snapshot> fenix_member_entry_t::create_snapshot(
  int size, int max_count
) {
  // Default implementation creates a base Snapshot
  // Derived classes override to create policy-specific Entry types
  return std::make_unique<Snapshot>(size, max_count);
}

Snapshot& fenix_member_entry_t::current_snapshot() {
  if (!stage_snapshot_) {
    FENIX_THROW(FENIX_ERROR_NODATA_FOUND);
  }
  return *stage_snapshot_;
}

Snapshot* fenix_member_entry_t::search_snapshot(
  int ts, std::source_location loc
) {
  Snapshot* ret = nullptr;
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

Snapshot* fenix_member_entry_t::find_snapshot(
  int ts, std::source_location loc
) {
  Snapshot* ret = search_snapshot(ts, loc);
  if (!ret) FENIX_THROW_FROM(FENIX_ERROR_NODATA_FOUND, loc);
  return ret;
}

void fenix_member_entry_t::attr_set(int attr, void* value) {
  switch (attr) {
  case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
    user_data = {(char*)value, user_data.size()};
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT: {
    int new_count = *((int*)value);
    if (new_count != elm_count() &&
        (!commit_snapshots_.empty() ||
         stage_snapshot_->region() != SUBSET_EMPTY)) {
      FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
    }
    if (new_count == FENIX_RESIZEABLE) {
      user_data = {user_data.data()};
    } else {
      user_data = {user_data.data(), (size_t)new_count * datatype_size};
    }
    break;
  }
  case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE: {
    MPI_Datatype* dtype = (MPI_Datatype*)value;
    int dtype_size;
    int err = MPI_Type_size(*dtype, &dtype_size);
    if (err) FENIX_THROW("Invalid MPI_Datatype");

    if (dtype_size != datatype_size &&
        (!commit_snapshots_.empty() ||
         stage_snapshot_->region() != SUBSET_EMPTY)) {
      FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
    }

    int old_count = elm_count();
    datatype_size = dtype_size;

    if (user_data.is_bounded()) {
      size_t new_size = old_count * dtype_size;
      user_data       = {user_data.data(), new_size};
    }
    break;
  }
  default:
    FENIX_THROW(FENIX_ERROR_INVALID_ATTRIBUTE_NAME);
  }
}

void fenix_member_entry_t::attr_get(int attr, void* value) {
  switch (attr) {
  case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
    *((void**)value) = user_data.data();
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
    *((int*)value) = elm_count();
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE:
    // Can't reconstruct original MPI_Datatype from size alone
    FENIX_THROW(FENIX_ERROR_INVALID_ATTRIBUTE_NAME);
    break;
  case FENIX_DATA_MEMBER_ATTRIBUTE_SIZE:
    *((size_t*)value) = user_data.size();
    break;
  default:
    FENIX_THROW(FENIX_ERROR_INVALID_ATTRIBUTE_NAME);
  }
}
} //namespace fenix::data
