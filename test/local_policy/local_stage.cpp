#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

constexpr int my_group        = 0;
constexpr int my_member       = 0;
constexpr int start_timestamp = 0;
constexpr int group_depth     = 1;
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

  constexpr int data_size = 100;
  std::vector<int> data(data_size);
  std::vector<int> restored(data_size);

  for (int i = 0; i < data_size; i++) {
    data[i] = rank * 1000 + i;
  }

  Fenix_Data_member_create(
    my_group, my_member, data.data(), data_size, MPI_INT
  );

  member_stage(my_group, my_member, {{0, data_size - 1}});
  member_storev(my_group, my_member, SUBSET_PRESTAGED);
  Fenix_Data_commit_barrier(my_group, NULL);

  DataSubset data_found;
  member_restore(
    my_group, my_member, restored.data(), data_size, start_timestamp, data_found
  );

  bool successful = true;
  for (int i = 0; i < data_size; i++) {
    if (restored[i] != data[i]) {
      fprintf(
        stderr, "FAILURE on rank %d: restored[%d]=%d != data[%d]=%d\n", rank, i,
        restored[i], i, data[i]
      );
      successful = false;
      break;
    }
  }

  if (successful) {
    printf("Rank %d successfully restored data with prestaged subset\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return !successful;
}
