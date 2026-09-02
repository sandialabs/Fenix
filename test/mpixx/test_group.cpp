/**
 * @file mpixx_group_test.cpp
 * @brief Thorough test suite for fenix::mpixx::Group RAII wrapper
 */

#include <mpi.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#include "fenix_opt.hpp"
#include "fenix/mpixx/comm.hpp"
#include "fenix/mpixx/group.hpp"

using namespace fenix::mpixx;

void test_basic_construction() {
  printf("Test: basic construction\n");

  // Default constructor creates NULL group
  Group g1;
  fenix_require(
    g1.get() == MPI_GROUP_NULL, "default constructor should create NULL"
  );
  fenix_require(!g1, "NULL group should be falsy");

  // Construct from MPI_GROUP_EMPTY
  Group g2(MPI_GROUP_EMPTY);
  fenix_require(g2.get() == MPI_GROUP_EMPTY, "should store EMPTY group");
  fenix_require(!g2, "EMPTY group should be falsy");

  // Get group from COMM_WORLD
  Group world_group = Group::from_comm(MPI_COMM_WORLD);
  fenix_require(world_group, "COMM_WORLD group should be truthy");
  fenix_require(world_group.get() != MPI_GROUP_NULL, "should not be NULL");
  fenix_require(world_group.get() != MPI_GROUP_EMPTY, "should not be EMPTY");

  printf("  PASSED\n");
}

void test_group_properties() {
  printf("Test: group properties (size, rank)\n");

  Group world_group = Group::from_comm(MPI_COMM_WORLD);

  int world_size, world_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  fenix_require(
    world_group.size() == world_size, "group size should match comm size"
  );
  fenix_require(
    world_group.rank() == world_rank, "group rank should match comm rank"
  );

  printf("  PASSED\n");
}

void test_move_semantics() {
  printf("Test: move semantics\n");

  Group g1            = Group::from_comm(MPI_COMM_WORLD);
  MPI_Group raw_group = g1.get();

  // Move constructor
  Group g2(std::move(g1));
  fenix_require(g1.get() == MPI_GROUP_NULL, "moved-from should be NULL");
  fenix_require(g2.get() == raw_group, "moved-to should have original handle");

  // Move assignment
  Group g3;
  g3 = std::move(g2);
  fenix_require(g2.get() == MPI_GROUP_NULL, "moved-from should be NULL");
  fenix_require(g3.get() == raw_group, "moved-to should have original handle");

  printf("  PASSED\n");
}

void test_release() {
  printf("Test: release ownership\n");

  Group g       = Group::from_comm(MPI_COMM_WORLD);
  MPI_Group raw = g.release();

  fenix_require(
    g.get() == MPI_GROUP_NULL, "after release, group should be NULL"
  );
  fenix_require(raw != MPI_GROUP_NULL, "released handle should be valid");

  // Must manually free the released group
  MPI_Group_free(&raw);

  printf("  PASSED\n");
}

void test_comparison() {
  printf("Test: comparison (identical and similar)\n");

  Group world1 = Group::from_comm(MPI_COMM_WORLD);
  Group world2 = Group::from_comm(MPI_COMM_WORLD);

  // Different handles to same group should be IDENT
  fenix_require(world1 == world2, "two COMM_WORLD groups should be identical");
  fenix_require(
    world1.similar(world2), "identical groups should also be similar"
  );

  int world_size = world1.size();
  if (world_size > 1) {
    // Create reordered group
    std::vector<int> ranks(world_size);
    for (int i = 0; i < world_size; i++) {
      ranks[i] = (world_size - 1) - i; // Reverse order
    }
    Group reversed = Group::incl(world1, ranks);

    fenix_require(
      !(world1 == reversed), "original and reversed should not be identical"
    );
    fenix_require(
      world1.similar(reversed),
      "original and reversed should be similar (same members)"
    );
  }

  printf("  PASSED\n");
}

