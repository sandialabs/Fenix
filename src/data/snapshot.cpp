#include "fenix.hpp"
#include "fenix/data/snapshot.hpp"
#include "fenix_opt.hpp"

namespace fenix::data {

DataSnapshot::DataSnapshot(int elm_size, int max_count)
  : timestamp_(-2), elm_size_(elm_size), elm_max_count_(max_count) {
  if (max_count != -1) buf_.reserve(elm_size * max_count);
}

DataSnapshot::~DataSnapshot() = default;

void DataSnapshot::reset() {
  timestamp_ = -2;
  buf_.clear();
  protected_subsets.clear();
  staged_subsets.clear();
  cohort_rank = -1;
  cohort_     = mpixx::Group(); // Reset to null group
}

void DataSnapshot::init_cohort(MPI_Comm cohort_comm) {
  if (!cohort_) {
    cohort_ = mpixx::Group::from_comm(cohort_comm);

    // Get cohort size and this rank's position
    int cohort_size = cohort_.size();
    MPI_Comm_rank(cohort_comm, &cohort_rank);

    // Allocate vectors (one per cohort member, including self)
    protected_subsets.resize(cohort_size);
    staged_subsets.resize(cohort_size);
  }
}

void DataSnapshot::reinit_cohort(MPI_Comm cohort_comm) {
  // Get fresh cohort group (automatically frees old one via move assignment)
  cohort_ = mpixx::Group::from_comm(cohort_comm);

  // Get cohort size and this rank's position
  int cohort_size = cohort_.size();
  MPI_Comm_rank(cohort_comm, &cohort_rank);

  // Allocate or resize vectors (one per cohort member, including self)
  protected_subsets.resize(cohort_size);
  staged_subsets.resize(cohort_size);
}

util::Serializer DataSnapshot::create_serializer(
  const util::DataRef& source, std::optional<SerializeFunc>& sf,
  const DataSubset& subset
) {
  util::DataRef output = source;
  if (subset.is_bounded()) {
    output = output.bounded(subset.max_count() * elm_size_);
  }

  if (output.is_bounded()) {
    if (buf_.size() < output.size()) buf_.resize(output.size());
  } else if (!sf) {
    FENIX_THROW(FENIX_ERROR_INVALID_SUBSET);
  }

  return util::Serializer(buf_, sf, output, FENIX_SERIALIZE, elm_size_);
}

util::Serializer DataSnapshot::create_deserializer(
  const util::DataRef& dst, std::optional<SerializeFunc>& sf,
  const DataSubset& subset
) {
  return util::Serializer(buf_, sf, dst, FENIX_DESERIALIZE, elm_size_);
}

void DataSnapshot::add_and_fit(const DataSubset& subset) {
  fenix_assert(subset != SUBSET_PRESTAGED);
  fenix_assert(staged_subset() != SUBSET_PRESTAGED);
  fenix_assert(staged_subset().max_count() > 0 || staged_subset().empty());

  staged_subset() += subset;
  if (elm_max_count_) staged_subset().bound(elm_max_count_ - 1);

  int new_count = elm_max_count_;
  if (!new_count) new_count = staged_subset().max_count();
  fenix_assert(new_count || staged_subset().empty());

  int new_size = new_count * elm_size_;
  if (new_size > buf_.size()) buf_.resize(new_size);
}

} // namespace fenix::data
