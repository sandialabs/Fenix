#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

constexpr int kKillID   = 2;
constexpr int my_group  = 0;
constexpr int my_member = 0;
int errflag;

using fenix::DataSubset;
using namespace fenix::data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm, .spares = 1});

  // Policy values to be used for group_create calls
  int policy_vals[] = {5, 1, 3};

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Use fixed size data (same for all ranks) for parity mode
  constexpr int kDataSize = 100;
  std::vector<int> data(kDataSize);

  try {
    if (fenix::role() == fenix::RECOVERED_RANK) fenix::throw_exception();

    // Initialize with rank-specific data
    for (int i = 0; i < 100; i++) {
      data[i] = rank * 1000 + i;
    }

    // Use IMR mode 5 (parity) with set size of 3, rank separation of 1
    // With 6 ranks and set_size=3, we get 2 sets of 3 ranks each
    group_create(my_group, {.policy_value = policy_vals});
    member_create(my_group, my_member, data.data(), kDataSize, MPI_INT);
    member_store(my_group, my_member);
    commit_barrier(my_group);

    if (rank == kKillID) {
      fprintf(stderr, "Doing kill on node %d\n", rank);
      raise(SIGTERM);
    }

    MPI_Barrier(res_comm);
  } catch (const fenix::CommException& e) {
    //We've had a failure! Time to recover data.
    fprintf(stderr, "Starting data recovery on rank %d\n", rank);
    fenix_require(e.fenix_err == FENIX_SUCCESS);

    group_create(my_group, {.policy_value = policy_vals});
    member_define(my_group, my_member, data.data(), kDataSize, MPI_INT);
    member_repair(my_group, my_member);
    member_load(my_group, my_member);

    //Validate restored data
    bool successful   = true;
    int expected_base = rank * 1000;
    for (int i = 0; i < kDataSize && successful; i++) {
      if (data[i] != rank * 1000 + i) {
        fprintf(
          stderr, "FAILURE rank %d: data[%d] = %d, expected %d\n", rank, i,
          data[i], expected_base + i
        );
        successful = false;
      }
    }
    fenix_require(successful);

    fprintf(stderr, "SUCCESS: Data restored correctly on rank %d\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
