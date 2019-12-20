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

const int kCount = 300;
const int kTag = 1;
//const int kKillID = 2;
int kNumIterations = 2;

void my_recover_callback(MPI_Comm new_comm, int error, void *callback_data) {
  int rank;
  double *y = (double *) callback_data;
}

int main(int argc, char **argv) {

  if( argc < 3 ) {
     printf("Usage: fenix_ring <number of spare process> <mpi rank being killed>\n");
     exit(0);
  }
  void (*recPtr)(MPI_Comm, int, void *);
  recPtr = &my_recover_callback;
  double x[4] = {0, 0, 0, 0};
  int i;
  int inmsg[300]  ;
  int outmsg[300] ;
  int recovered = 0;
  int reset = 0;
  MPI_Status status;

  int fenix_role;
  MPI_Comm world_comm;
  MPI_Comm new_comm;
  int spare_ranks = atoi(argv[1]);
  int kKillID = atoi(argv[2]);
  MPI_Info info = MPI_INFO_NULL;
  int num_ranks;
  int rank;
  int error;
  int my_group = 0;
  int my_timestamp = 0;
  int my_depth = 1;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if( rank == 0 ) {
     printf("Executing the program with %d spare ranks. Rank %d will be killed.\n",spare_ranks,kKillID);
  }
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv,
             spare_ranks, 0, info, &error);

  if(error){
    fprintf(stderr, "FAILURE, Fenix_Init error %d\n", error);
    return 1;
  }

  MPI_Comm_size(new_comm, &num_ranks);
  MPI_Comm_rank(new_comm, &rank);

  /* This is called even for recovered/survived ranks */
  /* If called by SURVIVED ranks, make sure the group has been exist */
  /* Recovered rank needs to initalize the data                      */
  Fenix_Data_group_create( my_group, new_comm, my_timestamp, my_depth, FENIX_DATA_POLICY_IN_MEMORY_RAID,
        (int[]){1, num_ranks/2}, &error);

  if (fenix_role == FENIX_ROLE_INITIAL_RANK) {

    for (i = 0; i < kCount; i++) {
        inmsg[i] = -1;  
    }

    for( i = 4; i < kCount; i++ ) {
      outmsg[i] = i+rank *2;
    }
    for( i = 0; i < 4; i++ ) {
      x[i] = (double)(i+1)/(double)3.0;
    }
    outmsg[0] = rank + 1;
    outmsg[1] = rank + 2;
    outmsg[2] = rank + 3;
    outmsg[3] = -1000;

    Fenix_Data_member_create(my_group, 777, outmsg, kCount, MPI_INT);
    Fenix_Data_member_create(my_group, 778, x, 4, MPI_DOUBLE);
    Fenix_Data_member_create(my_group, 779, inmsg, kCount, MPI_INT);

    Fenix_Data_member_store(my_group, 779, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_member_store(my_group, 778, FENIX_DATA_SUBSET_FULL);

    Fenix_Data_commit(my_group, &my_timestamp);
    recovered = 0;
    reset = 0;
    

  } else {
    int out_flag = 0;
    
    Fenix_Data_member_restore(my_group, 777, outmsg, kCount, 2, NULL);
    Fenix_Data_member_restore(my_group, 778, x, 4, 1, NULL);
    Fenix_Data_member_restore(my_group, 779, inmsg, kCount, 1, NULL);
    fprintf(stderr, "Did restore on node %d\n", rank);
    
    Fenix_Data_member_attr_set(my_group, 777, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
        outmsg, &out_flag);
    Fenix_Data_member_attr_set(my_group, 778, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
        x, &out_flag);
    Fenix_Data_member_attr_set(my_group, 779, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
        inmsg, &out_flag);

    printf("inmsg = %d\n",inmsg[0]);
    printf("outmsg = %d\n",outmsg[0]);
    recovered = 1;
    reset = 1;
  }

  for (i = 0; i < kCount; i++) {
      inmsg[i] = -1;  
  }

  for( i = 4; i < kCount; i++ ) {
    outmsg[i] = 0;
  }
  for( i = 0; i < 4; i++ ) {
      x[i] = (double)(i+1)/(double)10.0;
  }
 
  Fenix_Data_member_store(my_group, 779, FENIX_DATA_SUBSET_FULL);
  Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
  Fenix_Data_member_store(my_group, 778, FENIX_DATA_SUBSET_FULL);
  Fenix_Data_commit(my_group, &my_timestamp);

  if (rank == kKillID && recovered == 0) {
    fprintf(stderr, "Doing kill, node:%d\n", rank);
    pid_t pid = getpid();
    kill(pid, SIGTERM);
  }

  for (i = 0; i < kNumIterations; i++) {
     
  
    if (rank == 0) {
      MPI_Send(outmsg, kCount, MPI_INT, 1, kTag, new_comm); // send to rank # 1
      MPI_Recv(inmsg, kCount, MPI_INT, (num_ranks - 1), kTag, new_comm,
               &status); // recv from last rank #
    }
    else {
      MPI_Recv(inmsg, kCount, MPI_INT, (rank - 1), kTag, new_comm,
               &status); // recv from prev rank #
      outmsg[0] = inmsg[0] + 1;
      outmsg[1] = inmsg[1] + 1;
      outmsg[2] = inmsg[2] + 1;
      outmsg[3] = inmsg[3] ;
      MPI_Send(outmsg, kCount, MPI_INT, ((rank + 1) % num_ranks), kTag,
               new_comm); // send to next rank #
    }
  }

  int checksum[300];
  MPI_Allreduce(inmsg, checksum, kCount, MPI_INT, MPI_SUM, new_comm);
  MPI_Barrier(new_comm);

  Fenix_Data_member_store(my_group, 777,FENIX_DATA_SUBSET_FULL);
  Fenix_Data_member_store(my_group, 778,FENIX_DATA_SUBSET_FULL);
  Fenix_Data_member_store(my_group, 779,FENIX_DATA_SUBSET_FULL);
  Fenix_Data_commit(my_group, &my_timestamp);

  int sum;
  if (rank == 0 ) {
    sum = (num_ranks * (num_ranks + 1)) / 2;
    if( sum == checksum[0] ) {
       printf("SUCCESS: num_ranks: %d; sum: %d; checksum: %d\n", num_ranks, sum, checksum[0]);
    } else {
       printf("FAILURE: num_ranks: %d; sum: %d; checksum: %d\n", num_ranks, sum, checksum[0]);
    }
    printf("End of the program %d\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
