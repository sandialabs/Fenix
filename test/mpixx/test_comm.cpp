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
// Questions? Contact Matthew Whitlock (mwhitlo@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <fenix/mpixx/comm.hpp>
#include <fenix_opt.hpp>
#include <mpi.h>
#include <cstdio>
#include <utility>

using fenix::mpixx::Comm;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int world_rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Test 1: Construction and basic accessors
  {
    Comm empty_comm;
    fenix_require(!empty_comm, "Empty comm should be falsy");
    fenix_require(
      empty_comm.get() == MPI_COMM_NULL, "Empty comm should have MPI_COMM_NULL"
    );
  }

  // Test 2: Wrapping MPI_COMM_WORLD (without taking ownership)
  {
    Comm world_comm(MPI_COMM_WORLD);
    fenix_require(world_comm, "World comm should be truthy");
    fenix_require(
      world_comm.get() == MPI_COMM_WORLD,
      "World comm get() should return MPI_COMM_WORLD"
    );
    // Test implicit conversion
    MPI_Comm raw = world_comm;
    fenix_require(
      raw == MPI_COMM_WORLD, "Implicit conversion should return MPI_COMM_WORLD"
    );
    fenix_require(
      world_comm.size() == world_size,
      "World comm size() should match world_size"
    );
    fenix_require(
      world_comm.rank() == world_rank,
      "World comm rank() should match world_rank"
    );

    // Release to prevent freeing MPI_COMM_WORLD
    MPI_Comm released = world_comm.release();
    fenix_require(
      released == MPI_COMM_WORLD, "release() should return the original comm"
    );
    fenix_require(!world_comm, "After release(), comm should be empty");
  }

  // Test 3: Member function dup()
  {
    Comm world_comm(MPI_COMM_WORLD);

    Comm dup_comm = world_comm.dup();
    fenix_require(dup_comm, "Member dup() should create valid comm");
    fenix_require(
      dup_comm.get() != MPI_COMM_NULL, "Dup comm should not be MPI_COMM_NULL"
    );
    fenix_require(
      dup_comm.get() != MPI_COMM_WORLD,
      "Dup comm should be different from MPI_COMM_WORLD"
    );
    fenix_require(
      dup_comm.size() == world_size, "Dup comm should have same size as world"
    );
    fenix_require(
      dup_comm.rank() == world_rank, "Dup comm should have same rank as world"
    );

    // Release world_comm to prevent freeing MPI_COMM_WORLD
    world_comm.release();
    // dup_comm will be freed automatically
  }

  // Test 4: Static function dup()
  {
    Comm dup_comm = Comm::dup(MPI_COMM_WORLD);
    fenix_require(dup_comm, "Static dup() should create valid comm");
    fenix_require(
      dup_comm.size() == world_size, "Static dup comm should have correct size"
    );
    fenix_require(
      dup_comm.rank() == world_rank, "Static dup comm should have correct rank"
    );
  }

  // Test 5: Member function split()
  {
    Comm world_comm(MPI_COMM_WORLD);

    int color       = world_rank % 2;
    Comm split_comm = world_comm.split(color, world_rank);
    fenix_require(split_comm, "Member split() should create valid comm");
    fenix_require(
      split_comm.size() > 0, "Split comm should have non-zero size"
    );
    fenix_require(
      split_comm.size() <= world_size,
      "Split comm size should not exceed world size"
    );
    fenix_require(split_comm.rank() >= 0, "Split comm should have valid rank");

    world_comm.release();
  }

  // Test 6: Static function split()
  {
    int color       = world_rank / (world_size / 2 + 1); // All in color 0 or 1
    Comm split_comm = Comm::split(MPI_COMM_WORLD, color, world_rank);
    fenix_require(split_comm, "Static split() should create valid comm");
    fenix_require(
      split_comm.size() > 0, "Static split comm should have non-zero size"
    );
    fenix_require(
      split_comm.rank() >= 0, "Static split comm should have valid rank"
    );
  }

  // Test 7: Move semantics
  {
    Comm comm1        = Comm::dup(MPI_COMM_WORLD);
    MPI_Comm raw_comm = comm1.get();
    fenix_require(raw_comm != MPI_COMM_NULL, "comm1 should have valid handle");

    Comm comm2 = std::move(comm1);
    fenix_require(!comm1, "After move construction, source should be empty");
    fenix_require(
      comm1.get() == MPI_COMM_NULL,
      "After move construction, source should have MPI_COMM_NULL"
    );
    fenix_require(
      comm2, "After move construction, destination should be valid"
    );
    fenix_require(
      comm2.get() == raw_comm, "Move constructor should preserve comm handle"
    );
  }

  // Test 8: Move assignment
  {
    Comm comm1         = Comm::dup(MPI_COMM_WORLD);
    Comm comm2         = Comm::dup(MPI_COMM_WORLD);
    MPI_Comm raw_comm1 = comm1.get();
    MPI_Comm raw_comm2 = comm2.get();

    fenix_require(raw_comm1 != MPI_COMM_NULL, "comm1 should have valid handle");
    fenix_require(raw_comm2 != MPI_COMM_NULL, "comm2 should have valid handle");
    fenix_require(
      raw_comm1 != raw_comm2, "comm1 and comm2 should be different"
    );

    comm1 = std::move(comm2);
    fenix_require(!comm2, "After move assignment, source should be empty");
    fenix_require(
      comm2.get() == MPI_COMM_NULL,
      "After move assignment, source should have MPI_COMM_NULL"
    );
    fenix_require(comm1, "After move assignment, destination should be valid");
    fenix_require(
      comm1.get() == raw_comm2, "Move assignment should transfer the handle"
    );
    // Note: raw_comm1 was freed during move assignment
  }

  // Test 9: CommRef - non-owning reference
  {
    using fenix::mpixx::CommRef;

    // CommRef should not free on destruction
    MPI_Comm dup_comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);

    {
      CommRef ref(dup_comm);
      fenix_require(ref, "CommRef should be valid");
      fenix_require(ref.get() == dup_comm, "CommRef should hold the comm");
      fenix_require(ref.size() == world_size, "CommRef size should work");
      fenix_require(ref.rank() == world_rank, "CommRef rank should work");

      // Test implicit conversion
      MPI_Comm raw = ref;
      fenix_require(raw == dup_comm, "CommRef implicit conversion should work");
    }
    // After CommRef goes out of scope, comm should still be valid
    int test_rank;
    int ret = MPI_Comm_rank(dup_comm, &test_rank);
    fenix_require(
      ret == MPI_SUCCESS, "Comm should still be valid after CommRef destruction"
    );
    fenix_require(
      test_rank == world_rank,
      "Comm should still work after CommRef destruction"
    );

    // Clean up manually
    MPI_Comm_free(&dup_comm);
  }

  // Test 10: CommRef move semantics
  {
    using fenix::mpixx::CommRef;

    MPI_Comm dup_comm1, dup_comm2;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm1);
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm2);

    CommRef ref1(dup_comm1);
    CommRef ref2(dup_comm2);

    MPI_Comm raw1 = ref1.get();
    MPI_Comm raw2 = ref2.get();

    ref1 = std::move(ref2);
    fenix_require(ref1.get() == raw2, "Move assignment should transfer handle");
    fenix_require(!ref2, "After move assignment, source should be empty");

    // Both comms should still be valid (neither was freed)
    int test_rank;
    int ret1 = MPI_Comm_rank(raw1, &test_rank);
    int ret2 = MPI_Comm_rank(raw2, &test_rank);
    fenix_require(ret1 == MPI_SUCCESS, "Original comm1 should still be valid");
    fenix_require(ret2 == MPI_SUCCESS, "Original comm2 should still be valid");

    MPI_Comm_free(&dup_comm1);
    MPI_Comm_free(&dup_comm2);
  }

  // Test 11: CommRef copy constructor
  {
    using fenix::mpixx::CommRef;

    MPI_Comm dup_comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);

    CommRef ref1(dup_comm);
    CommRef ref2(ref1); // Copy constructor

    fenix_require(
      ref1, "After copy construction, source should still be valid"
    );
    fenix_require(ref2, "After copy construction, destination should be valid");
    fenix_require(
      ref1.get() == dup_comm, "Source should still reference original comm"
    );
    fenix_require(
      ref2.get() == dup_comm, "Destination should reference same comm"
    );
    fenix_require(ref1.size() == world_size, "Source size should work");
    fenix_require(ref2.size() == world_size, "Destination size should work");

    MPI_Comm_free(&dup_comm);
  }

  // Test 12: CommRef copy assignment
  {
    using fenix::mpixx::CommRef;

    MPI_Comm dup_comm1, dup_comm2;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm1);
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm2);

    CommRef ref1(dup_comm1);
    CommRef ref2(dup_comm2);

    fenix_require(ref1.get() == dup_comm1, "ref1 should reference dup_comm1");
    fenix_require(ref2.get() == dup_comm2, "ref2 should reference dup_comm2");

    ref1 = ref2; // Copy assignment

    fenix_require(
      ref1.get() == dup_comm2,
      "After copy assignment, ref1 should reference dup_comm2"
    );
    fenix_require(
      ref2.get() == dup_comm2,
      "After copy assignment, ref2 should still reference dup_comm2"
    );
    fenix_require(ref1.size() == world_size, "ref1 size should work");
    fenix_require(ref2.size() == world_size, "ref2 size should work");

    // Both original comms should still be valid (neither was freed)
    int test_rank;
    int ret1 = MPI_Comm_rank(dup_comm1, &test_rank);
    int ret2 = MPI_Comm_rank(dup_comm2, &test_rank);
    fenix_require(ret1 == MPI_SUCCESS, "dup_comm1 should still be valid");
    fenix_require(ret2 == MPI_SUCCESS, "dup_comm2 should still be valid");

    MPI_Comm_free(&dup_comm1);
    MPI_Comm_free(&dup_comm2);
  }

  // Test 13: CommRef from Comm (const reference constructor)
  {
    using fenix::mpixx::CommRef;

    Comm owned   = Comm::dup(MPI_COMM_WORLD);
    MPI_Comm raw = owned.get();
    fenix_require(owned, "Owned comm should be valid");

    CommRef ref(owned);
    fenix_require(owned, "After creating CommRef, Comm should still be valid");
    fenix_require(ref.get() == raw, "CommRef should reference the same comm");
    fenix_require(ref.size() == world_size, "CommRef size should work");
    fenix_require(owned.size() == world_size, "Owned comm should still work");

    // owned will be freed automatically, ref won't free on destruction
  }

  // Test 14: CommRef from Comm (const reference assignment)
  {
    using fenix::mpixx::CommRef;

    MPI_Comm dup_comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);
    CommRef ref(dup_comm);

    Comm owned   = Comm::dup(MPI_COMM_WORLD);
    MPI_Comm raw = owned.get();
    fenix_require(owned, "Owned comm should be valid");

    ref = owned;
    fenix_require(
      owned, "After assigning to CommRef, Comm should still be valid"
    );
    fenix_require(ref.get() == raw, "CommRef should reference the new comm");
    fenix_require(ref.size() == world_size, "CommRef size should work");
    fenix_require(owned.size() == world_size, "Owned comm should still work");

    // Original dup_comm should still be valid (not freed by assignment)
    int test_rank;
    int ret = MPI_Comm_rank(dup_comm, &test_rank);
    fenix_require(ret == MPI_SUCCESS, "Original comm should still be valid");

    // Clean up manually
    MPI_Comm_free(&dup_comm);
    // owned will be freed automatically
  }

  // Test 15: CommRef implicit construction from MPI_Comm
  {
    using fenix::mpixx::CommRef;

    MPI_Comm dup_comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);

    // Implicit conversion
    CommRef ref = dup_comm;
    fenix_require(ref, "CommRef should be valid");
    fenix_require(ref.get() == dup_comm, "CommRef should reference the comm");
    fenix_require(ref.size() == world_size, "CommRef size should work");

    // Comm should still be valid (not freed by CommRef)
    int test_rank;
    int ret = MPI_Comm_rank(dup_comm, &test_rank);
    fenix_require(ret == MPI_SUCCESS, "Original comm should still be valid");

    // Clean up manually
    MPI_Comm_free(&dup_comm);
  }

  // Test 16: CommRef as function parameter (implicit conversion)
  {
    using fenix::mpixx::CommRef;

    auto test_func = [](CommRef ref) -> int { return ref.size(); };

    MPI_Comm dup_comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);

    // Implicit conversion when passing to function
    int size = test_func(dup_comm);
    fenix_require(
      size == world_size, "Implicit conversion in function call should work"
    );

    // Comm should still be valid
    int test_rank;
    int ret = MPI_Comm_rank(dup_comm, &test_rank);
    fenix_require(ret == MPI_SUCCESS, "Original comm should still be valid");

    MPI_Comm_free(&dup_comm);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (world_rank == 0) {
    fprintf(stderr, "=== fenix::mpixx::Comm tests PASSED ===\n");
  }

  MPI_Finalize();
  return 0;
}
