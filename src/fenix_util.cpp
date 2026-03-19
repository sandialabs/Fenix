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
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <exception>

#include "fenix_opt.hpp"
#include "fenix_util.hpp"
#include "fenix_exception.hpp"
#include "fenix_ext.hpp"

namespace fenix::util {

int resume_application(bool new_exception) {
  if (!initialized() || finalized()) {
    return FENIX_ERROR_CANCELLED;
  }

  switch (get_option(FENIX_RESUME_MODE)) {
  case JUMP:
    longjmp(*fenix_rt.recover_environment, 1);
  case THROW:
    if (new_exception) fenix::throw_exception();
    else throw;
  case RETURN:
    return FENIX_ERROR_CANCELLED;
  default:
    fenix_assert(false, "Unknown FENIX_RESUME_MODE");
    throw RuntimeException(
      FENIX_ERROR_INVALID_SETTING_OPTION, "Unknown FENIX_RESUME_MODE"
    );
  }
}

} // namespace fenix::util

char* logname;

void __fenix_ranks_agree(
  int* invec, int* inoutvec, int* len, MPI_Datatype* dtype
) {
  int index;
  for (index = 0; index < *len; index++) {
    inoutvec[index] = (inoutvec[index] == invec[index]) ? invec[index] : -1;
  }
}

int __fenix_binary_search(int* a, int length, int key) {
  int low   = 0;
  int high  = length - 1;
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

int __fenix_comparator(const void* p, const void* q) {
  return *(int*)p - *(int*)q;
}

int __fenix_get_size(MPI_Datatype type) {
  int size = -1;
  MPI_Type_size(type, &size);
  return size;
}

int __fenix_get_current_rank(MPI_Comm comm) {
  int rank = -1;
  PMPI_Comm_rank(comm, &rank);
  return rank;
}

int __fenix_get_world_size(MPI_Comm comm) {
  int size = -1;
  PMPI_Comm_size(comm, &size);
  return size;
}

void* s_calloc(int count, size_t size) {
  void* retval = calloc(count, size);
  if (!retval) {
    debug_print(
      "Out of memory: calloc failed on alloc %lu bytes.\n", (unsigned long)size
    );
  }
  return retval;
}

void* s_malloc(size_t size) {
  void* retval = malloc(size);
  if (!retval) {
    debug_print(
      "Out of memory: malloc failed on alloc %lu bytes.\n", (unsigned long)size
    );
  }
  return retval;
}

void* s_realloc(void* mem, size_t size) {
  void* retval = realloc(mem, size);
  if (!retval) {
    debug_print(
      "Out of memory: malloc failed on alloc %lu bytes.\n", (unsigned long)size
    );
  }
  return retval;
}
