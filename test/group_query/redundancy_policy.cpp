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
//        Rob Van der Wijngaart, Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

constexpr int my_group = 0;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  if (rank == 0) fprintf(stderr, "Test: Query redundancy policy\n");

  // Create a group with FENIX_DATA_POLICY_IN_MEMORY_RAID policy
  // Set explicit policy values: mode=1, rank_separation=1
  int policy_vals_in[2] = {1, 1};
  int errflag;
  int ret = Fenix_Data_group_create(
    my_group, res_comm, 0, 1,
    FENIX_DATA_POLICY_IN_MEMORY_RAID, policy_vals_in, &errflag
  );
  fenix_require(ret == FENIX_SUCCESS);
  if (rank == 0) fprintf(stderr, "Group created with IN_MEMORY_RAID policy (mode=1, rank_separation=1)\n");

  // Query the redundancy policy
  int policy_name;
  int policy_vals_out[3] = {-1, -1, -1};  // Initialize to detect if values are set
  int flag;
  ret = Fenix_Data_group_get_redundancy_policy(
    my_group, &policy_name, policy_vals_out, &flag
  );

  fenix_require(ret == FENIX_SUCCESS);
  fenix_require(flag == FENIX_SUCCESS);
  fenix_require(policy_name == FENIX_DATA_POLICY_IN_MEMORY_RAID);

  // Verify returned policy values match what we set
  fenix_require(policy_vals_out[0] == 1);  // mode
  fenix_require(policy_vals_out[1] == 1);  // rank_separation

  if (rank == 0) {
    fprintf(stderr, "Policy query successful\n");
    fprintf(stderr, "  Policy name: %d (expected: %d)\n",
            policy_name, FENIX_DATA_POLICY_IN_MEMORY_RAID);
    fprintf(stderr, "  Mode: %d (expected: 1)\n", policy_vals_out[0]);
    fprintf(stderr, "  Rank separation: %d (expected: 1)\n", policy_vals_out[1]);
    fprintf(stderr, "  Flag value: %d (expected: %d)\n", flag, FENIX_SUCCESS);
  }

  // Test 2: Mode 5 (set-based) IMR
  if (rank == 0) fprintf(stderr, "\nTest 2: Mode 5 (set-based) IMR\n");

  constexpr int my_group_mode5 = 1;
  int policy_vals_mode5[3] = {5, 1, 3};
  ret = Fenix_Data_group_create(
    my_group_mode5, res_comm, 0, 1,
    FENIX_DATA_POLICY_IN_MEMORY_RAID, policy_vals_mode5, &errflag
  );
  fenix_require(ret == FENIX_SUCCESS);
  if (rank == 0) fprintf(stderr, "Group 1 created with IN_MEMORY_RAID policy (mode=5, rank_separation=1, set_size=3)\n");

  // Query the redundancy policy for mode 5
  int policy_name_mode5;
  int policy_vals_out_mode5[3] = {-1, -1, -1};
  int flag_mode5;
  ret = Fenix_Data_group_get_redundancy_policy(
    my_group_mode5, &policy_name_mode5, policy_vals_out_mode5, &flag_mode5
  );

  fenix_require(ret == FENIX_SUCCESS);
  fenix_require(flag_mode5 == FENIX_SUCCESS);
  fenix_require(policy_name_mode5 == FENIX_DATA_POLICY_IN_MEMORY_RAID);

  // Verify returned policy values match what we set
  fenix_require(policy_vals_out_mode5[0] == 5);  // mode
  fenix_require(policy_vals_out_mode5[1] == 1);  // rank_separation
  // Note: set_size gets overwritten by MPI_Comm_size(set_comm, &set_size) in Group constructor
  // so we cannot verify it matches the input value. We just verify it was set to something > 0.
  fenix_require(policy_vals_out_mode5[2] > 0);  // set_size (actual communicator size)

  if (rank == 0) {
    fprintf(stderr, "Policy query successful for mode 5\n");
    fprintf(stderr, "  Policy name: %d (expected: %d)\n",
            policy_name_mode5, FENIX_DATA_POLICY_IN_MEMORY_RAID);
    fprintf(stderr, "  Mode: %d (expected: 5)\n", policy_vals_out_mode5[0]);
    fprintf(stderr, "  Rank separation: %d (expected: 1)\n", policy_vals_out_mode5[1]);
    fprintf(stderr, "  Set size: %d (actual set_comm size)\n", policy_vals_out_mode5[2]);
    fprintf(stderr, "  Flag value: %d (expected: %d)\n", flag_mode5, FENIX_SUCCESS);
  }

  Fenix_Finalize();
  MPI_Finalize();

  if (rank == 0) fprintf(stderr, "Test PASSED\n");

  return 0;
}