void test_incl_excl() {
  printf("Test: incl and excl\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 2) {
    // Include first two ranks
    std::vector<int> first_two = {0, 1};
    Group g_incl               = Group::incl(world, first_two);
    fenix_require(g_incl.size() == 2, "incl group should have 2 members");

    // Exclude first rank
    std::vector<int> first = {0};
    Group g_excl           = Group::excl(world, first);
    fenix_require(
      g_excl.size() == size - 1, "excl group should have size-1 members"
    );
  }

  // Empty incl creates EMPTY group
  std::vector<int> empty;
  Group g_empty = Group::incl(world, empty);
  fenix_require(
    g_empty.get() == MPI_GROUP_EMPTY,
    "incl with empty ranks should give EMPTY group"
  );

  printf("  PASSED\n");
}

void test_range_incl_excl() {
  printf("Test: range_incl and range_excl\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 4) {
    // Include every other rank: 0, 2, 4, ...
    std::vector<std::array<int, 3>> ranges = {{0, size - 1, 2}};
    Group g_even                           = Group::range_incl(world, ranges);

    int expected_size = (size + 1) / 2; // Ceiling division
    fenix_require(
      g_even.size() == expected_size,
      "range_incl should include every other rank"
    );

    // Exclude first half
    std::vector<std::array<int, 3>> first_half = {{0, size / 2 - 1, 1}};
    Group g_second_half = Group::range_excl(world, first_half);
    fenix_require(
      g_second_half.size() == size - size / 2,
      "range_excl should exclude first half"
    );
  }

  printf("  PASSED\n");
}

void test_union() {
  printf("Test: union (+ operator)\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 4) {
    // Create two disjoint groups
    std::vector<int> first_half, second_half;
    for (int i = 0; i < size / 2; i++) {
      first_half.push_back(i);
    }
    for (int i = size / 2; i < size; i++) {
      second_half.push_back(i);
    }

    Group g1 = Group::incl(world, first_half);
    Group g2 = Group::incl(world, second_half);

    // Union using operator+
    Group g_union = g1 + g2;
    fenix_require(g_union.size() == size, "union should contain all ranks");
    fenix_require(g_union.similar(world), "union should be similar to world");

    // Union using static method
    Group g_union2 = Group::union_of(g1, g2);
    fenix_require(g_union2.size() == size, "union_of should contain all ranks");
  }

  printf("  PASSED\n");
}

void test_intersection() {
  printf("Test: intersection (| operator)\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 4) {
    // Create two overlapping groups
    std::vector<int> first_three_quarters, last_three_quarters;
    for (int i = 0; i < (3 * size) / 4; i++) {
      first_three_quarters.push_back(i);
    }
    for (int i = size / 4; i < size; i++) {
      last_three_quarters.push_back(i);
    }

    Group g1 = Group::incl(world, first_three_quarters);
    Group g2 = Group::incl(world, last_three_quarters);

    // Intersection using operator|
    Group g_inter = g1 | g2;

    // Intersection should be middle half
    int expected_size = size / 2;
    fenix_require(
      g_inter.size() == expected_size, "intersection should contain middle half"
    );

    // Intersection using static method
    Group g_inter2 = Group::intersection(g1, g2);
    fenix_require(
      g_inter2.size() == expected_size,
      "intersection should contain middle half"
    );
  }

  printf("  PASSED\n");
}

void test_difference() {
  printf("Test: difference (- operator)\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 3) {
    // Create group with first two ranks
    std::vector<int> first_two = {0, 1};
    Group g_small              = Group::incl(world, first_two);

    // Difference: world - small = ranks 2, 3, ...
    Group g_diff = world - g_small;
    fenix_require(
      g_diff.size() == size - 2, "difference should exclude first two ranks"
    );

    // Difference using static method
    Group g_diff2 = Group::difference(world, g_small);
    fenix_require(
      g_diff2.size() == size - 2, "difference should exclude first two ranks"
    );

    // Verify rank 0 is not in difference
    Group g_zero       = Group::incl(world, {0});
    Group intersection = g_diff | g_zero;
    fenix_require(
      intersection.get() == MPI_GROUP_EMPTY,
      "rank 0 should not be in difference"
    );
  }

  printf("  PASSED\n");
}

