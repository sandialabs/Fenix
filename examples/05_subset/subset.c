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
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar
//        and Michael Heroux
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
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

int max_iter = 2;
const int kCount = 20;
const int kKillID = 2;

int main(int argc, char **argv) {

  int i;
  int subset[20];
  MPI_Status status;

  int fenix_role;
  MPI_Comm world_comm;
  MPI_Comm new_comm;
  int spare_ranks = atoi(*++argv);
  MPI_Info info = MPI_INFO_NULL;
  int num_ranks;
  int rank;
  int error;
  int my_group = 0;
  int my_timestamp = 0;
  int my_depth = 0;
  int recovered = 0;

  // to create subset data
  Fenix_Data_subset subset_specifier;
  int num_blocks = 10;
  int start_offset = 0; 
  int end_offset= 2;
  int stride = 5;

  MPI_Init(&argc, &argv);
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv,
             spare_ranks, 0, info, &error);

  MPI_Comm_size(new_comm, &num_ranks);
  MPI_Comm_rank(new_comm, &rank);

  Fenix_Data_group_create(my_group, new_comm, my_timestamp, my_depth);

  if (fenix_role == FENIX_ROLE_INITIAL_RANK) {

    // Creating subset with fixed stride
    Fenix_Data_subset_create(num_blocks, start_offset, end_offset, stride, &subset_specifier);

    // init my subset data 
    int index;
    for (index = 0; index < kCount; index++) {
        subset[index] = -1;   
    }

    Fenix_Data_member_create(my_group, 777, subset, kCount, MPI_INT);
    Fenix_Data_member_store(my_group, 777, subset_specifier);
    //Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_commit(my_group, &my_timestamp);
  } else {
    // Creating subset with fixed stride
    Fenix_Data_subset_create(num_blocks, start_offset, end_offset, stride, &subset_specifier);

    Fenix_Data_member_restore(my_group, 777, subset, kCount, 1);
    recovered = 1;
    
    if (rank == kKillID) {
        int index;
        for (index = 0; index < kCount; index++) {
            printf("RECOVERED: subset[%d]: %d\n", index, subset[index]);  
        }
    }
  }

#if 1
  if (rank == kKillID && recovered == 0) {
    pid_t pid = getpid();
    kill(pid, SIGKILL);
  }
#endif

  int index;
  int subset_index;
  for (index = 0; index < max_iter; index++) {
      for (subset_index = 0; subset_index < kCount; subset_index++) {
          subset[subset_index] = subset_index + 1; 
      }
      Fenix_Data_member_store(my_group, 777, subset_specifier);
  } 

 // Fenix_Data_member_store(my_group, 777, subset_specifier);
  Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
  Fenix_Data_commit(my_group, &my_timestamp);

#if 0
  if (rank == kKillID) {
     for (index = 0; index < kCount; index++) {
         printf("subset[%d]: %d; rank: %d; role: %d\n", index, subset[index], rank, fenix_role); 
      }
      printf("\n");
  } else if (rank == 0) {
   //  for (index = 0; index < kCount; index++) {
         //printf("subset[%d]: %d; rank: %d; role: %d\n", index, subset[index], rank, fenix_role); 
   //   }
      //printf("\n");
  }
#endif
 
  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
