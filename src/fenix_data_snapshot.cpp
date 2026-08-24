#include "fenix.hpp"
#include "fenix_data_snapshot.hpp"
#include "fenix_opt.hpp"

namespace fenix::data {

DataSnapshot::DataSnapshot(int elm_size, int max_count)
  : timestamp_(-2), elm_size_(elm_size), elm_max_count_(max_count) {
  if (max_count != -1) buf_.reserve(elm_size * max_count);
}

void DataSnapshot::reset() {
  timestamp_ = -2;
  buf_.clear();
  region_ = {};
}

void DataSnapshot::add_and_fit(const DataSubset& subset) {
  fenix_assert(subset != SUBSET_PRESTAGED);
  fenix_assert(region_ != SUBSET_PRESTAGED);
  fenix_assert(region_.max_count() > 0 || region_.empty());

  region_ += subset;
  if (elm_max_count_) region_.bound(elm_max_count_ - 1);

  int new_count = elm_max_count_;
  if (!new_count) new_count = region_.max_count();
  fenix_assert(new_count || region_.empty());

  int new_size = new_count * elm_size_;
  if (new_size > buf_.size()) buf_.resize(new_size);
}

} // namespace fenix::data
