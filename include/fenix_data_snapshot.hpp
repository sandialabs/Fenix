#ifndef __FENIX_DATA_SNAPSHOT_HPP__
#define __FENIX_DATA_SNAPSHOT_HPP__

#include "fenix_data_buffer.hpp"
#include "fenix_data_subset.hpp"

namespace fenix::data {

// DataSnapshot provides common storage and operations for checkpoint entries
// used by data recovery policies.
// Derived classes can extend with policy-specific storage and operations
// (e.g., partner buffers for redundancy in IMR policies).
class DataSnapshot {
 protected:
  DataBuffer buf_;    ///< Primary checkpoint data buffer
  DataSubset region_; ///< Data region captured in this snapshot
  int timestamp_;     ///< Snapshot version/commit timestamp
  int elm_size_;      ///< Size of each element in bytes
  int elm_max_count_; ///< Maximum number of elements

 public:
  DataSnapshot(int elm_size, int max_count);
  virtual ~DataSnapshot() = default;

  DataSnapshot(DataSnapshot&&)            = default;
  DataSnapshot& operator=(DataSnapshot&&) = default;

  // DataSnapshots are  move-only
  DataSnapshot(const DataSnapshot&)            = delete;
  DataSnapshot& operator=(const DataSnapshot&) = delete;

  // Clear the buffer and region, reset timestamp to -2.
  void reset();

  char* data() { return buf_.data(); }
  int size() const { return buf_.size(); }
  void resize(int size) { buf_.resize(size); }

  // Merges the given subset into the snapshot's region and resizes the
  // buffer if needed to accommodate the expanded region.
  void add_and_fit(const DataSubset& subset);

  // -2 == uninitialized, -1 for staging, >= 0 committed
  int timestamp() const { return timestamp_; }
  void set_timestamp(int ts) { timestamp_ = ts; }

  const DataSubset& region() const { return region_; }
  DataSubset& region() { return region_; }

  DataBuffer& buf() { return buf_; }

  int elm_size() const { return elm_size_; }
  int elm_max_count() const { return elm_max_count_; }
};

// Heterogeneous comparator for snapshot timestamp ordering
// Enables direct lookup by timestamp: commit_snapshots_.find(timestamp)
struct DataSnapshotTimestampComparator {
  using is_transparent = void; // Enables heterogeneous lookup

  bool operator()(
    const std::unique_ptr<DataSnapshot>& a,
    const std::unique_ptr<DataSnapshot>& b
  ) const {
    return a->timestamp() < b->timestamp();
  }

  bool operator()(const std::unique_ptr<DataSnapshot>& a, int timestamp) const {
    return a->timestamp() < timestamp;
  }

  bool operator()(int timestamp, const std::unique_ptr<DataSnapshot>& a) const {
    return timestamp < a->timestamp();
  }
};

} // namespace fenix::data

#endif // __FENIX_DATA_SNAPSHOT_HPP__
