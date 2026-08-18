#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

constexpr int my_group        = 0;
constexpr int my_member       = 0;
constexpr int start_timestamp = 0;
constexpr int group_depth     = 3;
int errflag;

using fenix::DataSubset;
using namespace fenix::data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  Fenix_Data_group_create(
    my_group, res_comm, start_timestamp, group_depth, FENIX_DATA_POLICY_LOCAL,
    NULL, &errflag
  );

  constexpr int data_size = 50;
  std::vector<int> data(data_size);
  std::vector<int> restored(data_size);

  Fenix_Data_member_create(
    my_group, my_member, data.data(), data_size, MPI_INT
  );

  for (int iter = 0; iter < 5; iter++) {
    for (int i = 0; i < data_size; i++) {
      data[i] = rank * 10000 + iter * 1000 + i;
    }

    Fenix_Data_member_store(my_group, my_member, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_commit(my_group, NULL);
  }

  int num_snapshots;
  Fenix_Data_group_get_number_of_snapshots(my_group, &num_snapshots);

  if (num_snapshots != group_depth + 1) {
    fprintf(
      stderr, "FAILURE on rank %d: expected %d snapshots, got %d\n", rank,
      group_depth + 1, num_snapshots
    );
    Fenix_Finalize();
    MPI_Finalize();
    return 1;
  }

  bool successful = true;
  for (int s = 0; s < num_snapshots; s++) {
    int timestamp;
    Fenix_Data_group_get_snapshot_at_position(my_group, s, &timestamp);

    // Timestamps are newest→oldest, so position 0 = newest (iter 4), position 3
    // = oldest (iter 1)
    int expected_iter =
      (5 - 1) - s; // Last iteration is 4, decrement by position

    DataSubset data_found;
    member_restore(
      my_group, my_member, restored.data(), data_size, timestamp, data_found
    );

    for (int i = 0; i < data_size; i++) {
      int expected = rank * 10000 + expected_iter * 1000 + i;
      if (restored[i] != expected) {
        fprintf(
          stderr,
          "FAILURE on rank %d snapshot %d: restored[%d]=%d != expected=%d\n",
          rank, s, i, restored[i], expected
        );
        successful = false;
        break;
      }
    }
    if (!successful) break;
  }

  if (successful) {
    printf("Rank %d successfully validated depth limiting\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return !successful;
}
