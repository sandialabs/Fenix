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
#include <iostream>

using namespace fenix::data;

constexpr int my_group    = 0;
constexpr int member_id_1 = 10;
constexpr int member_id_2 = 20;
constexpr int member_id_3 = 30;

// Global vector to track the order in which members are staged/serialized
std::vector<int> staged_members_order;

// Data for each member
std::vector<int> data1;
std::vector<int> data2;
std::vector<int> data3;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  if (num_ranks != 2) {
    if (rank == 0)
      fprintf(stderr, "SKIP: This test requires exactly 2 ranks\n");
    Fenix_Finalize();
    MPI_Finalize();
    return 0;
  }

  if (rank == 0)
    fprintf(stderr, "Test: checkpoint stores members in creation order\n");

  // Create group
  group_create(my_group, {.depth = 1});

  // Initialize data for each member
  data1 = {100, 101, 102, 103, 104, 105, 106, 107, 108, 109};
  data2 = {200, 201, 202, 203, 204, 205, 206, 207,
           208, 209, 210, 211, 212, 213, 214};
  data3 = {300, 301, 302, 303, 304, 305, 306, 307, 308, 309,
           310, 311, 312, 313, 314, 315, 316, 317, 318, 319};

  // Create 3 members with custom serializers that track staging order
  if (rank == 0)
    fprintf(stderr, "  Creating members with custom serializers\n");

  // Member 1 with custom serializer
  member_define(
    my_group, member_id_1, nullptr, FENIX_RESIZEABLE, MPI_INT,
    [](std::iostream& strm, int dir, void* b, int offset, int count) {
      fenix_require(offset == 0 && b == nullptr);
      if (dir == FENIX_SERIALIZE) {
        staged_members_order.push_back(member_id_1);
        int size = data1.size();
        strm.write((char*)&size, sizeof(int));
        strm.write((char*)data1.data(), sizeof(int) * size);
      } else {
        int size;
        strm.read((char*)&size, sizeof(int));
        data1.resize(size);
        strm.read((char*)data1.data(), sizeof(int) * size);
      }
    }
  );

  // Member 2 with custom serializer
  member_define(
    my_group, member_id_2, nullptr, FENIX_RESIZEABLE, MPI_INT,
    [](std::iostream& strm, int dir, void* b, int offset, int count) {
      fenix_require(offset == 0 && b == nullptr);
      if (dir == FENIX_SERIALIZE) {
        staged_members_order.push_back(member_id_2);
        int size = data2.size();
        strm.write((char*)&size, sizeof(int));
        strm.write((char*)data2.data(), sizeof(int) * size);
      } else {
        int size;
        strm.read((char*)&size, sizeof(int));
        data2.resize(size);
        strm.read((char*)data2.data(), sizeof(int) * size);
      }
    }
  );

  // Member 3 with custom serializer
  member_define(
    my_group, member_id_3, nullptr, FENIX_RESIZEABLE, MPI_INT,
    [](std::iostream& strm, int dir, void* b, int offset, int count) {
      fenix_require(offset == 0 && b == nullptr);
      if (dir == FENIX_SERIALIZE) {
        staged_members_order.push_back(member_id_3);
        int size = data3.size();
        strm.write((char*)&size, sizeof(int));
        strm.write((char*)data3.data(), sizeof(int) * size);
      } else {
        int size;
        strm.read((char*)&size, sizeof(int));
        data3.resize(size);
        strm.read((char*)data3.data(), sizeof(int) * size);
      }
    }
  );

  // Checkpoint and verify creation order
  if (rank == 0) fprintf(stderr, "  Calling checkpoint\n");

  staged_members_order.clear();

  int timestamp = -1;
  int ret       = checkpoint(my_group, SUBSET_FULL, {}, &timestamp);

  if (ret != FENIX_SUCCESS) {
    fprintf(
      stderr, "Rank %d: ERROR - checkpoint failed with code %d\n", rank, ret
    );
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0) {
    fprintf(stderr, "  Checkpoint succeeded at timestamp %d\n", timestamp);
    fprintf(stderr, "  Staged members order: [");
    for (size_t i = 0; i < staged_members_order.size(); i++) {
      fprintf(stderr, "%d", staged_members_order[i]);
      if (i < staged_members_order.size() - 1) fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
  }

  // Verify order is {10, 20, 30} (creation order)
  if (staged_members_order.size() != 3) {
    fprintf(
      stderr, "Rank %d: ERROR - expected 3 staged members, got %zu\n", rank,
      staged_members_order.size()
    );
    MPI_Abort(res_comm, 1);
  }

  if (staged_members_order[0] != member_id_1 ||
      staged_members_order[1] != member_id_2 ||
      staged_members_order[2] != member_id_3) {
    fprintf(
      stderr,
      "Rank %d: ERROR - expected order [10, 20, 30], got [%d, %d, %d]\n", rank,
      staged_members_order[0], staged_members_order[1], staged_members_order[2]
    );
    MPI_Abort(res_comm, 1);
  }

  if (rank == 0)
    fprintf(stderr, "  ✓ Members stored in creation order: [10, 20, 30]\n");

  // Verify data integrity
  if (rank == 0) fprintf(stderr, "  Verifying data integrity\n");

  data1.clear();
  data2.clear();
  data3.clear();
  member_restore(
    my_group, member_id_1, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    timestamp
  );
  member_restore(
    my_group, member_id_2, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    timestamp
  );
  member_restore(
    my_group, member_id_3, FENIX_DATA_RESTORE_INPLACE, FENIX_DATA_RESTORE_FULL,
    timestamp
  );

  bool data_ok = true;
  for (int i = 0; i < 10 && data_ok; i++) {
    if (data1[i] != 100 + i) {
      fprintf(
        stderr, "Rank %d: ERROR - member 10 data[%d] = %d, expected %d\n", rank,
        i, data1[i], 100 + i
      );
      data_ok = false;
    }
  }
  for (int i = 0; i < 15 && data_ok; i++) {
    if (data2[i] != 200 + i) {
      fprintf(
        stderr, "Rank %d: ERROR - member 20 data[%d] = %d, expected %d\n", rank,
        i, data2[i], 200 + i
      );
      data_ok = false;
    }
  }
  for (int i = 0; i < 20 && data_ok; i++) {
    if (data3[i] != 300 + i) {
      fprintf(
        stderr, "Rank %d: ERROR - member 30 data[%d] = %d, expected %d\n", rank,
        i, data3[i], 300 + i
      );
      data_ok = false;
    }
  }

  if (!data_ok) MPI_Abort(res_comm, 1);

  if (rank == 0) fprintf(stderr, "  ✓ All data restored correctly\n");

  if (rank == 0)
    fprintf(
      stderr, "\nTest passed! Members are checkpointed in creation order.\n"
    );

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
