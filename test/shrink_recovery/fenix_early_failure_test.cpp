#include <fenix.h>
#include <fenix_opt.hpp>
#include <mpi.h>
#ifndef MPICH_VERSION
#include <mpi-ext.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Test early failure handling when a process fails BEFORE Fenix_Init.
 *
 * This test verifies that Fenix can handle failures that occur during the
 * initial MPI setup phase, before Fenix has established its fault tolerance
 * infrastructure. This tests the rebuild_proc_groups() logic during preinit.
 *
 * The test verifies:
 * 1. Fenix detects and handles failures during initial shrink in preinit
 * 2. Group accounting is correct even when initial procs don't match shrunk
 *    world
 * 3. Spare replacement works correctly for early failures
 * 4. Final communicator size matches expected value
 */

int main(int argc, char** argv) {

  if (argc < 3) {
    printf("Usage: %s <# spares> <fail_rank 1> <fail_rank 2> ...\n", *argv);
    exit(0);
  }

  int spare_ranks  = atoi(argv[1]);
  int num_failures = argc - 2;

  MPI_Init(&argc, &argv);

  int old_world_size, new_world_size = -1;
  int old_rank, new_rank             = -1;

  MPI_Comm_size(MPI_COMM_WORLD, &old_world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &old_rank);

  // Expected final size: initial active ranks - unrecovered failures
  int initial_active_ranks = old_world_size - spare_ranks;
  int unrecovered_failures = std::max(num_failures - spare_ranks, 0);
  int expected_final_size  = initial_active_ranks - unrecovered_failures;

  // Check if this rank should fail BEFORE Fenix_Init
  bool should_fail = false;
  for (int i = 2; i < argc; i++) {
    if (atoi(argv[i]) == old_rank) {
      should_fail = true;
      break;
    }
  }

  // Reliable barrier - we're not trying to test the case that a process
  // fails while another is still trying to MPI_Init
  int unused = 1;
  MPIX_Comm_agree(MPI_COMM_WORLD, &unused);

  // Kill this process immediately if it should fail early
  if (should_fail) {
    raise(SIGKILL);
  }

  int fenix_status;
  MPI_Comm new_comm;
  int error;

  // This is where Fenix should detect the early failures during shrink
  Fenix_Init(
    &fenix_status, MPI_COMM_WORLD, &new_comm, &argc, &argv, spare_ranks, &error
  );

  MPI_Comm_size(new_comm, &new_world_size);
  MPI_Comm_rank(new_comm, &new_rank);

  // Verify that Fenix handled the early failures correctly
  fenix_require(
    new_world_size == expected_final_size,
    "Early failure: Communicator size mismatch: expected %d, got %d",
    expected_final_size, new_world_size
  );

  // For early failures, all surviving ranks should be INITIAL_RANK
  // (they never recovered, they just survived the initial shrink)
  fenix_require(
    fenix_status == FENIX_ROLE_INITIAL_RANK,
    "Early failure: Expected INITIAL_RANK role, got %d", fenix_status
  );

  // Verify that SPARE_RANKS_DEPLETED warning is set when shrinking occurred
  if (num_failures > spare_ranks) {
    fenix_require(
      error == FENIX_WARNING_SPARE_RANKS_DEPLETED,
      "Early failure: Expected SPARE_RANKS_DEPLETED warning, got %d", error
    );
  } else {
    // If we had enough spares, no warning should be set
    fenix_require(
      error == FENIX_SUCCESS, "Early failure: Expected FENIX_SUCCESS, got %d",
      error
    );
  }

  // Note: fail_world is only populated during __fenix_repair_ranks() for
  // mid-execution recovery. Early initialization failures are not tracked
  // in the fail list since they occur before Fenix's fault tracking is set up.
  int *fails, num_fails;
  num_fails = Fenix_Process_fail_list(&fails);

  fenix_require(
    num_fails == 0,
    "Early failure: Expected empty fail list (early failures not tracked), got "
    "%d failures",
    num_fails
  );

  // Verify Fenix_get_n_ranks_with_role for early failure scenario
  int n_ranks;

  // Test FAILED_RANK: should equal number of failures
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_FAILED_RANK, &n_ranks);
  fenix_require(
    n_ranks == num_failures,
    "Early failure: MISSING_RANK count mismatch: expected %d, got %d",
    num_failures, n_ranks
  );

  // Test INITIAL_RANK: all ranks should still be INITIAL after early failure
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_INITIAL_RANK, &n_ranks);
  fenix_require(
    n_ranks == new_world_size,
    "Early failure: INITIAL_RANK count mismatch: expected %d, got %d",
    new_world_size, n_ranks
  );

  // Test SURVIVOR_RANK: should be 0 (no recovery happened, just init)
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_SURVIVOR_RANK, &n_ranks);
  fenix_require(
    n_ranks == 0, "Early failure: SURVIVOR_RANK should be 0, got %d", n_ranks
  );

  // Test RECOVERED_RANK: should be 0 (no recovery happened, just init)
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_RECOVERED_RANK, &n_ranks);
  fenix_require(
    n_ranks == 0, "Early failure: RECOVERED_RANK should be 0, got %d", n_ranks
  );

  // Test SPARE_RANK: remaining spares after early failures
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_SPARE_RANK, &n_ranks);
  int expected_spares = std::max(spare_ranks - num_failures, 0);
  fenix_require(
    n_ranks == expected_spares,
    "Early failure: SPARE_RANK count mismatch: expected %d, got %d",
    expected_spares, n_ranks
  );

  // Verify Fenix_get_rank_role for all ranks in new_comm
  for (int i = 0; i < new_world_size; i++) {
    int queried_role;
    Fenix_get_rank_role(new_comm, i, &queried_role);

    // After early failure, all ranks should still be INITIAL
    fenix_require(
      queried_role == FENIX_ROLE_INITIAL_RANK,
      "Early failure: Rank %d in new_comm should be INITIAL, got %d", i,
      queried_role
    );
  }

  // Verify our own role matches what get_rank_role returns
  int our_queried_role;
  Fenix_get_rank_role(new_comm, new_rank, &our_queried_role);
  fenix_require(
    our_queried_role == fenix_status,
    "Early failure: Rank %d: get_rank_role returned %d, but our role is %d",
    new_rank, our_queried_role, fenix_status
  );

  printf(
    "Rank %d (was %d): early failure test PASSED - "
    "initial_active=%d, early_failures=%d, spares=%d, final=%d, missing=%d\n",
    new_rank, old_rank, initial_active_ranks, num_failures, spare_ranks,
    new_world_size, unrecovered_failures
  );

  Fenix_Finalize();
  MPI_Finalize();

  return 0;
}
