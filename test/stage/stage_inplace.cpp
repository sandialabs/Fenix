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
constexpr int my_member = 0;

using fenix::DataSubset;
using namespace fenix::data;

std::vector<int> buffer_a;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  // Use only 2 ranks as specified
  if (num_ranks != 2) {
    if (rank == 0)
      fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  if (rank == 0)
    fprintf(
      stderr, "Test: member_stage_inplace with dynamically allocated buffer\n"
    );

  // Create a group with depth=1
  group_create(my_group, {.depth = 1});

  // Create buffer A with 100 elements (initial buffer)
  buffer_a.resize(100);
  for (int i = 0; i < buffer_a.size(); i++) {
    buffer_a[i] = rank * 1000 + i; // Values like 0, 1, 2, ..., 99 for rank 0
  }

  if (rank == 0) {
    fprintf(
      stderr, "Buffer A[0]=%d, Buffer A[99]=%d\n", buffer_a[0], buffer_a[99]
    );
  }

  // Create a member with initial buffer A
  member_create(my_group, my_member, buffer_a.data(), 100, MPI_INT);

  // Allocate buffer_b with malloc() - Fenix will take ownership and call free()
  // IMPORTANT: Must use malloc(), not new, because Fenix uses free() internally
  int* buffer_b = (int*)malloc(100 * sizeof(int));
  if (!buffer_b) {
    fprintf(stderr, "Rank %d: Failed to allocate buffer_b\n", rank);
    MPI_Abort(res_comm, 1);
  }

  for (int i = 0; i < 100; i++) {
    buffer_b[i] =
      (rank + 1) * 10000 + i; // Values like 10000, 10001, ..., 10099 for rank 0
  }

  if (rank == 0) {
    fprintf(
      stderr, "Buffer B[0]=%d, Buffer B[99]=%d\n", buffer_b[0], buffer_b[99]
    );
  }

  // Save buffer B values for later verification
  std::vector<int> buffer_b_values(buffer_b, buffer_b + 100);

  // Call member_stage_inplace with buffer B - Fenix takes ownership!
  // Do NOT free buffer_b after this call - Fenix owns it and will call free()
  if (rank == 0)
    fprintf(
      stderr,
      "Calling member_stage_inplace (Fenix takes ownership of buffer_b)\n"
    );
  member_stage_inplace(my_group, my_member, buffer_b, SUBSET_FULL);

  // Store the staged data
  member_store(my_group, my_member, SUBSET_PRESTAGED);

  // Commit the checkpoint
  commit(my_group);

  if (rank == 0) fprintf(stderr, "Checkpoint committed\n");

  // Clear buffer A to verify we're getting data from checkpoint
  for (int i = 0; i < buffer_a.size(); i++) {
    buffer_a[i] = -8888;
  }

  if (rank == 0) fprintf(stderr, "Cleared buffer A\n");

  // Restore data using INPLACE - restores to the original member buffer
  // (buffer_a)
  member_restore(
    my_group, my_member, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    FENIX_DATA_SNAPSHOT_LATEST
  );

  if (rank == 0) fprintf(stderr, "Data restored to buffer A\n");

  // Verify restored data in buffer_a matches what was in buffer_b
  bool success = true;
  for (int i = 0; i < buffer_a.size(); i++) {
    int expected = buffer_b_values[i]; // This was buffer_b's original value
    if (buffer_a[i] != expected) {
      fprintf(
        stderr,
        "Rank %d: Mismatch at index %d! Expected %d (from buffer B), got %d\n",
        rank, i, expected, buffer_a[i]
      );
      success = false;
      break;
    }
  }

  if (success) {
    if (rank == 0)
      fprintf(
        stderr,
        "SUCCESS: Buffer A now contains data from buffer B (stage_inplace "
        "worked correctly)\n"
      );
    printf("Rank %d: Test passed\n", rank);
  } else {
    fprintf(stderr, "FAILURE on rank %d: Data does not match buffer B\n", rank);
    printf("Rank %d: Test FAILED\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return success ? 0 : 1;
}
