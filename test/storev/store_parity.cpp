#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Use fixed size data (same for all ranks) for parity mode
  constexpr int kDataSize = 100;
  std::vector<int> data(kDataSize);

  bool should_throw = Fenix_get_role() == FENIX_ROLE_RECOVERED_RANK;
  while (true) {
    try {
      if (should_throw) {
        should_throw = false;
        fenix::throw_exception();
      }

      //Initial work and commits
      if (Fenix_get_role() == FENIX_ROLE_INITIAL_RANK) {
        // Use IMR mode 5 (parity) with set size of 3, rank separation of 1
        // With 6 ranks and set_size=3, we get 2 sets of 3 ranks each
        int policy_vals[] = {5, 1, 3};
        Fenix_Data_group_create(
          my_group, res_comm, start_timestamp, group_depth,
          FENIX_DATA_POLICY_IMR, policy_vals, &errflag
        );
        if (errflag != FENIX_SUCCESS) {
          fprintf(
            stderr, "Rank %d: group create failed with %d\n", rank, errflag
          );
          exit(1);
        }

        Fenix_Data_member_create(
          my_group, my_member, data.data(), kDataSize, MPI_INT
        );

        // Initialize data
        for (int i = 0; i < kDataSize; i++) {
          data[i] = rank * 1000 + i;
        }

        // Store and commit
        member_store(my_group, my_member);
        Fenix_Data_commit_barrier(my_group, NULL);

        fprintf(stderr, "Rank %d: Store and commit successful\n", rank);

        if (rank == kKillID) {
          fprintf(stderr, "Doing kill on rank %d\n", rank);
          raise(SIGTERM);
        }
      }

      Fenix_Finalize();
      break;
    } catch (const fenix::CommException& e) {
      const fenix::CommException* err = &e;
      while (true) {
        try {
          //We've had a failure! Time to recover data.
          fprintf(stderr, "Starting data recovery on rank %d\n", rank);
          if (err->fenix_err != FENIX_SUCCESS) {
            fprintf(
              stderr, "FAILURE on Fenix Init (%d). Exiting.\n", err->fenix_err
            );
            exit(1);
          }

          //Repair the group from the spare
          Fenix_Data_group_create(
            my_group, res_comm, start_timestamp, group_depth,
            FENIX_DATA_POLICY_IMR, (int[]){5, 1, 3}, &errflag
          );

          //Restore data
          DataSubset stored_subset;
          int ret = member_restore(
            my_group, my_member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST,
            stored_subset
          );
          if (ret != FENIX_SUCCESS) {
            fprintf(stderr, "Rank %d restore failure w/ code %d\n", rank, ret);
            MPI_Abort(MPI_COMM_WORLD, 1);
          }

          //Load the data
          ret = member_lrestore(
            my_group, my_member, data.data(), kDataSize,
            FENIX_DATA_SNAPSHOT_LATEST, stored_subset
          );
          if (ret != FENIX_SUCCESS) {
            fprintf(stderr, "Rank %d lrestore failure w/ code %d\n", rank, ret);
            MPI_Abort(MPI_COMM_WORLD, 1);
          }

          //Validate restored data
          bool successful   = true;
          int expected_base = rank * 1000;
          for (int i = 0; i < kDataSize && successful; i++) {
            if (data[i] != expected_base + i) {
              fprintf(
                stderr, "FAILURE rank %d: data[%d] = %d, expected %d\n", rank,
                i, data[i], expected_base + i
              );
              successful = false;
            }
          }
          fenix_require(successful);

          fprintf(
            stderr, "SUCCESS: Data restored correctly on rank %d\n", rank
          );

          break;
        } catch (const fenix::CommException& e) {
          fatal_print("Nested CommException during recovery");
        }
      }
    }
  }

  MPI_Finalize();
  return 0;
}
