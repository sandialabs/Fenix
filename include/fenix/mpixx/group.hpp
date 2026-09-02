#ifndef FENIX_MPIXX_GROUP_HPP
#define FENIX_MPIXX_GROUP_HPP

#include <mpi.h>

#include <utility>
#include <vector>

#include "fenix/mpixx/util.hpp"

namespace fenix::mpixx {

// RAII wrapper for MPI_Group with move-only semantics
// Owns an MPI_Group handle and automatically frees it on destruction.
// Does NOT free MPI_GROUP_EMPTY or MPI_GROUP_NULL.
// Accessors check MPI initialization state and return MPI_GROUP_NULL
// if MPI is not initialized or has been finalized.
class Group {
 public:
  // Construct from existing MPI_Group (takes ownership)
  explicit Group(MPI_Group group) noexcept : group_(group) {}

  // Default constructor creates MPI_GROUP_NULL
  Group() noexcept : Group(MPI_GROUP_NULL) {}

  // Destructor automatically frees non-builtin groups
  virtual ~Group() { free(); }

  // Move semantics (frees old group before taking ownership)
  Group& operator=(Group&& o) noexcept;
  Group& operator=(MPI_Group group) { return *this = Group(group); }
  Group(Group&& o) noexcept { *this = std::move(o); }

  // Delete copy operations (move-only)
  Group(const Group&)            = delete;
  Group& operator=(const Group&) = delete;

  // Accessors
  MPI_Group get() const noexcept {
    return mpi_active() ? group_ : MPI_GROUP_NULL;
  }

  // Implicit conversion to MPI_Group
  operator MPI_Group() const noexcept { return get(); }

  explicit operator bool() const noexcept {
    return get() != MPI_GROUP_NULL && get() != MPI_GROUP_EMPTY;
  }

  // Release ownership without freeing
  MPI_Group release() noexcept;

  // MPI group operations
  int size() const;
  int rank() const;

  // Translate ranks from this group to another group
  std::vector<int> translate_ranks(
    const std::vector<int>& ranks1, const Group& group2
  ) const;

  // Translate all ranks from this group to another group
  // Returns vector where result[i] = rank in group2 corresponding to rank i in
  // this group
  std::vector<int> translate_ranks(const Group& group2) const;

  // Translate ranks from this group to another group (single rank)
  int translate_rank(int rank1, const Group& group2) const;

  // Check if this is a builtin group (NULL or EMPTY)
  bool is_builtin() const noexcept;

  // Free the group (safe even if group_ is MPI_GROUP_NULL or builtin)
  virtual void free();

  // ========== Group Comparison ==========

  // Check if two groups are identical (MPI_IDENT)
  bool operator==(const Group& other) const;
  bool operator!=(const Group& other) const { return !(*this == other); }

  // Check if two groups are similar (MPI_SIMILAR - same members, may differ in
  // order)
  bool similar(const Group& other) const;

  // ========== Group Set Operations ==========

  // Union: Create group containing all ranks from both groups
  Group operator+(const Group& other) const;

  // Intersection: Create group containing ranks in both groups
  Group operator|(const Group& other) const;

  // Difference: Create group containing ranks in this but not other
  Group operator-(const Group& other) const;

  // Compound assignment operators (modify this group in place)
  Group& operator+=(const Group& other); // Union and assign
  Group& operator|=(const Group& other); // Intersection and assign
  Group& operator-=(const Group& other); // Difference and assign

  // ========== Group Construction Factory Methods ==========

  // Create group from communicator
  static Group from_comm(MPI_Comm comm);

  // Create group containing specified ranks from source group
  static Group incl(MPI_Group source, const std::vector<int>& ranks);

  // Create group excluding specified ranks from source group
  static Group excl(MPI_Group source, const std::vector<int>& ranks);

  // Create group containing ranks in given range
  // Each range is {first, last, stride}
  static Group range_incl(
    MPI_Group source, const std::vector<std::array<int, 3>>& ranges
  );

  // Create group excluding ranks in given range
  static Group range_excl(
    MPI_Group source, const std::vector<std::array<int, 3>>& ranges
  );

  // Union of two groups
  static Group union_of(const Group& group1, const Group& group2);

  // Intersection of two groups
  static Group intersection(const Group& group1, const Group& group2);

  // Difference of two groups (ranks in group1 but not group2)
  static Group difference(const Group& group1, const Group& group2);

 private:
  MPI_Group group_ = MPI_GROUP_NULL;

  // Helper: Check if a given MPI_Group is builtin
  static bool is_builtin_group(MPI_Group group) noexcept;

  // Helper: Get comparison result between groups
  static int compare(const Group& group1, const Group& group2);
};

// Non-owning reference to an MPI_Group
// Does not free the group on destruction, only releases ownership.
// Useful for storing groups that are owned elsewhere (e.g., builtins).
// Unlike Group, GroupRef is copyable since it doesn't own the resource.
class GroupRef : public Group {
 public:
  // Implicit constructor from MPI_Group
  GroupRef(MPI_Group group = MPI_GROUP_NULL) : Group(group) {}
  GroupRef(const Group& g) : Group(g.get()) {}
  GroupRef(const GroupRef& other) : Group(other.get()) {}

  GroupRef& operator=(const GroupRef& other) {
    *this = other.get();
    return *this;
  }
  GroupRef& operator=(const Group& g) {
    *this = g.get();
    return *this;
  }
  GroupRef& operator=(GroupRef&& other) {
    if (this != &other) {
      (void)release();
      *this = other.release();
    }
    return *this;
  }
  GroupRef& operator=(MPI_Group group) {
    (void)release();
    // Directly assign to base class via Group's assignment operator
    Group::operator=(Group(group));
    return *this;
  }

  ~GroupRef() override { (void)release(); }
};

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_GROUP_HPP
