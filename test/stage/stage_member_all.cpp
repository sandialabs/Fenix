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

constexpr int my_group = 0;
constexpr int member_0 = 0;
constexpr int member_1 = 1;
constexpr int member_2 = 2;

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
    if (rank == 0) fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  if (rank == 0) fprintf(stderr, "Test: member_store with FENIX_DATA_MEMBER_ALL\n");

  // Create group
  group_create(my_group, {.depth = 1});

  // Create 3 members with different sizes: 50, 75, 100 elements
  std::vector<int> data0(50);
  std::vector<int> data1(75);
  std::vector<int> data2(100);

  // Initialize each member with distinct data patterns
  // member 0: rank*100+i
  for (int i = 0; i < data0.size(); i++) {
    data0[i] = rank * 100 + i;
  }

  // member 1: rank*200+i
  for (int i = 0; i < data1.size(); i++) {
    data1[i] = rank * 200 + i;
  }

  // member 2: rank*300+i
  for (int i = 0; i < data2.size(); i++) {
    data2[i] = rank * 300 + i;
  }

  if (rank == 0) fprintf(stderr, "Creating 3 members with sizes 50, 75, 100\n");

  // Create the three members
  member_create(my_group, member_0, data0.data(), data0.size(), MPI_INT);
  member_create(my_group, member_1, data1.data(), data1.size(), MPI_INT);
  member_create(my_group, member_2, data2.data(), data2.size(), MPI_INT);

  // Store all members at once using FENIX_DATA_MEMBER_ALL
  if (rank == 0) fprintf(stderr, "Storing all members with FENIX_DATA_MEMBER_ALL\n");

  int ret = member_store(my_group, FENIX_DATA_MEMBER_ALL, SUBSET_FULL);

  if (ret != FENIX_SUCCESS) {
    fprintf(stderr, "Rank %d: member_store failed with code %d\n", rank, ret);
    MPI_Abort(res_comm, 1);
  }

  // Commit the checkpoint
  commit(my_group);

  if (rank == 0) fprintf(stderr, "Checkpoint succeeded, clearing data\n");

  // Clear all member data
  for (int& val : data0) val = -1;
  for (int& val : data1) val = -1;
  for (int& val : data2) val = -1;

  // Restore each member individually
  if (rank == 0) fprintf(stderr, "Restoring members individually\n");

  DataSubset restored_subset;

  ret = member_restore(
    my_group, member_0, data0.data(), data0.size(),
    FENIX_DATA_SNAPSHOT_LATEST, restored_subset
  );
  if (ret != FENIX_SUCCESS) {
    fprintf(stderr, "Rank %d: member_restore for member 0 failed with code %d\n", rank, ret);
    MPI_Abort(res_comm, 1);
  }

  ret = member_restore(
    my_group, member_1, data1.data(), data1.size(),
    FENIX_DATA_SNAPSHOT_LATEST, restored_subset
  );
  if (ret != FENIX_SUCCESS) {
    fprintf(stderr, "Rank %d: member_restore for member 1 failed with code %d\n", rank, ret);
    MPI_Abort(res_comm, 1);
  }

  ret = member_restore(
    my_group, member_2, data2.data(), data2.size(),
    FENIX_DATA_SNAPSHOT_LATEST, restored_subset
  );
  if (ret != FENIX_SUCCESS) {
    fprintf(stderr, "Rank %d: member_restore for member 2 failed with code %d\n", rank, ret);
    MPI_Abort(res_comm, 1);
  }

  // Verify all 3 members were stored and restored correctly
  if (rank == 0) fprintf(stderr, "Verifying restored data\n");

  bool success = true;

  // Verify member 0: rank*100+i
  for (int i = 0; i < data0.size(); i++) {
    if (data0[i] != rank * 100 + i) {
      fprintf(stderr, "Rank %d: member 0 data mismatch at index %d! Expected %d, got %d\n",
              rank, i, rank * 100 + i, data0[i]);
      success = false;
      break;
    }
  }

  // Verify member 1: rank*200+i
  for (int i = 0; i < data1.size(); i++) {
    if (data1[i] != rank * 200 + i) {
      fprintf(stderr, "Rank %d: member 1 data mismatch at index %d! Expected %d, got %d\n",
              rank, i, rank * 200 + i, data1[i]);
      success = false;
      break;
    }
  }

  // Verify member 2: rank*300+i
  for (int i = 0; i < data2.size(); i++) {
    if (data2[i] != rank * 300 + i) {
      fprintf(stderr, "Rank %d: member 2 data mismatch at index %d! Expected %d, got %d\n",
              rank, i, rank * 300 + i, data2[i]);
      success = false;
      break;
    }
  }

  if (success) {
    if (rank == 0) fprintf(stderr, "All data verified successfully!\n");
    if (rank == 0) fprintf(stderr, "Test passed: FENIX_DATA_MEMBER_ALL stored and restored all 3 members\n");
  } else {
    fprintf(stderr, "Rank %d: FAILURE - data verification failed\n", rank);
    MPI_Abort(res_comm, 1);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
