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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

const int kDomainSize = 20;
const double kCommonValue = 2.0;

double *init_domain(int size, int value) {
  int i;
  double *current_domain = (double *) malloc(kDomainSize * sizeof(double));
  for (i = 0; i < size; i ++) {
    current_domain[i] = value;
  }
  return current_domain;
}

int main(int argc, char **argv) {

  int num_ranks; // total ranks
  int rank; // current rank
  MPI_Comm new_comm;
  MPI_Request request[2]; /* required variable for non-blocking calls */

  int i, j;
  int fenix_status;
  int error;
  int recovered = 0;
  int my_flag;
  double *domain_one_data = init_domain(kDomainSize, kCommonValue);
  double *domain_two_data = init_domain(kDomainSize, kCommonValue);
  double c = 0.1; /* Heat Constant */
  double *tmp_data;
  MPI_Status status;

  if (argc != 2) {
    printf("usage: %s <# of time steps>\n", *argv);
    exit(0);
  }

  int num_time_steps = atoi(*++ argv);

  MPI_Init(&argc, &argv);
  MPI_Comm_dup (MPI_COMM_WORLD, &new_comm );
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
      MPI_Send(&old_domain_data[kDomainSize - 1], 1, MPI_DOUBLE, right, kTag, new_comm);
    } else if (rank == num_ranks - 1) {
      left = rank - 1;
      /* Post receive to the neighbor */
      buf[1] = kBufData;
      MPI_Irecv(&buf[0], 1, MPI_DOUBLE, left, kTag, new_comm, &request[0]);
      /* Send message to the neighbor */
      MPI_Send(&old_domain_data[0], 1, MPI_DOUBLE, left, kTag, new_comm);

    } else {
      left = rank - 1;
      right = rank + 1;
      /* Post receive to the neighbor */
      MPI_Irecv(&buf[0], 1, MPI_DOUBLE, left, kTag, new_comm, &request[0]);
      MPI_Irecv(&buf[1], 1, MPI_DOUBLE, right, kTag, new_comm, &request[1]);
      /* Send message to the neighbor */
      MPI_Send(&old_domain_data[0], 1, MPI_DOUBLE, left, kTag, new_comm);
      MPI_Send(&old_domain_data[kDomainSize - 1], 1, MPI_DOUBLE, right, kTag, new_comm);
    }

    for ( j = 1; j < kDomainSize - 1; j ++) {
      new_domain_data[j] = c * (old_domain_data[j - 1] + old_domain_data[j + 1]) +
                           (1.0 - kCommonValue * c) * old_domain_data[j];
    }

    if (rank != 0)
      MPI_Wait(&request[0], &status);
    if (rank != num_ranks - 1)
      MPI_Wait(&request[1], &status);

    /* Compute the domain using ghost points */
    new_domain_data[0] =
            c * (buf[0] + old_domain_data[1]) + (1.0 - kCommonValue * c) * old_domain_data[0];
    new_domain_data[kDomainSize - 1] = c * (old_domain_data[kDomainSize - 2] + buf[1]) +
                                            (1.0 - kCommonValue * c) *
                                            old_domain_data[kDomainSize - 1];

    tmp_data = old_domain_data;
    old_domain_data = new_domain_data;
    new_domain_data = tmp_data;
  }

  MPI_Barrier(new_comm);

  if (rank == 0) {
    printf("End of the program\n");
    for ( i = 0; i < kDomainSize; i ++) {
      printf("x[%d]: %14.13f\n", i, old_domain_data[i]);
    }
  }

  MPI_Comm_free (&new_comm);
  MPI_Finalize();
  return 0;
}
