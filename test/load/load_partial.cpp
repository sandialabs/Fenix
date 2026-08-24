#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace fenix::data;

constexpr int my_group  = 0;
constexpr int my_member = 1;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  if (num_ranks != 2) {
    if (rank == 0)
      fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  group_create(my_group, {.depth = 1});

  if (rank == 0)
    fprintf(stderr, "Test: member_load with smaller target buffer\n");

  // Create data and checkpoint
  std::vector<int> data(100);
  for (int i = 0; i < 100; i++) data[i] = rank * 1000 + i;

  member_create(my_group, my_member, data.data(), 100, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Test loading into a smaller buffer (only first 50 elements)
  if (rank == 0)
    fprintf(stderr, "  Loading first 50 elements into smaller buffer\n");
  std::vector<int> target_buffer(50, -999);
  fenix::DataSubset found_subset;

  int ret =
    member_load(my_group, my_member, target_buffer.data(), 50, 0, found_subset);
  if (ret != FENIX_SUCCESS) {
    fprintf(
      stderr, "Rank %d: ERROR loading into smaller buffer, ret=%d\n", rank, ret
    );
    MPI_Abort(res_comm, 1);
  }

  // Verify first 50 elements were loaded
  for (int i = 0; i < 50; i++) {
    if (target_buffer[i] != rank * 1000 + i) {
      fprintf(
        stderr, "Rank %d: ERROR at [%d], got %d, expected %d\n", rank, i,
        target_buffer[i], rank * 1000 + i
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ First 50 elements loaded correctly\n");

  // Test loading into a larger buffer (should succeed with partial restore
  // warning)
  if (rank == 0)
    fprintf(
      stderr,
      "  Loading into larger buffer (150 elements, checkpoint has 100)\n"
    );
  std::vector<int> large_buffer(150, -888);

  try {
    ret = member_load(
      my_group, my_member, large_buffer.data(), 150, 0, found_subset
    );
  } catch (const fenix::RuntimeException& e) {
    ret = e.error;
  }

  // Should get FENIX_WARNING_PARTIAL_RESTORE (101) since checkpoint only has
  // 100 elements
  if (ret != FENIX_WARNING_PARTIAL_RESTORE) {
    fprintf(
      stderr, "Rank %d: ERROR expected partial restore warning (%d), got %d\n",
      rank, FENIX_WARNING_PARTIAL_RESTORE, ret
    );
    MPI_Abort(res_comm, 1);
  }

  // Verify first 100 elements were loaded
  for (int i = 0; i < 100; i++) {
    if (large_buffer[i] != rank * 1000 + i) {
      fprintf(
        stderr, "Rank %d: ERROR at [%d], got %d, expected %d\n", rank, i,
        large_buffer[i], rank * 1000 + i
      );
      MPI_Abort(res_comm, 1);
    }
  }

  // Verify elements 100-149 were not touched
  for (int i = 100; i < 150; i++) {
    if (large_buffer[i] != -888) {
      fprintf(
        stderr, "Rank %d: ERROR - element [%d] was modified (got %d)\n", rank,
        i, large_buffer[i]
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0)
    fprintf(stderr, "  ✓ Partial restore warning correctly returned\n");

  if (rank == 0) fprintf(stderr, "All member_load buffer size tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
