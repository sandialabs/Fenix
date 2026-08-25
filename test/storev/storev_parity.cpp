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

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  std::vector<int> data;

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
        Fenix_Data_member_create(
          my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
        );

        // Each rank stores a different subset
        data.resize(100);

        // Initialize with rank-specific data
        for (int i = 0; i < 100; i++) {
          data[i] = rank * 1000 + i;
        }

        Fenix_Data_member_attr_set(
          my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
          &errflag
        );

        // Each rank stores a different-sized subset
        // Rank 0: elements [0, 19]   (20 elements)
        // Rank 1: elements [20, 39]  (20 elements)
        // Rank 2: elements [40, 59]  (20 elements)
        // Rank 3: elements [60, 74]  (15 elements)
        // Rank 4: elements [75, 89]  (15 elements)
        // Rank 5: elements [90, 99]  (10 elements)
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
      }

      Fenix_Finalize();
      break;
    } catch (const fenix::CommException& e) {
      const fenix::CommException* err = &e;
      while (true) {
        try {
          //We've had a failure! Time to recover data.
          fprintf(stderr, "Starting data recovery on rank %d\n", rank);
          fenix_require(err->fenix_err == FENIX_SUCCESS);

          //Repair the group from the spare
          Fenix_Data_group_create(
            my_group, res_comm, start_timestamp, group_depth,
            FENIX_DATA_POLICY_IMR, (int[]){5, 1, 3}, &errflag
          );

          //Do a null restore to get information about the stored subset
          DataSubset stored_subset;
          int ret = member_restore(
            my_group, my_member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST,
            stored_subset
          );
          if (ret != FENIX_SUCCESS) {
            fprintf(stderr, "Rank %d restore failure w/ code %d\n", rank, ret);
            MPI_Abort(MPI_COMM_WORLD, 1);
          }

          fprintf(
            stderr, "Rank %d restored subset [%zu, %zu]\n", rank,
            stored_subset.start(), stored_subset.end()
          );

          //Resize data buffer to fit the restored subset
          data.resize(100);

          //Set all data to a sentinel value for testing
          for (int& i : data) i = -999;

          //Now do an lrestore to get the recovered data.
          // Note: This will throw FENIX_WARNING_PARTIAL_RESTORE because we're
          // requesting more data than was stored, but that's expected - we
          // still get the subset that was stored
          try {
            ret = member_lrestore(
              my_group, my_member, data.data(), data.size(),
              FENIX_DATA_SNAPSHOT_LATEST, stored_subset
            );
            if (ret != FENIX_SUCCESS && ret != FENIX_WARNING_PARTIAL_RESTORE) {
              fprintf(
                stderr, "Rank %d lrestore failure w/ code %d\n", rank, ret
              );
              MPI_Abort(MPI_COMM_WORLD, 1);
            }
          } catch (const fenix::RuntimeException& e) {
            // Expected: partial restore warning because we requested more than
            // was stored
            if (e != FENIX_WARNING_PARTIAL_RESTORE) {
              fprintf(
                stderr, "Rank %d unexpected exception: %s\n", rank, e.what()
              );
              MPI_Abort(MPI_COMM_WORLD, 1);
            }
          }

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
          fenix_require(
            stored_subset.start() == expected_start &&
            stored_subset.end() == expected_end
          );

          // Validate restored data values in the subset
          for (int i = expected_start; i <= expected_end; i++) {
            int expected_value = rank * 1000 + i;
            fenix_require(data[i] == expected_value);
          }

          // Verify data outside the subset wasn't touched (should still be
          // -999)
          for (int i = 0; i < expected_start; i++) {
            fenix_require(data[i] == -999);
          }
          for (int i = expected_end + 1; i < 100; i++) {
            fenix_require(data[i] == -999);
          }

          fprintf(
            stderr, "SUCCESS: Data restored correctly on rank %d\n", rank
          );
          break;
        } catch (const fenix::CommException& e) {
          fprintf(
            stderr, "Nested CommException on rank %d during recovery\n", rank
          );
          err          = &e;
          should_throw = true;
        }
      }
    }
  }

  MPI_Finalize();
  return 0;
}
