#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <vector>

constexpr int gid = 0;
constexpr int mid = 42;

using fenix::DataSubset;
using namespace fenix::data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int rank;
  MPI_Comm_rank(res_comm, &rank);

  std::vector<int> data(8, 0);
  group_create(gid, {.depth = 3});
  member_create(gid, mid, data.data(), data.size(), MPI_INT);

  DataSubset first_subset({1, 2});
  DataSubset second_subset({4, 6});
  DataSubset expected_all(first_subset, second_subset);

  int timestamp0 = -1;
  int timestamp1 = -1;
  member_store(gid, mid, first_subset);
  commit(gid, &timestamp0);
  member_store(gid, mid, second_subset);
  commit(gid, &timestamp1);

  fenix_require(timestamp0 == 0, "Expected first timestamp to be 0");
  fenix_require(timestamp1 == 1, "Expected second timestamp to be 1");

  fenix_require(
    member_stored_subset(gid, mid, timestamp0) == first_subset,
    "C++ stored subset should match timestamp 0"
  );
  fenix_require(
    member_stored_subset(gid, mid, timestamp1) == second_subset,
    "C++ stored subset should match timestamp 1"
  );
  fenix_require(
    member_stored_subset(gid, mid, FENIX_DATA_SNAPSHOT_LATEST) == second_subset,
    "LATEST should return the most recent subset"
  );
  fenix_require(
    member_stored_subset(gid, mid, FENIX_DATA_SNAPSHOT_ALL) == expected_all,
    "ALL should return the union of committed subsets"
  );

  bool caught_missing = false;
  try {
    (void)member_stored_subset(gid, mid, FENIX_DATA_SNAPSHOT_NONE);
  } catch (const fenix::RuntimeException& e) {
    caught_missing = true;
    fenix_require(
      e.error == FENIX_ERROR_NODATA_FOUND,
      "Missing timestamp should raise FENIX_ERROR_NODATA_FOUND"
    );
  }
  fenix_require(caught_missing, "Expected missing timestamp exception");

  bool caught_invalid = false;
  try {
    (void)member_stored_subset(gid, mid, -FENIX_DATA_SNAPSHOT_NONE);
  } catch (const fenix::RuntimeException& e) {
    caught_invalid = true;
    fenix_require(
      e.error == FENIX_ERROR_INVALID_TIMESTAMP,
      "Invalid timestamp should raise FENIX_ERROR_INVALID_TIMESTAMP"
    );
  }
  fenix_require(caught_invalid, "Expected invalid timestamp exception");

  if (rank == 0) fprintf(stderr, "member stored subset tests passed\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
