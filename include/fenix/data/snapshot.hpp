#ifndef __FENIX_DATA_SNAPSHOT_HPP__
#define __FENIX_DATA_SNAPSHOT_HPP__

#include <mpi.h>
#include <optional>
#include "fenix/data/util/buffer.hpp"
#include "fenix/data/util/data_ref.hpp"
#include "fenix/data/util/serializer.hpp"
#include "fenix/data/subset.hpp"

namespace fenix::data {

// DataSnapshot provides common storage and operations for checkpoint entries
// used by data recovery policies.
// Derived classes can extend with policy-specific storage and operations
// (e.g., partner buffers for redundancy in IMR policies).
class DataSnapshot {
 protected:
  util::DataBuffer buf_; ///< Primary checkpoint data buffer
  int timestamp_;        ///< Snapshot version/commit timestamp
  int elm_size_;         ///< Size of each element in bytes
  int elm_max_count_;    ///< Maximum number of elements

  MPI_Group cohort_ = MPI_GROUP_NULL;

 public:
  int cohort_rank = -1; ///< This rank's index within the cohort

  std::vector<DataSubset> protected_subsets; ///< Subsets protected by each
                                             ///< cohort member (including self)
  std::vector<DataSubset>
    staged_subsets; ///< Subsets staged by each cohort member (including self)

  DataSnapshot(int elm_size, int max_count);
  virtual ~DataSnapshot();

  DataSnapshot(DataSnapshot&&)            = default;
  DataSnapshot& operator=(DataSnapshot&&) = default;

  // DataSnapshots are move-only
  DataSnapshot(const DataSnapshot&)            = delete;
  DataSnapshot& operator=(const DataSnapshot&) = delete;

  // Clear the buffer and region, reset timestamp to -2, cohort to
  // MPI_GROUP_NULL.
  void reset();

  // Called when snapshot is first staged to - sets cohort and allocates
  // storage for tracking subsets per cohort partner
  virtual void init_cohort(MPI_Comm cohort_comm);

  // Called during repair - replaces cohort if already initialized, or
  // initializes if not
  virtual void reinit_cohort(MPI_Comm cohort_comm);

  // Create a serializer that writes to this snapshot's buffer from the source
  util::Serializer create_serializer(
    const util::DataRef& source, std::optional<SerializeFunc>& sf,
    const DataSubset& subset
  );

  // Create a deserializer that reads from this snapshot's buffer to destination
  util::Serializer create_deserializer(
    const util::DataRef& dst, std::optional<SerializeFunc>& sf,
    const DataSubset& subset
  );

  char* data() { return buf_.data(); }
  int size() const { return buf_.size(); }
  void resize(int size) { buf_.resize(size); }

  // Merges the given subset into the snapshot's region and resizes the
  // buffer if needed to accommodate the expanded region.
  void add_and_fit(const DataSubset& subset);

  // -2 == uninitialized, -1 for staging, >= 0 committed
  int timestamp() const { return timestamp_; }
  void set_timestamp(int ts) { timestamp_ = ts; }

  const DataSubset& staged_subset() const {
    return staged_subsets[cohort_rank];
  }
  DataSubset& staged_subset() { return staged_subsets[cohort_rank]; }

  const DataSubset& protected_subset() const {
    return protected_subsets[cohort_rank];
  }
  DataSubset& protected_subset() { return protected_subsets[cohort_rank]; }

  util::DataBuffer& buf() { return buf_; }

  int elm_size() const { return elm_size_; }
  void set_elm_size(int size) { elm_size_ = size; }
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