void test_translate_ranks() {
  printf("Test: translate_ranks\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 2) {
    // Create reversed group
    std::vector<int> ranks(size);
    for (int i = 0; i < size; i++) {
      ranks[i] = (size - 1) - i;
    }
    Group reversed = Group::incl(world, ranks);

    // Translate rank 0 from world to reversed
    // In world: rank 0 is rank 0
    // In reversed: rank 0 (which is actually old rank size-1) should map to...
    // Wait, we need to translate FROM world TO reversed
    // Rank 0 in world corresponds to which rank in reversed?
    // reversed is built from world with ranks [size-1, size-2, ..., 1, 0]
    // So rank 0 in world appears at position (size-1) in reversed

    int translated = world.translate_rank(0, reversed);
    fenix_require(
      translated == size - 1,
      "rank 0 in world should be rank size-1 in reversed"
    );

    // Translate multiple ranks
    std::vector<int> world_ranks = {0, 1};
    std::vector<int> reversed_ranks =
      world.translate_ranks(world_ranks, reversed);
    fenix_require(reversed_ranks.size() == 2, "should translate two ranks");
    fenix_require(
      reversed_ranks[0] == size - 1, "rank 0 should translate to size-1"
    );
    fenix_require(
      reversed_ranks[1] == size - 2, "rank 1 should translate to size-2"
    );

    // Translate all ranks using convenience method
    std::vector<int> all_reversed = world.translate_ranks(reversed);
    fenix_require(all_reversed.size() == size, "should translate all ranks");
    for (int i = 0; i < size; i++) {
      fenix_require(
        all_reversed[i] == size - 1 - i, "all ranks should be reversed"
      );
    }
  }

  printf("  PASSED\n");
}

void test_group_ref() {
  printf("Test: GroupRef (non-owning reference)\n");

  Group world         = Group::from_comm(MPI_COMM_WORLD);
  MPI_Group raw_group = world.get();

  // Create GroupRef from Group
  GroupRef ref1(world);
  fenix_require(
    ref1.get() == raw_group, "GroupRef should reference same handle"
  );

  // Copy GroupRef (should be allowed)
  GroupRef ref2 = ref1;
  fenix_require(
    ref2.get() == raw_group, "copied GroupRef should have same handle"
  );

  // Original world Group should still be valid after refs go out of scope
  fenix_require(
    world.get() == raw_group, "original Group should still own handle"
  );

  printf("  PASSED\n");
}

void test_comm_group_method() {
  printf("Test: Comm::group() method\n");

  CommRef world_comm(MPI_COMM_WORLD);

  // Get group using Comm method
  Group g = world_comm.group();
  fenix_require(g, "Comm::group() should return valid group");

  int comm_size, group_size;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  group_size = g.size();
  fenix_require(
    group_size == comm_size, "group from Comm should match comm size"
  );

  // Also test static version
  Group g2 = Comm::group(MPI_COMM_WORLD);
  fenix_require(g2, "Comm::group(MPI_Comm) should return valid group");
  fenix_require(g2.size() == comm_size, "static version should also work");

  printf("  PASSED\n");
}

