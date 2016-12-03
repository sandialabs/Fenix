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
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

const int kDOmainSIze = 20;
const double kCommonValue = 2.0;

void my_recover_callback(int status, MPI_Comm new_comm, int error, void *callback_data) {
  int rank;
  int i;
  MPI_Comm_rank(new_comm, &rank);
  double *data = (double *) callback_data;
  if (rank == 0) {
    printf("Reinitialize data\n");
  }
  for ( i = 0; i < kDOmainSIze; i ++) {
    data[i] = kCommonValue;
  }
}

double *init_domain(int size, int value) {
  int i;
  double *current_domain = (double *) malloc(kDOmainSIze * sizeof(double));
  for ( i = 0; i < size; i ++) {
    current_domain[i] = value;
  }
  return current_domain;
}

int main(int argc, char **argv) {

  int num_ranks; /* total ranks */
  int rank; /* My local rank ID */

  MPI_Request request[2]; /* required variables for non-blocking calls */

  int fenix_status;
  MPI_Status status;
  MPI_Comm world_comm = NULL;
  MPI_Comm new_comm;
  int i,j;
  int error;
  int recovered = 0;
  int my_flag;
  double c = 0.1; /* Heat Constant */
  double *domain_one_data = init_domain(kDOmainSIze, kCommonValue);
  double *domain_two_data = init_domain(kDOmainSIze, kCommonValue);
  double *tmp_data;

  void (*recPtr)(int, MPI_Comm, int, void *);
  recPtr = &my_recover_callback;

  if (argc != 3) {
    printf("usage: %s <# of time steps>  <# spare ranks>\n", *argv);
    exit(0);
  }

  int num_time_steps = atoi(*++ argv);
  int spare_ranks = atoi(*++ argv);

  MPI_Init(&argc, &argv);
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  MPI_Comm_rank(world_comm, &rank);

  Fenix_Init(&fenix_status, world_comm, &new_comm, &argc, &argv, spare_ranks, 0, MPI_INFO_NULL,
             &error);

  if (fenix_status == FENIX_ROLE_INITIAL_RANK) {
    recovered = 0;
    Fenix_Callback_register(recPtr, (void *) domain_one_data);
  } else if (fenix_status == FENIX_ROLE_RECOVERED_RANK) {
    recovered = 1;
    Fenix_Callback_register(recPtr, (void *) domain_one_data);
  }
  else if (fenix_status == FENIX_ROLE_SURVIVOR_RANK) {
    recovered = 1;
  }

  double *old_domain_data = domain_one_data;
  double *new_domain_data = domain_two_data;

  MPI_Comm_size(new_comm, &num_ranks);
  MPI_Comm_rank(new_comm, &rank);

  for ( i = 0; i < num_time_steps; i ++) {
    const int kTag = 1;
    const double kBufData = 1.0;
    int left;
    int right;
    double buf[2];
    if (rank == 0) {
      right = rank + 1;
      buf[0] = kBufData;
      /* Post receive to the neighbor */
      MPI_Irecv(&buf[1], 1, MPI_DOUBLE, right, kTag, new_comm, &request[1]);
      /* Send message to the neighbor */
      MPI_Send(&old_domain_data[kDOmainSIze - 1], 1, MPI_DOUBLE, right, kTag, new_comm);
    } else if (rank == num_ranks - 1) {
      left = rank - 1;
      /* Post receive to the neighbor */
      buf[1] = kBufData;
      MPI_Irecv(&buf[0], 1, MPI_DOUBLE, left, kTag, new_comm, &request[0]);
      /* Send message to the neighbor */
      MPI_Send(&old_domain_data[0], 1, MPI_DOUBLE, left, kTag, new_comm);

    } else {
      const int kKillId = 2;
      const int kKillIter = 2;
      if (rank == kKillId && i == kKillIter && recovered == 0) {
        pid_t pid = getpid();
        kill(pid, SIGKILL);
      }
      left = rank - 1;
      right = rank + 1;
      /* Post receive to the neighbor */
      MPI_Irecv(&buf[0], 1, MPI_DOUBLE, left, kTag, new_comm, &request[0]);
      MPI_Irecv(&buf[1], 1, MPI_DOUBLE, right, kTag, new_comm, &request[1]);
      /* Send message to the neighbor */
      MPI_Send(&old_domain_data[0], 1, MPI_DOUBLE, left, kTag, new_comm);
      MPI_Send(&old_domain_data[kDOmainSIze - 1], 1, MPI_DOUBLE, right, kTag, new_comm);
    }

    for ( j = 1; j < kDOmainSIze - 1; j ++) {
      new_domain_data[j] = c * (old_domain_data[j - 1] + old_domain_data[j + 1]) +
                           (1.0 - kCommonValue * c) * old_domain_data[j];
    }

    if (rank != 0)
      MPI_Wait(&request[0], &status);
    if (rank != num_ranks - 1)
      MPI_Wait(&request[1], &status);

    new_domain_data[0] =
            c * (buf[0] + old_domain_data[1]) + (1.0 - kCommonValue * c) * old_domain_data[0];
    new_domain_data[kDOmainSIze - 1] = c * (old_domain_data[kDOmainSIze - 2] + buf[1]) +
                                            (1.0 - kCommonValue * c) *
                                            old_domain_data[kDOmainSIze - 1];

    tmp_data = old_domain_data;
    old_domain_data = new_domain_data;
    new_domain_data = tmp_data;
  }

  MPI_Barrier(new_comm);

  if (rank == 0) {
    printf("End of the program\n");
    for ( i = 0; i < kDOmainSIze; i ++) {
      printf("x[%d]: %14.13f\n", i, old_domain_data[i]);
    }
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}
