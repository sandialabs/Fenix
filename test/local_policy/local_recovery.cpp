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

      if (Fenix_get_role() == FENIX_ROLE_INITIAL_RANK) {
        Fenix_Data_group_create(
          my_group, res_comm, start_timestamp, group_depth,
          FENIX_DATA_POLICY_LOCAL, NULL, &errflag
        );
        Fenix_Data_member_create(
          my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
        );

        data.resize(100 + rank);
        for (int i = 0; i < data.size(); i++) {
          data[i] = rank * 1000 + i + 1;
        }

        Fenix_Data_member_attr_set(
          my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
          &errflag
        );
        member_stage(my_group, my_member, {{0, data.size() - 1}});
        member_storev(my_group, my_member, SUBSET_PRESTAGED);
        Fenix_Data_commit_barrier(my_group, NULL);

        data.resize(50 + rank);
        for (int i = 0; i < data.size(); i++) {
          data[i] = rank * 2000 + i + 1;
        }

        Fenix_Data_member_attr_set(
          my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
          &errflag
        );
        member_stage(my_group, my_member, {{0, data.size() - 1}});
        member_storev(my_group, my_member, SUBSET_PRESTAGED);
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
          fprintf(stderr, "Starting data recovery on rank %d\n", rank);
          if (err->fenix_err != FENIX_SUCCESS) {
            fprintf(
              stderr, "FAILURE on Fenix Init (%d). Exiting.\n", err->fenix_err
            );
            exit(1);
          }

          Fenix_Data_group_create(
            my_group, res_comm, start_timestamp, group_depth,
            FENIX_DATA_POLICY_LOCAL, NULL, &errflag
          );

          // LOCAL policy has no resilience across ranks
          // Recovered rank: member doesn't exist (FENIX_ERROR_INVALID_MEMBERID)
          // Surviving ranks: member still exists with local snapshots (restore
          // succeeds)
          bool is_recovered = (Fenix_get_role() == FENIX_ROLE_RECOVERED_RANK);
          bool caught_expected_exception = false;
          try {
            DataSubset stored_subset;
            member_restore(
              my_group, my_member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST,
              stored_subset
            );
            // If we get here, no exception was thrown
            if (is_recovered) {
              // Recovered rank should have thrown an exception
              fprintf(
                stderr,
                "FAILURE on rank %d: Expected exception was not thrown\n", rank
              );
              MPI_Abort(MPI_COMM_WORLD, 1);
            } else {
              // Surviving ranks should succeed - they have local data
              fprintf(
                stderr, "Rank %d: Successfully restored local snapshot\n", rank
              );
              caught_expected_exception = true;
            }
          } catch (const fenix::RuntimeException& ex) {
            if (is_recovered) {
              // Recovered rank should throw INVALID_MEMBERID
              if (ex.error == FENIX_ERROR_INVALID_MEMBERID) {
                caught_expected_exception = true;
                fprintf(
                  stderr,
                  "Rank %d: Caught expected exception (error code %d)\n", rank,
                  ex.error
                );
              } else {
                fprintf(
                  stderr,
                  "FAILURE on rank %d: Caught unexpected exception with code "
                  "%d\n",
                  rank, ex.error
                );
                MPI_Abort(MPI_COMM_WORLD, 1);
              }
            } else {
              // Surviving ranks should not throw an exception
              fprintf(
                stderr,
                "FAILURE on rank %d: Unexpected exception with code %d\n", rank,
                ex.error
              );
              MPI_Abort(MPI_COMM_WORLD, 1);
            }
          }

          if (!caught_expected_exception) {
            fprintf(stderr, "FAILURE on rank %d: Test logic error\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
          }

          break;
        } catch (const fenix::CommException& nested) {
          err = &nested;
        }
      }
    }
  }

  // Test passes if we successfully validated the exception
  printf(
    "Rank %d: Test passed - LOCAL policy correctly has no resilience\n", rank
  );

  MPI_Finalize();
  return 0;
}
