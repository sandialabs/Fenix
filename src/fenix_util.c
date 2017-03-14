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

#include "fenix_constants.h"
#include "fenix_opt.h"
#include "fenix_process_recovery.h"
#include "fenix_util.h"

/**
 * @brief
 * @param invec
 * @param inoutvec
 * @param length
 * @param datatype
 */
void __fenix_ranks_agree(int *invec, int *inoutvec, int *len, MPI_Datatype *dtype) {
  int index;
  for (index = 0; index < *len; index++) {
    inoutvec[index] = (inoutvec[index] == invec[index]) ? invec[index] : -1;
  }
}

/**
 * @brief
 * @param a
 * @param length
 * @param key
 */
int __fenix_binary_search(int *a, int length, int key) {
  int low = 0;
  int high = length - 1;
  int found = -1;
  while (found != 1 && low <= high) {
    int mid = low + (high - low) / 2;
    if (key < a[mid]) {
      high = mid - 1;
    } else if (key > a[mid]) {
      low = mid + 1;
    } else {
      found = 1;
    }
  }
  return found;
}

/**
 * @brief
 * @param p
 * @param q
 */
int __fenix_comparator(const void *p, const void *q) {
  return *(int *) p - *(int *) q;
}

/**
 * @brief
 * @param data_type
 */
int __fenix_get_size(MPI_Datatype type) {
  int size = -1;
  MPI_Type_size(type, &size);
  return size;
}

/**
 * @brief
 * @param comm
 */
int __fenix_get_current_rank(MPI_Comm comm) {
  int rank = - 1;
  PMPI_Comm_rank(comm, &rank);
  return rank;
}

/**
 * @brief
 * @param current_rank
 * @param comm
 */
int __fenix_get_partner_rank(int current_rank, MPI_Comm comm) {
  int size = - 1;
  PMPI_Comm_size(comm, &size);
  return ((current_rank + (size / 2)) % size);
}

#if 0
int get_partner_in_rank(int current_rank, MPI_Comm comm) { 
  int size = - 1;
  MPI_Comm_size(comm, &size);
  return ((current_rank + (size / 2)) % size);
}

int get_partner_out_rank(int current_rank, MPI_Comm comm) { 
  int size = - 1;
  MPI_Comm_size(comm, &size);
  return ((current_rank + (size / 2)) % size);
}
#endif


int __fenix_mpi_wait(MPI_Request *request) {
  MPI_Status status;
  int result = MPI_Wait(request, &status);
  return result;
}

int __fenix_mpi_test(MPI_Request *request) {
  MPI_Status status;
  int flag;
  MPI_Test(request, &flag, &status);
  return flag;
}

int __fenix_get_fenix_default_rank_separation( MPI_Comm comm  )
{
  int size = - 1;
  PMPI_Comm_size(comm, &size);
  return size / 2;
}
/**
 * @brief
 * @param comm
 */
int  __fenix_get_world_size(MPI_Comm comm) {
  int size = - 1;
  PMPI_Comm_size(comm, &size);
  return size;
}




/**
 * @brief
 * @param count
 * @param size
 */
void *s_calloc(int count, size_t size) {
    void *retval = calloc(count, size);
    if (!retval) {
       debug_print("Out of memory: calloc failed on alloc %lu bytes.\n", (unsigned long) size);
    }
    return retval;
}

/**
 * @brief 
 * @param size
 */
void *s_malloc(size_t size) {
    void *retval = malloc(size);
    if (!retval) {
        debug_print("Out of memory: malloc failed on alloc %lu bytes.\n", (unsigned long) size);
    }
    return retval;
}

/**
 * @brief
 * @param memory
 * @param size
 */
void *s_realloc(void *mem, size_t size) {
    void *retval = realloc(mem, size);
     if (!retval) {
       debug_print("Out of memory: malloc failed on alloc %lu bytes.\n", (unsigned long) size);
    }
    return retval;
}