void test_complex_operations() {
  printf("Test: complex operations (combining multiple ops)\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 6) {
    // Create three groups: first third, middle third, last third
    int third = size / 3;
    std::vector<int> first_third, middle_third, last_third;

    for (int i = 0; i < third; i++) {
      first_third.push_back(i);
    }
    for (int i = third; i < 2 * third; i++) {
      middle_third.push_back(i);
    }
    for (int i = 2 * third; i < size; i++) {
      last_third.push_back(i);
    }

    Group g1 = Group::incl(world, first_third);
    Group g2 = Group::incl(world, middle_third);
    Group g3 = Group::incl(world, last_third);

    // Union of first and last thirds
    Group g1_and_g3 = g1 + g3;

    // Difference: world minus middle third should equal (first + last)
    Group world_minus_middle = world - g2;
    fenix_require(
      world_minus_middle.similar(g1_and_g3),
      "world - middle should equal first + last"
    );

    // Intersection of (first+middle) and (middle+last) should be middle
    Group g1_and_g2    = g1 + g2;
    Group g2_and_g3    = g2 + g3;
    Group intersection = g1_and_g2 | g2_and_g3;
    fenix_require(
      intersection.similar(g2),
      "intersection of overlapping unions should be middle"
    );
  }

  printf("  PASSED\n");
}

void test_empty_group_operations() {
  printf("Test: operations on empty groups\n");

  Group empty(MPI_GROUP_EMPTY);
  fenix_require(empty.size() == 0, "empty group should have size 0");

  Group world = Group::from_comm(MPI_COMM_WORLD);

  // Union with empty
  Group union_empty = world + empty;
  fenix_require(
    union_empty.similar(world), "union with empty should equal world"
  );

  // Intersection with empty
  Group inter_empty = world | empty;
  fenix_require(
    inter_empty.get() == MPI_GROUP_EMPTY,
    "intersection with empty should be empty"
  );

  // Difference with empty
  Group diff_empty = world - empty;
  fenix_require(
    diff_empty.similar(world), "difference with empty should equal world"
  );

  printf("  PASSED\n");
}

void test_compound_assignment() {
  printf("Test: compound assignment operators (+=, -=, |=)\n");

  Group world = Group::from_comm(MPI_COMM_WORLD);
  int size    = world.size();

  if (size >= 4) {
    // Create two disjoint groups
    std::vector<int> first_half, second_half;
    for (int i = 0; i < size / 2; i++) {
      first_half.push_back(i);
    }
    for (int i = size / 2; i < size; i++) {
      second_half.push_back(i);
    }

    // Test += (union)
    Group g1          = Group::incl(world, first_half);
    Group g2          = Group::incl(world, second_half);
    int original_size = g1.size();
    g1 += g2;
    fenix_require(g1.size() == size, "+= should union groups");
    fenix_require(g1.similar(world), "after +=, should be similar to world");

    // Test |= (intersection)
    // Create overlapping groups
    std::vector<int> first_three_quarters, last_three_quarters;
    for (int i = 0; i < (3 * size) / 4; i++) {
      first_three_quarters.push_back(i);
    }
    for (int i = size / 4; i < size; i++) {
      last_three_quarters.push_back(i);
    }

    Group g3 = Group::incl(world, first_three_quarters);
    Group g4 = Group::incl(world, last_three_quarters);
    g3 |= g4;
    int expected_size = size / 2;
    fenix_require(
      g3.size() == expected_size, "|= should intersect groups (middle half)"
    );

    // Test -= (difference)
    Group g5 = Group::from_comm(MPI_COMM_WORLD);
    Group g6 = Group::incl(world, {0, 1});
    g5 -= g6;
    fenix_require(g5.size() == size - 2, "-= should remove specified ranks");
  }

  printf("  PASSED\n");
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    printf("=== mpixx::Group Test Suite ===\n");
  }

  test_basic_construction();
  test_group_properties();
  test_move_semantics();
  test_release();
  test_comparison();
  test_incl_excl();
  test_range_incl_excl();
  test_union();
  test_intersection();
  test_difference();
  test_translate_ranks();
  test_group_ref();
  test_comm_group_method();
  test_complex_operations();
  test_empty_group_operations();
  test_compound_assignment();

  if (rank == 0) {
    printf("=== All tests PASSED ===\n");
  }

  MPI_Finalize();
  return 0;
}
