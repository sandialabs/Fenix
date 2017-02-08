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
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

float *create_rand_nums(int num_elements) {
  float *rand_nums = (float *) malloc(sizeof(float) * num_elements);
  int i;
  for (i = 0; i < num_elements; i ++) {
    rand_nums[i] = (rand() / (float) RAND_MAX);
  }
  return rand_nums;
}

int main(int argc, char **argv) {

  // Standard
  int world_rank;
  int world_size;

  // Fenix
  int fenix_role;
  MPI_Comm world_comm = NULL;
  MPI_Comm new_comm = NULL;
  int spawn_mode = 0;
  int error;
  float local_sum = 0;
  int i;
  float *rand_nums = NULL;
  int kill_rank;
  int spare_ranks;
  int num_elements_per_proc;
  int recovered;

  if (argc != 4) {
    fprintf(stderr, "Usage: <# elements> <# spare ranks> <rank ID for killing>\n");
    exit(0);
  }
  kill_rank = atoi(argv[3]);
  spare_ranks = atoi(argv[2]);
  num_elements_per_proc = atoi(argv[1]);


  MPI_Init(&argc, &argv);
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv, 
              spare_ranks, spawn_mode, MPI_INFO_NULL, &error );

  MPI_Comm_rank(new_comm, &world_rank);
  MPI_Comm_size(new_comm, &world_size);

  if (fenix_role == FENIX_ROLE_INITIAL_RANK) {
    recovered = 0;
  } else {
    recovered = 1;
    if( rand_nums != NULL ) {
       free( rand_nums );
    }
  }

  srand(time(NULL) * world_rank);
  rand_nums = create_rand_nums(num_elements_per_proc);

  for (i = 0; i < num_elements_per_proc; i ++) {
    local_sum += rand_nums[i];
  }
  float global_sum;
  MPI_Allreduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, new_comm);
  float mean = global_sum / (num_elements_per_proc * world_size);

  float local_sq_diff = 0;
  for (i = 0; i < num_elements_per_proc; i ++) {
    local_sq_diff += (rand_nums[i] - mean) * (rand_nums[i] - mean);
  }



  if (world_rank == kill_rank && recovered == 0) {
    pid_t pid = getpid();
    kill(pid, SIGKILL);
  }



  float global_sq_diff;
  MPI_Reduce(&local_sq_diff, &global_sq_diff, 1, MPI_FLOAT, MPI_SUM, 0, new_comm);

  free(rand_nums);
  rand_nums = NULL;
  Fenix_Finalize();

  if (world_rank == 0) {
    float stddev = sqrt(global_sq_diff / (num_elements_per_proc * world_size));
    printf("Mean - %f, Standard deviation = %f\n", mean, stddev);
  }
  MPI_Finalize();

  return 0;
}
