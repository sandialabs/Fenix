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

  group_create(my_group, {.depth = 2});

  if (rank == 0) fprintf(stderr, "Test: member_load with target buffer\n");

  // Create initial data and checkpoint
  std::vector<int> data(100);
  for (int i = 0; i < 100; i++) data[i] = rank * 1000 + i;

  member_create(my_group, my_member, data.data(), 100, MPI_INT);
  checkpoint(my_group, SUBSET_FULL);

  // Modify data and create second checkpoint
  for (int i = 0; i < 100; i++) data[i] = rank * 2000 + i;
  checkpoint(my_group, SUBSET_FULL);

  // Test 1: Load latest checkpoint into a different buffer
  if (rank == 0)
    fprintf(
      stderr, "  Loading latest checkpoint (timestamp 1) into target buffer\n"
    );
  std::vector<int> target_buffer(100, -1);
  fenix::DataSubset found_subset;

  int ret = member_load(
    my_group, my_member, target_buffer.data(), 100, 1, found_subset
  );
  if (ret != FENIX_SUCCESS) {
    fprintf(stderr, "Rank %d: ERROR in member_load, ret=%d\n", rank, ret);
    MPI_Abort(res_comm, 1);
  }

  // Verify target buffer has data from second checkpoint
  for (int i = 0; i < 100; i++) {
    if (target_buffer[i] != rank * 2000 + i) {
      fprintf(
        stderr, "Rank %d: ERROR in target buffer[%d], got %d, expected %d\n",
        rank, i, target_buffer[i], rank * 2000 + i
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Loaded timestamp 1 correctly\n");

  // Test 2: Load from timestamp 0 (first checkpoint) into target buffer
  if (rank == 0)
    fprintf(
      stderr, "  Loading first checkpoint (timestamp 0) into target buffer\n"
    );
  std::fill(target_buffer.begin(), target_buffer.end(), -1);

  ret = member_load(
    my_group, my_member, target_buffer.data(), 100, 0, found_subset
  );
  if (ret != FENIX_SUCCESS) {
    fprintf(stderr, "Rank %d: ERROR loading timestamp 0, ret=%d\n", rank, ret);
    MPI_Abort(res_comm, 1);
  }

  // Verify target buffer has data from first checkpoint
  for (int i = 0; i < 100; i++) {
    if (target_buffer[i] != rank * 1000 + i) {
      fprintf(
        stderr,
        "Rank %d: ERROR loading timestamp 0 at [%d], got %d, expected %d\n",
        rank, i, target_buffer[i], rank * 1000 + i
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Loaded timestamp 0 correctly\n");

  // Test 3: Verify original data buffer was not modified
  for (int i = 0; i < 100; i++) {
    if (data[i] != rank * 2000 + i) {
      fprintf(
        stderr, "Rank %d: ERROR - original data was modified at [%d]\n", rank, i
      );
      MPI_Abort(res_comm, 1);
    }
  }

  if (rank == 0) fprintf(stderr, "  ✓ Original data buffer unmodified\n");

  if (rank == 0)
    fprintf(stderr, "All member_load target buffer tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
