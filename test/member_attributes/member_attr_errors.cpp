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
#include <fenix_exception.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace fenix::data;

constexpr int my_group = 0;

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

  group_create(my_group, {.depth = 1});

  std::vector<int> data(100);
  for (int i = 0; i < 100; i++) data[i] = rank * 1000 + i;

  member_create(my_group, 1, data.data(), 100, MPI_INT);

  // Stage the member first - this locks the count and datatype
  checkpoint(my_group, SUBSET_FULL);

  //===========================================================================
  // Test 1: Try to change count to DIFFERENT value after staging
  //===========================================================================
  if (rank == 0) fprintf(stderr, "Test 1: Change count to different value after staging\n");

  // Try to change count to a different value - should throw FENIX_ERROR_INVALID_LOGIC_CALL
  int new_count = 50;
  int flag = 0;
  bool caught_exception = false;

  try {
    member_attr_set(my_group, 1, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, &new_count, &flag);
    if (rank == 0) fprintf(stderr, "  ERROR: Count change after staging should have thrown!\n");
    MPI_Abort(res_comm, 1);
  } catch (const fenix::RuntimeException& e) {
    caught_exception = true;
    if (rank == 0) fprintf(stderr, "  Caught exception: %s\n", e.what());
    // Verify it's the right error code
    if (e.error != FENIX_ERROR_INVALID_LOGIC_CALL) {
      if (rank == 0) fprintf(stderr, "  ERROR: Expected FENIX_ERROR_INVALID_LOGIC_CALL, got %d\n", e.error);
      MPI_Abort(res_comm, 1);
    }
  }

  if (!caught_exception) {
    if (rank == 0) fprintf(stderr, "  ERROR: Count change after staging should have thrown!\n");
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0) fprintf(stderr, "  ✓ Count change correctly rejected after staging\n");

  //===========================================================================
  // Test 2: Try to change datatype to DIFFERENT size after staging
  //===========================================================================
  if (rank == 0) fprintf(stderr, "Test 2: Change datatype to different size after staging\n");

  // Try to change datatype to a different size - should throw FENIX_ERROR_INVALID_LOGIC_CALL
  MPI_Datatype new_dtype = MPI_DOUBLE; // Different size from MPI_INT
  caught_exception = false;

  try {
    member_attr_set(my_group, 1, FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE, &new_dtype, &flag);
    if (rank == 0) fprintf(stderr, "  ERROR: Datatype change after staging should have thrown!\n");
    MPI_Abort(res_comm, 1);
  } catch (const fenix::RuntimeException& e) {
    caught_exception = true;
    if (rank == 0) fprintf(stderr, "  Caught exception: %s\n", e.what());
    // Verify it's the right error code
    if (e.error != FENIX_ERROR_INVALID_LOGIC_CALL) {
      if (rank == 0) fprintf(stderr, "  ERROR: Expected FENIX_ERROR_INVALID_LOGIC_CALL, got %d\n", e.error);
      MPI_Abort(res_comm, 1);
    }
  }

  if (!caught_exception) {
    if (rank == 0) fprintf(stderr, "  ERROR: Datatype change after staging should have thrown!\n");
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0) fprintf(stderr, "  ✓ Datatype change correctly rejected after staging\n");

  //===========================================================================
  // Test 3: Verify changing count to SAME value after staging is allowed
  //===========================================================================
  if (rank == 0) fprintf(stderr, "Test 3: Change count to same value after staging (should be allowed)\n");

  // Set count to the SAME value (100) - should be allowed
  new_count = 100;
  try {
    member_attr_set(my_group, 1, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, &new_count, &flag);
    if (rank == 0) fprintf(stderr, "  ✓ Setting count to same value allowed\n");
  } catch (const fenix::RuntimeException& e) {
    if (rank == 0) {
      fprintf(stderr, "  ERROR: Setting count to same value should be allowed!\n");
      fprintf(stderr, "  Caught exception: %s\n", e.what());
    }
    MPI_Abort(res_comm, 1);
  }

  //===========================================================================
  // Test 4: Verify changing datatype to SAME size after staging is allowed
  //===========================================================================
  if (rank == 0) fprintf(stderr, "Test 4: Change datatype to same size after staging (should be allowed)\n");

  // Set datatype to the SAME type (MPI_INT) - should be allowed
  new_dtype = MPI_INT;
  try {
    member_attr_set(my_group, 1, FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE, &new_dtype, &flag);
    if (rank == 0) fprintf(stderr, "  ✓ Setting datatype to same type allowed\n");
  } catch (const fenix::RuntimeException& e) {
    if (rank == 0) {
      fprintf(stderr, "  ERROR: Setting datatype to same type should be allowed!\n");
      fprintf(stderr, "  Caught exception: %s\n", e.what());
    }
    MPI_Abort(res_comm, 1);
  }

  member_delete(my_group, 1);

  if (rank == 0) fprintf(stderr, "\nAll member attribute error tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
