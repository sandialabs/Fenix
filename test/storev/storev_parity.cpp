#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

constexpr int kKillID         = 2;
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
  fenix::init({.out_comm = &res_comm, .spares = 1});

  // Policy values to be used for group_create calls
  int policy_vals[] = {5, 1, 3};

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  std::vector<int> data(100);

  try {
    if (fenix::role() == fenix::RECOVERED_RANK) fenix::throw_exception();

    // Initialize with rank-specific data
    for (int i = 0; i < 100; i++) {
      data[i] = rank * 1000 + i;
    }

    // Use IMR mode 5 (parity) with set size of 3, rank separation of 1
    // With 6 ranks and set_size=3, we get 2 sets of 3 ranks each
    Fenix_Data_group_create(
      my_group, res_comm, start_timestamp, group_depth, FENIX_DATA_POLICY_IMR,
      policy_vals, &errflag
    );

    Fenix_Data_member_create(
      my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
    );

    // Each rank stores a different-sized subset
    int subset_start, subset_end;
    if (rank < 3) {
      subset_start = rank * 20;
      subset_end   = subset_start + 19;
    } else if (rank < 5) {
      subset_start = 60 + (rank - 3) * 15;
      subset_end   = subset_start + 14;
    } else {
      subset_start = 90;
      subset_end   = 99;
    }

    fprintf(
      stderr, "Rank %d storing subset [%d, %d]\n", rank, subset_start,
      subset_end
    );
    member_storev(my_group, my_member, {{subset_start, subset_end}});
    Fenix_Data_commit_barrier(my_group, NULL);

    if (rank == kKillID) {
      fprintf(stderr, "Doing kill on node %d\n", rank);
      raise(SIGTERM);
    }

    MPI_Barrier(res_comm);
  } catch (const fenix::CommException& e) {
    //We've had a failure! Time to recover data.
    fprintf(stderr, "Starting data recovery on rank %d\n", rank);
    fenix_require(e.fenix_err == FENIX_SUCCESS);

    //Repair the group from the spare
    Fenix_Data_group_create(
      my_group, res_comm, start_timestamp, group_depth, FENIX_DATA_POLICY_IMR,
      policy_vals, &errflag
    );
    member_repair(my_group, my_member);

    // Calculate expected subset for this rank
    int expected_start, expected_end;
    if (rank < 3) {
      expected_start = rank * 20;
      expected_end   = expected_start + 19;
    } else if (rank < 5) {
      expected_start = 60 + (rank - 3) * 15;
      expected_end   = expected_start + 14;
    } else {
      expected_start = 90;
      expected_end   = 99;
    }

    // Validate that the correct subset was restored
    DataSubset stored_subset = member_stored_subset(my_group, my_member);
    fenix_require(
      stored_subset.start() == expected_start &&
      stored_subset.end() == expected_end
    );

    //Set all data to a sentinel value for testing, then load
    data.resize(100);
    for (int& i : data) i = -999;
    member_load(my_group, my_member, data.data(), stored_subset);

    // Validate data values
    for (int i = 0; i < 100; i++) {
      if (i >= expected_start && i <= expected_end) {
        fenix_require(data[i] == rank * 1000 + i);
      } else {
        fenix_require(data[i] == -999);
      }
    }

    fprintf(stderr, "SUCCESS: Data restored correctly on rank %d\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
