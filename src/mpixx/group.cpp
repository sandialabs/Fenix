#include "fenix/mpixx/group.hpp"

#include <mpi.h>

#include <algorithm>
#include <array>
#include <vector>

#include "fenix.h"
#include "fenix_exception.hpp"
#include "fenix_opt.hpp"

namespace fenix::mpixx {

// ========== Move assignment ==========

Group& Group::operator=(Group&& o) noexcept {
  if (this != &o) {
    free();
    group_   = o.group_;
    o.group_ = MPI_GROUP_NULL;
  }
  return *this;
}

// ========== Release ownership ==========

MPI_Group Group::release() noexcept {
  MPI_Group g = group_;
  group_      = MPI_GROUP_NULL;
  return g;
}

// ========== MPI group operations ==========

int Group::size() const {
  int sz;
  int err = MPI_Group_size(get(), &sz);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return sz;
}

int Group::rank() const {
  int r;
  int err = MPI_Group_rank(get(), &r);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return r;
}

std::vector<int> Group::translate_ranks(
  const std::vector<int>& ranks1, const Group& group2
) const {
  if (ranks1.empty()) {
    return {};
  }

  std::vector<int> ranks2(ranks1.size());
  int err = MPI_Group_translate_ranks(
    get(), static_cast<int>(ranks1.size()), ranks1.data(), group2.get(),
    ranks2.data()
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return ranks2;
}

std::vector<int> Group::translate_ranks(const Group& group2) const {
  int sz = size();
  std::vector<int> all_ranks(sz);
  for (int i = 0; i < sz; i++) {
    all_ranks[i] = i;
  }
  return translate_ranks(all_ranks, group2);
}

int Group::translate_rank(int rank1, const Group& group2) const {
  int rank2;
  int err = MPI_Group_translate_ranks(get(), 1, &rank1, group2.get(), &rank2);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return rank2;
}

// ========== Builtin check ==========

bool Group::is_builtin_group(MPI_Group group) noexcept {
  return group == MPI_GROUP_NULL || group == MPI_GROUP_EMPTY;
}

bool Group::is_builtin() const noexcept { return is_builtin_group(group_); }

// ========== Free ==========

void Group::free() {
  if (mpi_active() && !is_builtin()) MPI_Group_free(&group_);
  group_ = MPI_GROUP_NULL;
}

// ========== Comparison ==========

int Group::compare(const Group& group1, const Group& group2) {
  int result;
  int err = MPI_Group_compare(group1.get(), group2.get(), &result);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return result;
}

bool Group::operator==(const Group& other) const {
  return compare(*this, other) == MPI_IDENT;
}

bool Group::similar(const Group& other) const {
  int result = compare(*this, other);
  return result == MPI_IDENT || result == MPI_SIMILAR;
}

// ========== Set operations ==========

Group Group::operator+(const Group& other) const {
  return union_of(*this, other);
}

Group Group::operator|(const Group& other) const {
  return intersection(*this, other);
}

Group Group::operator-(const Group& other) const {
  return difference(*this, other);
}

// ========== Compound assignment operators ==========

Group& Group::operator+=(const Group& other) {
  return *this = union_of(*this, other);
}

Group& Group::operator|=(const Group& other) {
  return *this = intersection(*this, other);
}

Group& Group::operator-=(const Group& other) {
  return *this = difference(*this, other);
}

// ========== Factory methods ==========

Group Group::from_comm(MPI_Comm comm) {
  MPI_Group group;
  int err = MPI_Comm_group(comm, &group);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(group);
}

Group Group::incl(MPI_Group source, const std::vector<int>& ranks) {
  if (ranks.empty()) {
    return Group(MPI_GROUP_EMPTY);
  }

  MPI_Group new_group;
  int err = MPI_Group_incl(
    source, static_cast<int>(ranks.size()), ranks.data(), &new_group
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

Group Group::excl(MPI_Group source, const std::vector<int>& ranks) {
  if (ranks.empty()) {
    // Excluding nothing gives us a copy of the source
    // MPI_Group_excl with n=0 should return a copy, but let's be explicit
    return incl(source, [&]() {
      int size;
      MPI_Group_size(source, &size);
      std::vector<int> all_ranks(size);
      for (int i = 0; i < size; i++) {
        all_ranks[i] = i;
      }
      return all_ranks;
    }());
  }

  MPI_Group new_group;
  int err = MPI_Group_excl(
    source, static_cast<int>(ranks.size()), ranks.data(), &new_group
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

Group Group::range_incl(
  MPI_Group source, const std::vector<std::array<int, 3>>& ranges
) {
  if (ranges.empty()) {
    return Group(MPI_GROUP_EMPTY);
  }

  MPI_Group new_group;
  // MPI expects int[][3], so we need to const_cast (MPI doesn't modify it)
  int err = MPI_Group_range_incl(
    source, static_cast<int>(ranges.size()),
    const_cast<int (*)[3]>(reinterpret_cast<const int (*)[3]>(ranges.data())),
    &new_group
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

Group Group::range_excl(
  MPI_Group source, const std::vector<std::array<int, 3>>& ranges
) {
  if (ranges.empty()) {
    // Excluding nothing gives us a copy - use incl with all ranks
    return incl(source, [&]() {
      int size;
      MPI_Group_size(source, &size);
      std::vector<int> all_ranks(size);
      for (int i = 0; i < size; i++) {
        all_ranks[i] = i;
      }
      return all_ranks;
    }());
  }

  MPI_Group new_group;
  int err = MPI_Group_range_excl(
    source, static_cast<int>(ranges.size()),
    const_cast<int (*)[3]>(reinterpret_cast<const int (*)[3]>(ranges.data())),
    &new_group
  );
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

Group Group::union_of(const Group& group1, const Group& group2) {
  MPI_Group new_group;
  int err = MPI_Group_union(group1.get(), group2.get(), &new_group);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

Group Group::intersection(const Group& group1, const Group& group2) {
  MPI_Group new_group;
  int err = MPI_Group_intersection(group1.get(), group2.get(), &new_group);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

Group Group::difference(const Group& group1, const Group& group2) {
  MPI_Group new_group;
  int err = MPI_Group_difference(group1.get(), group2.get(), &new_group);
  if (err != MPI_SUCCESS) {
    FENIX_THROW(FENIX_ERROR_INTERN);
  }
  return Group(new_group);
}

} // namespace fenix::mpixx
