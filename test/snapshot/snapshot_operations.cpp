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
#include <vector>
#include <iostream>
#include <algorithm>

constexpr int my_group  = 0;
constexpr int my_member = 0;

using fenix::DataSubset;
using namespace fenix::data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Create a group with depth=3 to hold 3 snapshots
  group_create(my_group, {.depth = 3});

  std::vector<int> data;

  // Create 3 checkpoints with different data
  if (rank == 0) fprintf(stderr, "Creating checkpoint 0 with value 0xAAAAAAAA\n");
  data.resize(10);
  for (int& i : data) i = 0xAAAAAAAA;
  member_define(my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT);
  checkpoint(my_group, DataSubset({0, 9}));

  if (rank == 0) fprintf(stderr, "Creating checkpoint 1 with value 0xBBBBBBBB\n");
  for (int& i : data) i = 0xBBBBBBBB;
  checkpoint(my_group, DataSubset({0, 9}));

  if (rank == 0) fprintf(stderr, "Creating checkpoint 2 with value 0xCCCCCCCC\n");
  for (int& i : data) i = 0xCCCCCCCC;
  checkpoint(my_group, DataSubset({0, 9}));

  // Test group_snapshots() returns {2, 1, 0} (newest to oldest)
  if (rank == 0) fprintf(stderr, "Testing group_snapshots() returns {2, 1, 0}\n");
  std::vector<int> snapshots = *group_snapshots(my_group);
  fenix_require(snapshots.size() == 3, "Expected 3 snapshots");
  fenix_require(snapshots[0] == 2, "Expected first snapshot to be 2 (newest)");
  fenix_require(snapshots[1] == 1, "Expected second snapshot to be 1");
  fenix_require(snapshots[2] == 0, "Expected third snapshot to be 0 (oldest)");

  // Delete timestamp 1
  if (rank == 0) fprintf(stderr, "Deleting snapshot with timestamp 1\n");
  snapshot_delete(my_group, 1);

  // Verify group_snapshots() now returns {2, 0} (newest to oldest)
  if (rank == 0) fprintf(stderr, "Testing group_snapshots() returns {2, 0}\n");
  snapshots = *group_snapshots(my_group);
  fenix_require(snapshots.size() == 2, "Expected 2 snapshots after deletion");
  fenix_require(snapshots[0] == 2, "Expected first snapshot to be 2 (newest)");
  fenix_require(snapshots[1] == 0, "Expected second snapshot to be 0 (oldest)");

  // Verify we can still load from the remaining snapshots
  if (rank == 0) fprintf(stderr, "Verifying snapshot 0 can still be loaded\n");
  for (int& i : data) i = 0;
  member_load(my_group, my_member, 0);
  for (int i = 0; i < 10; i++) {
    fenix_require(data[i] == 0xAAAAAAAA, "Expected data from snapshot 0");
  }

  if (rank == 0) fprintf(stderr, "Verifying snapshot 2 can still be loaded\n");
  for (int& i : data) i = 0;
  member_load(my_group, my_member, 2);
  for (int i = 0; i < 10; i++) {
    fenix_require(data[i] == 0xCCCCCCCC, "Expected data from snapshot 2");
  }

  // Test error: snapshot_delete with invalid timestamp (-1) should throw
  if (rank == 0) fprintf(stderr, "Testing snapshot_delete(-1) throws FENIX_ERROR_INVALID_TIMESTAMP\n");
  bool exception_caught = false;
  try {
    snapshot_delete(my_group, -1);
    if (rank == 0) fprintf(stderr, "ERROR: snapshot_delete(-1) did not throw exception\n");
  } catch (const fenix::RuntimeException& e) {
    exception_caught = true;
    if (rank == 0) {
      fprintf(stderr, "SUCCESS: Caught expected exception: %s\n", e.what());
      fenix_require(e.error == FENIX_ERROR_INVALID_TIMESTAMP,
                    "Expected FENIX_ERROR_INVALID_TIMESTAMP");
    }
  }
  fenix_require(exception_caught, "Expected exception to be thrown");

  if (rank == 0) fprintf(stderr, "All snapshot operation tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();

  return 0;
}
