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
// "AS IS" AND ANY // EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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
const int kCount = 20;
const int kKillID = 2;

int main(int argc, char **argv) {

  int i;
  int subset[1000];
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
  int num_blocks = 2;
  int *start_offsets = (int *) malloc(sizeof(int) * num_blocks); 
  int *end_offsets = (int *) malloc(sizeof(int) * num_blocks); 
  start_offsets[0] = 0;
    end_offsets[0] = 1;
  start_offsets[1] = 5;
    end_offsets[1] = 10;
  Fenix_Data_subset_createv(num_blocks, start_offsets, end_offsets, &subset_specifier);

  MPI_Init(&argc, &argv);
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv,
             spare_ranks, 0, info, &error);

  MPI_Comm_size(new_comm, &num_ranks);
  MPI_Comm_rank(new_comm, &rank);

  if(error){
    fprintf(stderr, "FAILURE on Fenix Init. This code is not structured to handle this. Exiting.\n");
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
    Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_commit(my_group, NULL);
  } else {
    fprintf(stderr, "Doing restore on rank %d\n", rank);
    Fenix_Data_member_restore(my_group, 777, subset, kCount, 1, NULL);
    fprintf(stderr, "Finished restore on rank %d\n", rank);
    recovered = 1;

    int out_flag;
    Fenix_Data_member_attr_set(my_group, 777, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
            subset, &out_flag);
    
  }

  if(recovered == 0){
      int index;
      int subset_index;
      for (index = 0; index < max_iter; index++) {
          for (subset_index = 0; subset_index < kCount; subset_index++) {
              subset[subset_index] = subset_index + 1; 
          }
      } 

      Fenix_Data_member_store(my_group, 777, subset_specifier);
      Fenix_Data_commit(my_group, NULL);
      
      MPI_Barrier(new_comm); //Make sure everyone is done committing before we kill and restart everyone
                             //else we may end up with only some nodes having the commit, and it being unusable

      if (rank == kKillID) {
        fprintf(stderr, "Doing kill on node %d\n", rank); 
        pid_t pid = getpid();
        kill(pid, SIGTERM);
      }
  }
  
  //make sure the kill and restart has happened before we test the recovery
  MPI_Barrier(new_comm);


  //Check for proper recovery.
  int successful = 1;
  for(int index = 0; index < kCount; index++){
    int in_subset = 0;
    for(int block = 0; block < num_blocks; block++){
        if(index >= start_offsets[block] && index <= end_offsets[block]){
            in_subset = 1;
            break;
        }
    }

    if(in_subset && subset[index] != index+1){
        fprintf(stderr, "Rank %d recovery error at index %d within subset. Found: %d\n", rank, index, subset[index]);
        successful = 0;
    } else if(!in_subset && subset[index] != -1){
        fprintf(stderr, "Rank %d recovery error at index %d outside subset. Found: %d\n", rank, index, subset[index]);
        successful = 0;
    }
  }

  if(successful){
    printf("Rank %d successfully recovered\n", rank);
  } else {
      printf("FAILURE on rank %d\n", rank);
  }

  Fenix_Data_subset_delete(&subset_specifier);
  free(start_offsets);
  free(end_offsets);

  Fenix_Finalize();
  MPI_Finalize();
  return !successful; //return error status
}
