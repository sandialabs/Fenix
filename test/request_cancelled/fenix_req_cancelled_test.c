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

const int kKillID = 1;

int main(int argc, char **argv) {

  fprintf(stderr, "This is actually running\n");
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

  int fenix_status;
  int recovered = 0;
  MPI_Comm new_comm;
  int error;
  MPI_Request req = MPI_REQUEST_NULL;
  fprintf(stderr, "Before Fenix init\n");
  Fenix_Init(&fenix_status, world_comm, &new_comm, &argc, &argv, spare_ranks, 0, MPI_INFO_NULL, &error);
  fprintf(stderr, "After Fenix init\n");
    
  MPI_Comm_size(new_comm, &new_world_size);
  MPI_Comm_rank(new_comm, &new_rank);

  if (fenix_status != FENIX_ROLE_INITIAL_RANK) {
    recovered = 1;
  } else {
    MPI_Irecv(&buffer, 1, MPI_INT, (new_rank+1)%new_world_size, 1, new_comm, &req);
    //Kill rank dies before being able to send
    if(new_rank == 0 || new_rank == 2) MPI_Send(&buffer, 1, MPI_INT, old_rank==0 ? new_world_size-1 : new_rank-1, 1, new_comm);
    MPI_Barrier(new_comm);
  }
  

  if (old_rank == kKillID &&  recovered == 0) {
    fprintf(stderr, "Before kill\n");
    pid_t pid = getpid();
    kill(pid, SIGTERM);
  }
  
  
  MPI_Barrier(new_comm);

  //After recovery, the slow ranks send
  if(new_rank == 1 || new_rank == 3 ) MPI_Send(&buffer, 1, MPI_INT, new_rank==0 ? new_world_size-1 : new_rank-1, 1, new_comm);
  
  MPI_Barrier(new_comm); //Lots of barriers to demonstrate a specific ordering of events.

  //Check result of old requests - cannot wait, must MPI_Test only on old pre-failure requests for now
  if(new_rank != kKillID){
    int flag;
    int cancelled = Fenix_check_cancelled(&req, MPI_STATUS_IGNORE);
    if(cancelled){
      printf("Rank %d's request was NOT satisfied before the failure\n", new_rank);
      MPI_Irecv(&buffer, 1, MPI_INT, (new_rank+1)%new_world_size, 1, new_comm, &req); //We can re-launch the IRecv if we know the
                                                                                      //other ranks are going to send now
    } else {
      printf("Rank %d's request was satisfied before the failure\n", new_rank);
    }
    
  }

  Fenix_Finalize();
  MPI_Finalize();

  return 0;
}
