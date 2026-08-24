#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace fenix::data;

constexpr int my_group = 0;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Create a group
  group_create(my_group, {.depth = 1});

  if (rank == 0)
    fprintf(stderr, "Test 1: Create multiple members and delete one\n");

  // Create 5 members
  std::vector<int> data1(10, 1);
  std::vector<int> data2(10, 2);
  std::vector<int> data3(10, 3);
  std::vector<int> data4(10, 4);
  std::vector<int> data5(10, 5);

  member_create(my_group, 1, data1.data(), 10, MPI_INT);
  member_create(my_group, 2, data2.data(), 10, MPI_INT);
  member_create(my_group, 3, data3.data(), 10, MPI_INT);
  member_create(my_group, 4, data4.data(), 10, MPI_INT);
  member_create(my_group, 5, data5.data(), 10, MPI_INT);

  // Verify all members exist
  fenix_require(member_created(my_group, 1));
  fenix_require(member_created(my_group, 2));
  fenix_require(member_created(my_group, 3));
  fenix_require(member_created(my_group, 4));
  fenix_require(member_created(my_group, 5));

  if (rank == 0) fprintf(stderr, "Test 2: Delete member in middle (id=3)\n");

  // Delete member 3 (in the middle)
  member_delete(my_group, 3);

  // Verify member 3 is gone and others remain
  fenix_require(member_created(my_group, 1));
  fenix_require(member_created(my_group, 2));
  fenix_require(!member_created(my_group, 3)); // Should NOT exist
  fenix_require(member_created(my_group, 4));
  fenix_require(member_created(my_group, 5));

  if (rank == 0) fprintf(stderr, "Test 3: Delete first member (id=1)\n");

  member_delete(my_group, 1);
  fenix_require(!member_created(my_group, 1));
  fenix_require(member_created(my_group, 2));
  fenix_require(member_created(my_group, 4));
  fenix_require(member_created(my_group, 5));

  if (rank == 0) fprintf(stderr, "Test 4: Delete last member (id=5)\n");

  member_delete(my_group, 5);
  fenix_require(!member_created(my_group, 5));
  fenix_require(member_created(my_group, 2));
  fenix_require(member_created(my_group, 4));

  if (rank == 0) fprintf(stderr, "Test 5: Delete remaining members\n");

  member_delete(my_group, 2);
  fenix_require(!member_created(my_group, 2));
  fenix_require(member_created(my_group, 4));

  member_delete(my_group, 4);
  fenix_require(!member_created(my_group, 4));

  if (rank == 0)
    fprintf(
      stderr, "Test 6: Try to delete non-existent member (should fail)\n"
    );

  // This should throw an exception
  bool caught_exception = false;
  try {
    member_delete(my_group, 999);
  } catch (const fenix::RuntimeException& e) {
    caught_exception = true;
    if (rank == 0) fprintf(stderr, "Caught expected exception: %s\n", e.what());
  }
  fenix_require(caught_exception);

  if (rank == 0)
    fprintf(stderr, "Test 7: Add members after deletion and delete again\n");

  // Re-add some members
  member_create(my_group, 10, data1.data(), 10, MPI_INT);
  member_create(my_group, 20, data2.data(), 10, MPI_INT);
  member_create(my_group, 30, data3.data(), 10, MPI_INT);

  fenix_require(member_created(my_group, 10));
  fenix_require(member_created(my_group, 20));
  fenix_require(member_created(my_group, 30));

  // Delete one
  member_delete(my_group, 20);
  fenix_require(member_created(my_group, 10));
  fenix_require(!member_created(my_group, 20));
  fenix_require(member_created(my_group, 30));

  if (rank == 0) fprintf(stderr, "All member_delete tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
