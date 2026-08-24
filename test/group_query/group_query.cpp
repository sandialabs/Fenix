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
#include <set>
#include <iostream>

constexpr int my_group = 0;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  if (rank == 0) fprintf(stderr, "Create group\n");
  fenix::data::group_create(my_group, {.depth = 0});

  if (rank == 0) fprintf(stderr, "Create members with IDs 10, 20, 30\n");
  std::vector<int> data1(10, 1);
  std::vector<int> data2(20, 2);
  std::vector<int> data3(30, 3);

  fenix::data::member_create(my_group, 10, data1.data(), 10, MPI_INT);
  fenix::data::member_create(my_group, 20, data2.data(), 20, MPI_INT);
  fenix::data::member_create(my_group, 30, data3.data(), 30, MPI_INT);

  if (rank == 0) fprintf(stderr, "Get number of members\n");
  int num_members = 0;
  int retcode = Fenix_Data_group_get_number_of_members(my_group, &num_members);
  fenix_require(retcode == FENIX_SUCCESS);
  fenix_require(num_members == 3);
  if (rank == 0)
    fprintf(stderr, "Number of members: %d (expected 3)\n", num_members);

  if (rank == 0) fprintf(stderr, "Iterate through members by position\n");
  std::set<int> collected_ids;
  for (int pos = 0; pos < num_members; pos++) {
    int member_id = -1;
    retcode =
      Fenix_Data_group_get_member_at_position(my_group, &member_id, pos);
    fenix_require(retcode == FENIX_SUCCESS);
    collected_ids.insert(member_id);
    if (rank == 0)
      fprintf(stderr, "Position %d: member_id = %d\n", pos, member_id);
  }

  if (rank == 0) fprintf(stderr, "Verify collected IDs match {10, 20, 30}\n");
  std::set<int> expected_ids = {10, 20, 30};
  fenix_require(collected_ids == expected_ids);

  if (rank == 0)
    fprintf(
      stderr, "Test error: get_member_at_position with invalid position (-1)\n"
    );
  int member_id     = -1;
  bool caught_error = false;
  try {
    retcode = Fenix_Data_group_get_member_at_position(my_group, &member_id, -1);
    // If using C API without exceptions, check error code
    if (retcode != FENIX_SUCCESS) {
      caught_error = true;
      if (rank == 0)
        fprintf(
          stderr, "Correctly returned error for position -1: %d\n", retcode
        );
    }
  } catch (const fenix::RuntimeException& e) {
    caught_error = true;
    if (rank == 0)
      fprintf(
        stderr, "Correctly caught exception for position -1: %s\n", e.what()
      );
  }
  fenix_require(caught_error);

  if (rank == 0)
    fprintf(
      stderr, "Test error: get_member_at_position with position >= size\n"
    );
  caught_error = false;
  try {
    retcode = Fenix_Data_group_get_member_at_position(
      my_group, &member_id, num_members
    );
    // If using C API without exceptions, check error code
    if (retcode != FENIX_SUCCESS) {
      caught_error = true;
      if (rank == 0)
        fprintf(
          stderr, "Correctly returned error for position %d: %d\n", num_members,
          retcode
        );
    }
  } catch (const fenix::RuntimeException& e) {
    caught_error = true;
    if (rank == 0)
      fprintf(
        stderr, "Correctly caught exception for position %d: %s\n", num_members,
        e.what()
      );
  }
  fenix_require(caught_error);

  if (rank == 0) fprintf(stderr, "All tests passed!\n");

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
