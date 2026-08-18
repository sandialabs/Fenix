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
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY and SANDIA CORPORATION
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RUTGERS
// UNIVERISY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

constexpr int my_group  = 0;
constexpr int member_10 = 10;
constexpr int member_20 = 20;
constexpr int member_30 = 30;

using fenix::DataSubset;
using namespace fenix::data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Use only 2 ranks
  if (num_ranks != 2) {
    if (rank == 0)
      fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  if (rank == 0) fprintf(stderr, "Test: checkpointv() with SUBSET_PRESTAGED\n");

  // Create group with depth=2
  group_create(my_group, {.depth = 2});

  // Create 3 members with different data
  std::vector<int> data10(50);
  std::vector<int> data20(75);
  std::vector<int> data30(100);

  // Initialize with distinct patterns
  // Member 10: rank*1000+i
  for (int i = 0; i < data10.size(); i++) {
    data10[i] = rank * 1000 + i;
  }

  // Member 20: rank*2000+i
  for (int i = 0; i < data20.size(); i++) {
    data20[i] = rank * 2000 + i;
  }

  // Member 30: rank*3000+i
  for (int i = 0; i < data30.size(); i++) {
    data30[i] = rank * 3000 + i;
  }

  if (rank == 0) fprintf(stderr, "  Creating 3 members (IDs: 10, 20, 30)\n");

  member_create(my_group, member_10, data10.data(), data10.size(), MPI_INT);
  member_create(my_group, member_20, data20.data(), data20.size(), MPI_INT);
  member_create(my_group, member_30, data30.data(), data30.size(), MPI_INT);

  // Different ranks stage different subsets
  if (rank == 0) {
    fprintf(stderr, "  Rank 0: Staging members 10 and 20\n");
    member_stage(my_group, member_10, SUBSET_FULL);
    member_stage(my_group, member_20, SUBSET_FULL);
  } else {
    fprintf(stderr, "  Rank 1: Staging members 20 and 30\n");
    member_stage(my_group, member_20, SUBSET_FULL);
    member_stage(my_group, member_30, SUBSET_FULL);
  }

  // Both ranks call checkpointv with SUBSET_PRESTAGED
  if (rank == 0) fprintf(stderr, "  Calling checkpointv(SUBSET_PRESTAGED)\n");

  int timestamp = -1;
  int ret       = checkpointv(my_group, SUBSET_PRESTAGED, &timestamp);

  // Strict verification: checkpointv must return FENIX_SUCCESS
  if (ret != FENIX_SUCCESS) {
    fprintf(
      stderr,
      "Rank %d: ERROR - checkpointv failed with code %d (expected "
      "FENIX_SUCCESS=0)\n",
      rank, ret
    );
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0)
    fprintf(stderr, "  Checkpoint succeeded at timestamp %d\n", timestamp);

  // Clear all member data
  if (rank == 0) fprintf(stderr, "  Clearing all data\n");
  for (int& val : data10) val = -1;
  for (int& val : data20) val = -1;
  for (int& val : data30) val = -1;

  // Restore all members and track return codes
  if (rank == 0) fprintf(stderr, "  Restoring members\n");

  DataSubset restored_subset;
  int ret_member10, ret_member20, ret_member30;

  // Restore member 10
  try {
    ret_member10 = member_restore(
      my_group, member_10, data10.data(), data10.size(), timestamp,
      restored_subset
    );
  } catch (const fenix::RuntimeException& e) {
    ret_member10 = e.error; 
  }

  // Restore member 20
  try {
    ret_member20 = member_restore(
      my_group, member_20, data20.data(), data20.size(), timestamp,
      restored_subset
    );
  } catch (const fenix::RuntimeException& e) {
    ret_member20 = e.error; 
  }

  // Restore member 30
  try {
    ret_member30 = member_restore(
      my_group, member_30, data30.data(), data30.size(), timestamp,
      restored_subset
    );
  } catch (const fenix::RuntimeException& e) {
    ret_member30 = e.error; 
  }

  // Verify only valid return codes
  const int FENIX_WARNING_PARTIAL_RESTORE = 101;
  bool valid_codes                        = true;

  auto is_valid_code = [](int code) {
    return code == FENIX_SUCCESS || code == FENIX_WARNING_PARTIAL_RESTORE;
  };

  if (!is_valid_code(ret_member10)) {
    fprintf(
      stderr, "Rank %d: ERROR - member 10 restore returned invalid code %d\n",
      rank, ret_member10
    );
    valid_codes = false;
  }
  if (!is_valid_code(ret_member20)) {
    fprintf(
      stderr, "Rank %d: ERROR - member 20 restore returned invalid code %d\n",
      rank, ret_member20
    );
    valid_codes = false;
  }
  if (!is_valid_code(ret_member30)) {
    fprintf(
      stderr, "Rank %d: ERROR - member 30 restore returned invalid code %d\n",
      rank, ret_member30
    );
    valid_codes = false;
  }

  if (!valid_codes) {
    MPI_Abort(res_comm, 1);
  }

  // Verify member 20 data (must succeed on both ranks)
  if (ret_member20 == FENIX_SUCCESS) {
    bool data20_ok = true;
    for (int i = 0; i < data20.size(); i++) {
      int expected = rank * 2000 + i;
      if (data20[i] != expected) {
        fprintf(
          stderr,
          "Rank %d: Member 20 data mismatch at index %d! Expected %d, got %d\n",
          rank, i, expected, data20[i]
        );
        data20_ok = false;
        break;
      }
    }
    if (!data20_ok) {
      fprintf(
        stderr, "Rank %d: FAILURE - member 20 data verification failed\n", rank
      );
      MPI_Abort(res_comm, 1);
    }
  }

  // Verify member 10 data if it succeeded
  if (ret_member10 == FENIX_SUCCESS) {
    bool data10_ok = true;
    for (int i = 0; i < data10.size(); i++) {
      int expected = rank * 1000 + i;
      if (data10[i] != expected) {
        data10_ok = false;
        break;
      }
    }
    if (!data10_ok) {
      fprintf(
        stderr, "Rank %d: FAILURE - member 10 data verification failed\n", rank
      );
      MPI_Abort(res_comm, 1);
    }
  }

  // Verify member 30 data if it succeeded
  if (ret_member30 == FENIX_SUCCESS) {
    bool data30_ok = true;
    for (int i = 0; i < data30.size(); i++) {
      int expected = rank * 3000 + i;
      if (data30[i] != expected) {
        data30_ok = false;
        break;
      }
    }
    if (!data30_ok) {
      fprintf(
        stderr, "Rank %d: FAILURE - member 30 data verification failed\n", rank
      );
      MPI_Abort(res_comm, 1);
    }
  }

  // MPI communication: rank 1 collects results from rank 0
  int rank0_results[3] = {0, 0, 0};
  int rank1_results[3] = {ret_member10, ret_member20, ret_member30};

  if (rank == 0) {
    // Send results to rank 1
    int my_results[3] = {ret_member10, ret_member20, ret_member30};
    MPI_Send(my_results, 3, MPI_INT, 1, 0, res_comm);
  } else {
    // Rank 1 receives results from rank 0
    MPI_Recv(rank0_results, 3, MPI_INT, 0, 0, res_comm, MPI_STATUS_IGNORE);
  }

  // Rank 1 verifies all results and prints report
  if (rank == 1) {
    fprintf(stderr, "\nRestore results:\n");
    fprintf(
      stderr, "  Rank 0: member 10=%d, member 20=%d, member 30=%d\n",
      rank0_results[0], rank0_results[1], rank0_results[2]
    );
    fprintf(
      stderr, "  Rank 1: member 10=%d, member 20=%d, member 30=%d\n",
      rank1_results[0], rank1_results[1], rank1_results[2]
    );
    fprintf(stderr, "  (0=SUCCESS, 101=PARTIAL_RESTORE)\n\n");

    // Verify expected behavior
    bool test_passed = true;

    // Member 20 must succeed on both ranks (prestaged on both)
    if (rank0_results[1] != FENIX_SUCCESS) {
      fprintf(
        stderr, "ERROR: Member 20 on rank 0 returned %d (expected SUCCESS=0)\n",
        rank0_results[1]
      );
      test_passed = false;
    }
    if (rank1_results[1] != FENIX_SUCCESS) {
      fprintf(
        stderr, "ERROR: Member 20 on rank 1 returned %d (expected SUCCESS=0)\n",
        rank1_results[1]
      );
      test_passed = false;
    }

    // Member 10: rank 0 should succeed (prestaged), rank 1 may warn (not
    // prestaged)
    if (rank0_results[0] != FENIX_SUCCESS) {
      fprintf(
        stderr,
        "ERROR: Member 10 on rank 0 returned %d (expected SUCCESS=0, prestaged "
        "on rank 0)\n",
        rank0_results[0]
      );
      test_passed = false;
    }
    if (rank1_results[0] != FENIX_SUCCESS &&
        rank1_results[0] != FENIX_WARNING_PARTIAL_RESTORE) {
      fprintf(
        stderr,
        "ERROR: Member 10 on rank 1 returned %d (expected SUCCESS or "
        "PARTIAL_RESTORE)\n",
        rank1_results[0]
      );
      test_passed = false;
    }

    // Member 30: rank 1 should succeed (prestaged), rank 0 may warn (not
    // prestaged)
    if (rank1_results[2] != FENIX_SUCCESS) {
      fprintf(
        stderr,
        "ERROR: Member 30 on rank 1 returned %d (expected SUCCESS=0, prestaged "
        "on rank 1)\n",
        rank1_results[2]
      );
      test_passed = false;
    }
    if (rank0_results[2] != FENIX_SUCCESS &&
        rank0_results[2] != FENIX_WARNING_PARTIAL_RESTORE) {
      fprintf(
        stderr,
        "ERROR: Member 30 on rank 0 returned %d (expected SUCCESS or "
        "PARTIAL_RESTORE)\n",
        rank0_results[2]
      );
      test_passed = false;
    }

    if (!test_passed) {
      fprintf(stderr, "\nTest FAILED: Unexpected return codes detected\n");
      MPI_Abort(res_comm, 1);
    }

    fprintf(stderr, "Verification:\n");
    fprintf(
      stderr,
      "  - Member 20: SUCCESS on both ranks (prestaged on both) - PASS\n"
    );
    fprintf(
      stderr, "  - Member 10: SUCCESS on rank 0, code %d on rank 1 - PASS\n",
      rank1_results[0]
    );
    fprintf(
      stderr, "  - Member 30: code %d on rank 0, SUCCESS on rank 1 - PASS\n",
      rank0_results[2]
    );
    fprintf(
      stderr,
      "\nTest passed! checkpointv(SUBSET_PRESTAGED) correctly handled "
      "different prestaged subsets per rank.\n"
    );
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
