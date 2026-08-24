#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

constexpr int my_group        = 0;
constexpr int my_member       = 0;
constexpr int start_timestamp = 0;
constexpr int group_depth     = 2;
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

  std::vector<int> data;

  Fenix_Data_member_create(
    my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
  );

  data.resize(100 + rank);
  for (int i = 0; i < data.size(); i++) {
    data[i] = rank * 10000 + i;
  }

  Fenix_Data_member_attr_set(
    my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
    &errflag
  );
  member_stage(my_group, my_member, {{0, data.size() - 1}});
  member_storev(my_group, my_member, SUBSET_PRESTAGED);
  Fenix_Data_commit_barrier(my_group, NULL);

  int first_size = data.size();

  data.resize(50 + rank);
  for (int i = 0; i < data.size(); i++) {
    data[i] = rank * 20000 + i;
  }

  Fenix_Data_member_attr_set(
    my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
    &errflag
  );
  member_stage(my_group, my_member, {{0, data.size() - 1}});
  member_storev(my_group, my_member, SUBSET_PRESTAGED);
  Fenix_Data_commit_barrier(my_group, NULL);

  int second_size = data.size();

  std::vector<int> restored(200);
  DataSubset data_found;

  member_restore(
    my_group, my_member, restored.data(), FENIX_DATA_RESTORE_FULL,
    start_timestamp + 1, data_found
  );

  bool successful = true;
  if (data_found.max_count() != second_size) {
    fprintf(
      stderr,
      "FAILURE on rank %d: expected size %d, got %d for second snapshot\n",
      rank, second_size, (int)(data_found.max_count())
    );
    successful = false;
  } else {
    for (int i = 0; i < second_size; i++) {
      int expected = rank * 20000 + i;
      if (restored[i] != expected) {
        fprintf(
          stderr, "FAILURE on rank %d: restored[%d]=%d != expected=%d\n", rank,
          i, restored[i], expected
        );
        successful = false;
        break;
      }
    }
  }

  if (successful) {
    printf("Rank %d successfully validated resizeable member\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return !successful;
}
