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
#include <mpi.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

const int kKillID = 1;

int main(int argc, char **argv) {

#warning "It's a good idea to complain when not enough parameters! Should add this code to other examples too."
  if (argc < 2) {
      printf("Usage: %s <# spare ranks> \n", *argv);
      exit(0);
  }

  int old_world_size, new_world_size = - 1;
  int old_rank = 1, new_rank = - 1;
  int spare_ranks = atoi(argv[1]);
  int buffer;

  MPI_Init(&argc, &argv);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm world_comm;
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  MPI_Comm_size(world_comm, &old_world_size);
  MPI_Comm_rank(world_comm, &old_rank);

  MPI_Info info;
  MPI_Info_create(&info);
  MPI_Info_set(info, "FENIX_RESUME_MODE", "NO_JUMP");

  int fenix_status;
  int recovered = 0;
  MPI_Comm new_comm;
  int error;
  Fenix_Init(&fenix_status, world_comm, &new_comm, &argc, &argv, spare_ranks, 0, info, &error);

  MPI_Comm_size(new_comm, &new_world_size);
  MPI_Comm_rank(new_comm, &new_rank);

  if (old_rank == kKillID) {
    assert(fenix_status == FENIX_ROLE_INITIAL_RANK);
    pid_t pid = getpid();
    kill(pid, SIGTERM);
  }

  if(new_rank == kKillID) {
      assert(fenix_status == FENIX_ROLE_RECOVERED_RANK);
      int sval = 33;
      MPI_Send(&sval, 1, MPI_INT, kKillID-1, 1, new_comm);
  }
  else if(new_rank == kKillID-1) {
      assert(fenix_status == FENIX_ROLE_INITIAL_RANK);
      int rval = 44;
      MPI_Status status;
      MPI_Recv(&rval, 1, MPI_INT, kKillID, 1, new_comm, &status);

      assert(fenix_status == FENIX_ROLE_SURVIVOR_RANK);
      assert(rval == 44);
      printf("Rank %d did not receive new value. old value is %d\n", new_rank, rval);

      MPI_Recv(&rval, 1, MPI_INT, kKillID, 1, new_comm, &status);
      assert(rval == 33);
      printf("Rank %d received new value %d\n", new_rank, rval);
  }
  else {
      assert(fenix_status == FENIX_ROLE_INITIAL_RANK);
      MPI_Barrier(new_comm);
      assert(fenix_status == FENIX_ROLE_SURVIVOR_RANK);
  }

  MPI_Barrier(new_comm);

  Fenix_Finalize();
  MPI_Finalize();

  return 0;
}

