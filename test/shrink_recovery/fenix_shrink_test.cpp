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

void* exitThread(void* should_exit) {
  usleep(10000);
  if (((intptr_t)should_exit) == 1) {
    pid_t pid = getpid();
    kill(pid, SIGTERM);
  }
  return NULL;
}

int main(int argc, char** argv) {

  if (argc < 3) {
    printf("Usage: %s <# spares> <fail_rank 1> <fail_rank 2> ...\n", *argv);
    exit(0);
  }

  int spare_ranks = atoi(argv[1]);
  int num_failures = argc - 2;

  MPI_Init(&argc, &argv);

  int old_world_size, new_world_size = -1;
  int old_rank, new_rank = -1;

  MPI_Comm world_comm;
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  MPI_Comm_size(world_comm, &old_world_size);
  MPI_Comm_rank(world_comm, &old_rank);

  // Expected final size: initial active ranks - unrecovered failures
  int initial_active_ranks = old_world_size - spare_ranks;
  int unrecovered_failures = (num_failures > spare_ranks) ?
                             (num_failures - spare_ranks) : 0;
  int expected_final_size = initial_active_ranks - unrecovered_failures;

  // Check if this rank should fail
  intptr_t should_cancel = 0;
  for (int i = 2; i < argc; i++) {
    if (atoi(argv[i]) == old_rank) {
      should_cancel = 1;
      break;
    }
  }

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, exitThread, (void*)should_cancel);

  int fenix_status;
  MPI_Comm new_comm;
  int error;
  Fenix_Init(
    &fenix_status, world_comm, &new_comm, &argc, &argv, spare_ranks, &error
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

  // Give time for exit thread to work
  if (fenix_status == FENIX_ROLE_INITIAL_RANK) {
    usleep(100000);
  }

  MPI_Barrier(new_comm);

  // Verify final communicator size
  fenix_require(
    new_world_size == expected_final_size,
    "Communicator size mismatch: expected %d, got %d",
    expected_final_size, new_world_size
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
    "Failed rank count mismatch: expected %d, got %d",
    num_failures, num_fails
  );

  // Verify each expected failure is in the list
  for (int i = 2; i < argc; i++) {
    int expected_fail = atoi(argv[i]);
    int found = 0;
    for (int j = 0; j < num_fails; j++) {
      if (fails[j] == expected_fail) {
        found = 1;
        break;
      }
    }
    fenix_require(
      found,
      "Expected failed rank %d not found in fail list",
      expected_fail
    );
  }

  // Verify Fenix_get_number_of_ranks_with_role
  int n_ranks;

  // Test MISSING_RANK: should equal number of unrecovered failures (shrinkage)
  Fenix_get_number_of_ranks_with_role(FENIX_ROLE_MISSING_RANK, &n_ranks);
  fenix_require(
    n_ranks == unrecovered_failures,
    "MISSING_RANK count mismatch: expected %d, got %d",
    unrecovered_failures, n_ranks
  );

  // Test SURVIVOR_RANK: user ranks that survived (didn't fail)
  Fenix_get_number_of_ranks_with_role(FENIX_ROLE_SURVIVOR_RANK, &n_ranks);
  int expected_survivors = initial_active_ranks - num_failures;
  fenix_require(
    n_ranks == expected_survivors,
    "SURVIVOR_RANK count mismatch: expected %d, got %d",
    expected_survivors, n_ranks
  );

  // Test RECOVERED_RANK: should be min(spare_ranks, num_failures)
  Fenix_get_number_of_ranks_with_role(FENIX_ROLE_RECOVERED_RANK, &n_ranks);
  int expected_recovered = (num_failures < spare_ranks) ? num_failures : spare_ranks;
  fenix_require(
    n_ranks == expected_recovered,
    "RECOVERED_RANK count mismatch: expected %d, got %d",
    expected_recovered, n_ranks
  );

  // Test SPARE_RANK: remaining spares after recovery
  Fenix_get_number_of_ranks_with_role(FENIX_ROLE_SPARE_RANK, &n_ranks);
  int expected_spares = (spare_ranks > num_failures) ?
                        (spare_ranks - num_failures) : 0;
  fenix_require(
    n_ranks == expected_spares,
    "SPARE_RANK count mismatch: expected %d, got %d",
    expected_spares, n_ranks
  );

  // Test INITIAL_RANK: should be 0 after recovery
  Fenix_get_number_of_ranks_with_role(FENIX_ROLE_INITIAL_RANK, &n_ranks);
  fenix_require(
    n_ranks == 0,
    "INITIAL_RANK count should be 0 after recovery, got %d",
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
      "Rank %d in new_comm should be SURVIVOR or RECOVERED, got %d",
      i, queried_role
    );
  }

  // Verify our own role matches what get_rank_role returns
  int our_queried_role;
  Fenix_get_rank_role(new_comm, new_rank, &our_queried_role);
  fenix_require(
    our_queried_role == fenix_status,
    "Rank %d: get_rank_role returned %d, but our role is %d",
    new_rank, our_queried_role, fenix_status
  );

  // ========================================================================
  // Test Fenix_repair_group functionality
  // ========================================================================

  MPI_Group world_group, current_group;
  MPI_Comm_group(MPI_COMM_WORLD, &world_group);
  MPI_Comm_group(new_comm, &current_group);

  // Test 1: Repairing the current comm's group should be a no-op
  MPI_Group repaired_current;
  Fenix_repair_group(current_group, &repaired_current);

  int current_size, repaired_current_size;
  MPI_Group_size(current_group, &current_size);
  MPI_Group_size(repaired_current, &repaired_current_size);
  fenix_require(
    current_size == repaired_current_size,
    "Rank %d: Current group size %d != repaired size %d",
    new_rank, current_size, repaired_current_size
  );

  int compare_result;
  MPI_Group_compare(current_group, repaired_current, &compare_result);
  fenix_require(
    compare_result == MPI_IDENT,
    "Rank %d: Repairing current group should be identity, got compare=%d",
    new_rank, compare_result
  );

  // Test 2: Create a group representing the original active ranks from MPI_COMM_WORLD
  // and repair it (should match current group)
  std::vector<int> original_ranks(initial_active_ranks);
  for (int i = 0; i < initial_active_ranks; i++) {
    original_ranks[i] = i;
  }

  MPI_Group original_group, repaired_original;
  MPI_Group_incl(world_group, initial_active_ranks, original_ranks.data(), &original_group);
  Fenix_repair_group(original_group, &repaired_original);

  int repaired_original_size;
  MPI_Group_size(repaired_original, &repaired_original_size);

  // Expected size: original size minus unrecovered failures
  int expected_size = initial_active_ranks - unrecovered_failures;
  fenix_require(
    repaired_original_size == expected_size,
    "Rank %d: Repaired original group size %d, expected %d (initial %d - missing %d)",
    new_rank, repaired_original_size, expected_size, initial_active_ranks, unrecovered_failures
  );

  // Should match current group
  MPI_Group_compare(repaired_original, current_group, &compare_result);
  fenix_require(
    compare_result == MPI_IDENT,
    "Rank %d: Repaired original group should match current group, got compare=%d",
    new_rank, compare_result
  );

  // Test 3: Repair subgroup (even ranks from original)
  int n_even = (initial_active_ranks + 1) / 2;
  std::vector<int> even_ranks(n_even);
  for (int i = 0; i < n_even; i++) {
    even_ranks[i] = i * 2;
  }

  MPI_Group even_group, repaired_even;
  MPI_Group_incl(world_group, n_even, even_ranks.data(), &even_group);
  Fenix_repair_group(even_group, &repaired_even);

  int repaired_even_size;
  MPI_Group_size(repaired_even, &repaired_even_size);

  std::vector<int> fail_list_vec = fenix::fail_list();

  // Repaired size should be between 0 and n_even
  fenix_require(
    repaired_even_size >= 0 && repaired_even_size <= n_even,
    "Rank %d: Repaired even group size %d out of range [0,%d]",
    new_rank, repaired_even_size, n_even
  );

  // Test 4: Verify each member of repaired even group has correct slot/rank mapping
  for (int i = 0; i < repaired_even_size; i++) {
    int pid_in_world, rank_in_new_comm;
    MPI_Group_translate_ranks(repaired_even, 1, &i, world_group, &pid_in_world);
    MPI_Group_translate_ranks(world_group, 1, &pid_in_world, current_group, &rank_in_new_comm);

    fenix_require(
      rank_in_new_comm != MPI_UNDEFINED,
      "Rank %d: Member %d of repaired even group (world pid %d) not in current comm",
      new_rank, i, pid_in_world
    );

    // Verify this process is either survivor or recovered
    int member_role;
    Fenix_get_rank_role(new_comm, rank_in_new_comm, &member_role);
    fenix_require(
      member_role == FENIX_ROLE_SURVIVOR_RANK || member_role == FENIX_ROLE_RECOVERED_RANK,
      "Rank %d: Repaired group member has invalid role %d",
      new_rank, member_role
    );

    // Verify the slot for this process is one of the even slots
    int member_slot;
    Fenix_get_rank_role(new_comm, rank_in_new_comm, &member_role);
    Fenix_rank_to_slot(new_comm, rank_in_new_comm, &member_slot);

    bool is_even_slot = (member_slot % 2 == 0);
    fenix_require(
      is_even_slot,
      "Rank %d: Repaired even group member has odd slot %d",
      new_rank, member_slot
    );
  }

  // Test 5: Repair empty group
  MPI_Group repaired_empty;
  Fenix_repair_group(MPI_GROUP_EMPTY, &repaired_empty);

  int repaired_empty_size;
  MPI_Group_size(repaired_empty, &repaired_empty_size);
  fenix_require(
    repaired_empty_size == 0,
    "Rank %d: Repaired empty group should have size 0, got %d",
    new_rank, repaired_empty_size
  );

  // Test 6: Group containing only failed ranks
  if (!fail_list_vec.empty()) {
    MPI_Group failed_only_group, repaired_failed;
    MPI_Group_incl(world_group, fail_list_vec.size(), fail_list_vec.data(), &failed_only_group);
    Fenix_repair_group(failed_only_group, &repaired_failed);

    int repaired_failed_size;
    MPI_Group_size(repaired_failed, &repaired_failed_size);

    // How many failed ranks were recovered vs missing?
    int expected_recovered = num_failures - unrecovered_failures;

    fenix_require(
      repaired_failed_size == expected_recovered,
      "Rank %d: Repaired failed-only group size %d, expected %d (recovered %d of %d failures)",
      new_rank, repaired_failed_size, expected_recovered, expected_recovered, num_failures
    );

    MPI_Group_free(&failed_only_group);
    MPI_Group_free(&repaired_failed);
  }

  // Test 7: Mixed group (some survivors, some failed-but-recovered, some missing)
  // Create a group with first 3 ranks (if they exist in original)
  if (initial_active_ranks >= 3) {
    std::vector<int> first_three = {0, 1, 2};
    MPI_Group first_three_group, repaired_first_three;
    MPI_Group_incl(world_group, 3, first_three.data(), &first_three_group);
    Fenix_repair_group(first_three_group, &repaired_first_three);

    int repaired_first_three_size;
    MPI_Group_size(repaired_first_three, &repaired_first_three_size);

    // The repaired group should contain at least 1 and at most 3 members
    fenix_require(
      repaired_first_three_size >= 1 && repaired_first_three_size <= 3,
      "Rank %d: Repaired first-3 group size %d out of range [1,3]",
      new_rank, repaired_first_three_size
    );

    // Verify each member exists in current comm with correct role
    int survivors_or_recovered = 0;
    for (int i = 0; i < repaired_first_three_size; i++) {
      int pid, rank_in_current;
      MPI_Group_translate_ranks(repaired_first_three, 1, &i, world_group, &pid);
      MPI_Group_translate_ranks(world_group, 1, &pid, current_group, &rank_in_current);

      fenix_require(
        rank_in_current != MPI_UNDEFINED,
        "Rank %d: Repaired first-3 member %d (pid %d) not in current group",
        new_rank, i, pid
      );

      // Verify this member was one of the original first_three
      bool was_in_original = false;
      for (int orig_rank : first_three) {
        int orig_slot, curr_slot;
        // Get slot for this original rank
        // Actually, simpler: check if this pid corresponds to one of slots 0, 1, or 2
        int pid_slot;
        Fenix_rank_to_slot(new_comm, rank_in_current, &pid_slot);
        if (pid_slot >= 0 && pid_slot < 3) {
          was_in_original = true;
          break;
        }
      }

      fenix_require(
        was_in_original,
        "Rank %d: Repaired member (pid %d, slot %d) not from original first-3",
        new_rank, pid, -1
      );

      survivors_or_recovered++;
    }

    fenix_require(
      survivors_or_recovered == repaired_first_three_size,
      "Rank %d: Count mismatch in repaired first-3",
      new_rank
    );

    MPI_Group_free(&first_three_group);
    MPI_Group_free(&repaired_first_three);
  }

  // Cleanup
  MPI_Group_free(&world_group);
  MPI_Group_free(&current_group);
  MPI_Group_free(&repaired_current);
  MPI_Group_free(&original_group);
  MPI_Group_free(&repaired_original);
  MPI_Group_free(&even_group);
  MPI_Group_free(&repaired_even);

  if (new_rank == 0) {
    printf("✓ All Fenix_repair_group() tests passed in shrink scenario\n");
    printf("  - Current group repair is identity\n");
    printf("  - Original %d ranks → %d survivors after repair\n",
           initial_active_ranks, expected_size);
    printf("  - Even-rank subgroup: %d ranks → %d after repair\n", n_even, repaired_even_size);
    printf("  - Empty group remains empty\n");
    printf("  - Failed-only group: %d failed → %d recovered\n",
           (int)fail_list_vec.size(), num_failures - unrecovered_failures);
  }

  printf(
    "Rank %d (was %d): shrink test PASSED - "
    "initial_active=%d, failures=%d, spares=%d, final=%d, missing=%d\n",
    new_rank, old_rank, initial_active_ranks,
    num_failures, spare_ranks, new_world_size, unrecovered_failures
  );

  Fenix_Finalize();
  pthread_join(thread_id, NULL);

  MPI_Finalize();

  return 0;
}
