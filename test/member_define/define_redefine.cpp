#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace fenix::data;

constexpr int my_group = 0;
constexpr int my_member = 1;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  if (num_ranks != 2) {
    if (rank == 0) fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  group_create(my_group, {.depth = 2});

  if (rank == 0) fprintf(stderr, "Test: member_define and redefine\n");

  // Test 1: Define creates new member if it doesn't exist
  if (rank == 0) fprintf(stderr, "  Defining new member\n");
  std::vector<int> data(50, rank * 100);

  member_define(my_group, my_member, data.data(), 50, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Test 2: Redefine modifies existing member (same count/type)
  if (rank == 0) fprintf(stderr, "  Redefining with same size\n");
  for (int i = 0; i < 50; i++) data[i] = rank * 200 + i;

  member_define(my_group, my_member, data.data(), 50, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Verify first checkpoint has old data
  std::fill(data.begin(), data.end(), -1);
  member_restore(my_group, my_member, FENIX_DATA_RESTORE_INPLACE,
                 FENIX_DATA_RESTORE_FULL, 0);

  for (int i = 0; i < 50; i++) {
    if (data[i] != rank * 100) {
      fprintf(stderr, "Rank %d: ERROR at timestamp 0, [%d]=%d (expected %d)\n",
              rank, i, data[i], rank * 100);
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ First checkpoint preserved\n");

  // Verify second checkpoint has new data
  std::fill(data.begin(), data.end(), -1);
  member_restore(my_group, my_member, FENIX_DATA_RESTORE_INPLACE,
                 FENIX_DATA_RESTORE_FULL, 1);

  for (int i = 0; i < 50; i++) {
    if (data[i] != rank * 200 + i) {
      fprintf(stderr, "Rank %d: ERROR at timestamp 1, [%d]=%d (expected %d)\n",
              rank, i, data[i], rank * 200 + i);
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Second checkpoint has redefined data\n");

  if (rank == 0) fprintf(stderr, "All member_define redefine tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
