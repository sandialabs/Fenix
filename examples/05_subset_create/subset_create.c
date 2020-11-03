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

#include <fenix.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

int max_iter = 2;
const int kCount = 100;
const int kKillID = 2;

int main(int argc, char **argv) {
fprintf(stderr, "Started\n");
  int i;
  int subset[500];
  MPI_Status status;

  if (argc < 2) {
      printf("Usage: %s <# spare ranks> \n", *argv);
      exit(0);
  }

  int fenix_role;
  MPI_Comm world_comm;
  MPI_Comm new_comm;
  int spare_ranks = atoi(argv[1]);
  MPI_Info info = MPI_INFO_NULL;
  int num_ranks;
  int rank;
  int error;
  int my_group = 0;
  int my_timestamp = 0;
  int my_depth = 1;
  int recovered = 0;

  // to create subset data
  Fenix_Data_subset subset_specifier;
  int num_blocks = 10;
  int start_offset = 0; 
  int end_offset= 2;
  int stride = 5;
  // Creating subset with fixed stride
  // This doesn't rely on any state information on the Fenix system, so can be called
  // prior to Fenix_Init.
  Fenix_Data_subset_create(num_blocks, start_offset, end_offset, stride, &subset_specifier);

  MPI_Init(&argc, &argv);
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv,
             spare_ranks, 0, info, &error);

  MPI_Comm_size(new_comm, &num_ranks);
  MPI_Comm_rank(new_comm, &rank);
  
  if(error){
    fprintf(stderr, "FAILURE on Fenix Init (%d). Exiting.\n", error);
    exit(1);
  }

  Fenix_Data_group_create(my_group, new_comm, my_timestamp, my_depth, FENIX_DATA_POLICY_IN_MEMORY_RAID,
          (int[]){1, num_ranks/2}, &error);

  if (fenix_role == FENIX_ROLE_INITIAL_RANK) {
    // init my subset data 
    int index;
    for (index = 0; index < kCount; index++) {
        subset[index] = -1;
    }

    Fenix_Data_member_create(my_group, 777, subset, kCount, MPI_INT);
    
    //Store the entire data set for the initial commit. This is not a requirement.
    Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_commit_barrier(my_group, NULL);

  } else {
    //We've had a failure! Time to recover data.
    fprintf(stderr, "Starting data recovery on node %d\n", rank);
    Fenix_Data_member_restore(my_group, 777, subset, kCount, FENIX_TIME_STAMP_MAX, NULL);

    int out_flag;
    Fenix_Data_member_attr_set(my_group, 777, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
        subset, &out_flag);

    
    recovered = 1;
  }
  
  int index;
  int subset_index;
 
  //Very simplistic method for determining if the work is already done.
  if(recovered == 0){
      for (index = 0; index < max_iter; index++) {
          for (subset_index = 0; subset_index < kCount; subset_index++) {
              subset[subset_index] = subset_index + 1; 
          }
      } 
  
      //Now that we've finished some work, we'll back it up.
      //We'll store only the small subset that we specified, though.
      //This means that as far as Fenix is concerned only data within that
      //subset was ever changed from the initialized value of -1
      Fenix_Data_member_store(my_group, 777, subset_specifier);
      Fenix_Data_commit_barrier(my_group, NULL);

      MPI_Barrier(new_comm); //Make sure everyone is done committing before we kill and restart everyone
                             //else we may end up with only some nodes having the commit, and it being unusable

  }

  
  //Kill a rank to test that we can recover from the commits we've made.
  if (rank == kKillID && recovered == 0) {
    fprintf(stderr, "Doing kill on node %d\n", rank);
    pid_t pid = getpid();
    kill(pid, SIGTERM);
  }

  //Make sure we've let rank 2 fail before proceeding, so we're definitely checking 
  //recovery after actually recovering.
  MPI_Barrier(new_comm);
  
  int successful = 1;
  for(int i = 0; i < kCount; i++){
    //If the data is in the subset, we expect it to equal index+1
    //else it should equal -1.
    
    //Inefficient way of checking if data is within a subset, but it'll do for this small example.
    int in_subset = 0;
    for(int block = 0; block < num_blocks; block++){
        if(i >= (start_offset + stride * block) && i <= (end_offset + stride * block)){
            in_subset = 1;
            break;
        }
    }

    if(in_subset && subset[i] != i + 1){
        fprintf(stderr, "Rank %d recovery error at index %d within subset. Found: %d\n", rank, i, subset[i]);
        successful = 0;
    } else if(!in_subset && subset[i] != -1){
        fprintf(stderr, "Rank %d recovery error at index %d outside subset. Found: %d\n", rank, i, subset[i]);
        successful = 0;
    }
  }

  
  if(successful){
    printf("Rank %d successfully recovered\n", rank);
  } else {
    printf("FAILURE on rank %d\n", rank);
  }


  Fenix_Data_subset_delete(&subset_specifier);  


  Fenix_Finalize();
  MPI_Finalize();
  return !successful; //return error status
}
