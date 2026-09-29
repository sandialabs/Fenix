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
  int unrecovered_failures =
    (num_failures > spare_ranks) ? (num_failures - spare_ranks) : 0;
  int expected_final_size = initial_active_ranks - unrecovered_failures;

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

  printf(
    "Rank %d (was %d): early failure test PASSED - "
    "initial_active=%d, early_failures=%d, spares=%d, final=%d\n",
    new_rank, old_rank, initial_active_ranks, num_failures, spare_ranks,
    new_world_size
  );

  Fenix_Finalize();
  MPI_Finalize();

  return 0;
}
