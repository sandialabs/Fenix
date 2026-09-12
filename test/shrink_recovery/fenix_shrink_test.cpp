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
#include <fenix_opt.hpp>
#include <mpi.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

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

  printf(
    "Rank %d (was %d): shrink test PASSED - "
    "initial_active=%d, failures=%d, spares=%d, final=%d\n",
    new_rank, old_rank, initial_active_ranks,
    num_failures, spare_ranks, new_world_size
  );

  Fenix_Finalize();
  pthread_join(thread_id, NULL);

  MPI_Finalize();

  return 0;
}
