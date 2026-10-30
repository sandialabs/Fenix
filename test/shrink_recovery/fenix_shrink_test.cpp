/*
//@HEADER
// ************************************************************************
//
//
//            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
//            _|        _|        _|_|    _|    _|      _|  _|
//            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
//            _|        _|        _|    _|_|    _|      _|  _|
//            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|
//
//
//
//
// Copyright (C) 2016 Rutgers University and Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <fenix.h>
#include <fenix.hpp>
#include <fenix_opt.hpp>
#include <fenix/mpixx/group.hpp>
#include <mpi.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <set>
#include <vector>

/*
 * Test shrinking recovery behavior when failures exceed available spares.
 *
 * This test verifies:
 * 1. Communicator shrinks when there are more failures than spares
 * 2. FENIX_WARNING_SPARE_RANKS_DEPLETED is returned on recovery
 * 3. Failed rank list is correctly reported
 * 4. Final communicator size matches expected (initial - failures + spares)
 */

void* exitThread(void* should_fail) {
  usleep(50000);
  if ((intptr_t)should_fail) {
    pid_t pid = getpid();
    kill(pid, SIGTERM);
  }
  return NULL;
}

int main(int argc, char** argv) {
  using fenix::mpixx::Group;

  if (argc < 3) {
    printf("Usage: %s <# spares> <fail_rank 1> <fail_rank 2> ...\n", *argv);
    exit(0);
  }

  int spare_ranks = atoi(argv[1]);

  std::set<int> failures;
  for (int i = 2; i < argc; i++) failures.insert(atoi(argv[i]));
  int num_failures = failures.size();

  int n_ranks; // Used for verifying Fenix_get_n_ranks_with_role

  MPI_Init(&argc, &argv);

  int old_world_size, new_world_size = -1;
  int old_rank, new_rank             = -1;

  MPI_Comm_size(MPI_COMM_WORLD, &old_world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &old_rank);

  // Expected final size: initial active ranks - unrecovered failures
  int initial_active_ranks = old_world_size - spare_ranks;
  int unrecovered_failures = std::max(num_failures - spare_ranks, 0);
  int expected_final_size  = initial_active_ranks - unrecovered_failures;

  MPI_Barrier(MPI_COMM_WORLD);
  pthread_t thread_id;
  intptr_t should_fail = failures.contains(old_rank);
  pthread_create(&thread_id, NULL, exitThread, (void*)should_fail);

  int fenix_status;
  MPI_Comm new_comm;
  int error;
  Fenix_Init(
    &fenix_status, MPI_COMM_WORLD, &new_comm, &argc, &argv, spare_ranks, &error
  );

  // Verify that spares depleted warning is returned for recovered ranks
  if (fenix_status == FENIX_ROLE_RECOVERED_RANK) {
    fenix_require(
      error == FENIX_WARNING_SPARE_RANKS_DEPLETED,
      "Expected FENIX_WARNING_SPARE_RANKS_DEPLETED, got %d", error
    );
  }

  MPI_Comm_size(new_comm, &new_world_size);
  MPI_Comm_rank(new_comm, &new_rank);

  if (fenix_status == FENIX_ROLE_INITIAL_RANK) {
    // Test INITIAL_RANK: should be all ranks before recovery
    n_ranks = -1;
    Fenix_get_n_ranks_with_role(FENIX_ROLE_INITIAL_RANK, &n_ranks);
    fenix_require(
      n_ranks == new_world_size,
      "INITIAL_RANK count should be %d before recovery, got %d", new_world_size,
      n_ranks
    );

    // Give time for exit thread to work before continuing
    usleep(300000);
  }

  MPI_Barrier(new_comm);

  // Verify final communicator size
  fenix_require(
    new_world_size == expected_final_size,
    "Communicator size mismatch: expected %d, got %d", expected_final_size,
    new_world_size
  );

  // Verify that we actually shrank if we had more failures than spares
  if (num_failures > spare_ranks) {
    fenix_require(
      new_world_size < initial_active_ranks,
      "Communicator should have shrunk: initial=%d, final=%d",
      initial_active_ranks, new_world_size
    );
  }

  // Get and verify failed rank list
  int *fails, num_fails;
  num_fails = Fenix_Process_fail_list(&fails);

  fenix_require(
    num_fails == num_failures,
    "Failed rank count mismatch: expected %d, got %d", num_failures, num_fails
  );

  // Verify each expected failure is in the list
  for (int i = 2; i < argc; i++) {
    int expected_fail = atoi(argv[i]);
    int found         = 0;
    for (int j = 0; j < num_fails; j++) {
      if (fails[j] == expected_fail) {
        found = 1;
        break;
      }
    }
    fenix_require(
      found, "Expected failed rank %d not found in fail list", expected_fail
    );
  }

  // Verify Fenix_get_n_ranks_with_role

  // Test FAILED_RANK: should equal number of failures
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_FAILED_RANK, &n_ranks);
  fenix_require(
    n_ranks == num_failures, "FAILED_RANK count mismatch: expected %d, got %d",
    num_failures, n_ranks
  );

  // Test SURVIVOR_RANK: user ranks that survived (didn't fail)
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_SURVIVOR_RANK, &n_ranks);
  int expected_survivors = initial_active_ranks - num_failures;
  fenix_require(
    n_ranks == expected_survivors,
    "SURVIVOR_RANK count mismatch: expected %d, got %d", expected_survivors,
    n_ranks
  );

  // Test RECOVERED_RANK: should be min(spare_ranks, num_failures)
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_RECOVERED_RANK, &n_ranks);
  int expected_recovered = std::min(num_failures, spare_ranks);
  fenix_require(
    n_ranks == expected_recovered,
    "RECOVERED_RANK count mismatch: expected %d, got %d", expected_recovered,
    n_ranks
  );

  // Test SPARE_RANK: remaining spares after recovery
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_SPARE_RANK, &n_ranks);
  int expected_spares = std::max(spare_ranks - num_failures, 0);
  fenix_require(
    n_ranks == expected_spares,
    "SPARE_RANK count mismatch: expected %d, got %d", expected_spares, n_ranks
  );

  // Test INITIAL_RANK: should be 0 after recovery
  n_ranks = -1;
  Fenix_get_n_ranks_with_role(FENIX_ROLE_INITIAL_RANK, &n_ranks);
  fenix_require(
    n_ranks == 0, "INITIAL_RANK count should be 0 after recovery, got %d",
    n_ranks
  );

  // Verify Fenix_get_rank_role for all ranks in new_comm
  for (int i = 0; i < new_world_size; i++) {
    int queried_role;
    Fenix_get_rank_role(new_comm, i, &queried_role);

    // Each rank should be either SURVIVOR or RECOVERED
    fenix_require(
      queried_role == FENIX_ROLE_SURVIVOR_RANK ||
        queried_role == FENIX_ROLE_RECOVERED_RANK,
      "Rank %d in new_comm should be SURVIVOR or RECOVERED, got %d", i,
      queried_role
    );
  }

  // Verify our own role matches what get_rank_role returns
  int our_queried_role;
  Fenix_get_rank_role(new_comm, new_rank, &our_queried_role);
  fenix_require(
    our_queried_role == fenix_status,
    "Rank %d: get_rank_role returned %d, but our role is %d", new_rank,
    our_queried_role, fenix_status
  );

  // ========================================================================
  // Test Fenix_repair_group functionality
  // ========================================================================

  Group g_new(new_comm);
  Group g_world(MPI_COMM_WORLD);
  Group g_orig        = g_world.range_incl({{0, initial_active_ranks - 1, 1}});
  Group g_orig_spares = g_world - g_orig;
  Group g_failures    = g_world.incl({failures.begin(), failures.end()});
  Group g_orig_failures  = g_orig | g_failures;
  Group g_spare_failures = g_orig_spares | g_failures;
  Group g_survived       = g_new | g_orig;
  Group g_recovered      = g_new | g_orig_spares;

  // Test 1: Repairing the current comm's group should be a no-op
  fenix_require(
    g_new == fenix::repair_group(g_new),
    "Rank %d: Repaired new group not equal to itself", new_rank
  );

  // Test 2: Create a group representing the original active ranks from
  // MPI_COMM_WORLD and repair it (should match current group)
  fenix_require(
    g_new == fenix::repair_group(g_orig),
    "Rank %d: Repaired original group not equal to new group", new_rank
  );

  // Test 3: Repair subgroup (even ranks from original)
  Group g_even = g_orig.range_incl({{0, g_orig.size() - 1, 2}});
  Group rg_even(fenix::repair_group(g_even));

  Group g_even_failures = g_even | g_failures;
  fenix_require(
    rg_even.size() >= g_even.size() - g_even_failures.size(),
    "Rank %d: Repaired even group size %d is smaller than minimum %d", new_rank,
    rg_even.size(), g_even.size() - g_even_failures.size()
  );
  fenix_require(
    rg_even.size() <= g_even.size(),
    "Rank %d: Repaired even group size %d is larger than original %d", new_rank,
    rg_even.size(), g_even.size()
  );

  // Test 4: Verify each member of repaired even group has correct slot/rank
  // mapping
  for (int i = 0; i < rg_even.size(); i++) {
    int rank_in_new = rg_even.translate_rank(i, g_new);
    fenix_require(
      rank_in_new != MPI_UNDEFINED,
      "Rank %d: rank %d of repaired even group (world rank %d) not in new comm",
      new_rank, i, rg_even.translate_rank(i, g_world)
    );

    int slot = fenix::rank_to_slot(new_comm, rank_in_new);
    fenix_require(
      slot % 2 == 0, "Rank %d: Repaired even group member has odd slot %d",
      new_rank, slot
    );
  }

  // Test 5: Repair empty group
  Group rg_empty(fenix::repair_group(MPI_GROUP_EMPTY));
  fenix_require(
    rg_empty.size() == 0,
    "Rank %d: Repaired empty group should have size 0, got %d", new_rank,
    rg_empty.size()
  );

  // Test 6: Group containing only failed ranks
  Group rg_orig_failures(fenix::repair_group(g_orig_failures));
  int n_recovered = g_orig_failures.size() - unrecovered_failures;
  fenix_require(
    rg_orig_failures.size() == n_recovered,
    "Rank %d: Repaired failed-only group size %d, expected %d (recovered %d "
    "of %d failures)",
    new_rank, rg_orig_failures.size(), n_recovered, n_recovered, num_failures
  );
  fenix_require(
    (rg_orig_failures | g_orig_failures).size() == 0,
    "Rank %d: Repaired failed-only group intersects with original failures",
    new_rank
  );
  fenix_require(
    (rg_orig_failures | g_orig).size() == 0,
    "Rank %d: Repaired failed-only group intersects with original group",
    new_rank
  );

  if (new_rank == 0) {
    printf("✓ All Fenix_repair_group() tests passed in shrink scenario\n");
    printf("  - New group repair is identity\n");
    printf("  - Original group repairs to new group\n");
    printf("  - Even-rank subgroup repairs to valid size and slots\n");
    printf("  - Empty group remains empty\n");
    printf(
      "  - Failed-only group repairs to correct size, no invalid replacements\n"
    );
  }

  printf(
    "Rank %d (was %d): shrink test PASSED - "
    "initial_active=%d, failures=%d, spares=%d, final=%d, missing=%d\n",
    new_rank, old_rank, initial_active_ranks, num_failures, spare_ranks,
    new_world_size, unrecovered_failures
  );

  Fenix_Finalize();
  pthread_join(thread_id, NULL);

  MPI_Finalize();

  return 0;
}
