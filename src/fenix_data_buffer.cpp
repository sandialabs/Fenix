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

#include "fenix_data_buffer.hpp"
#include "fenix_opt.hpp"
#include "fenix/tasks/mpi.hpp"

#include <cstdlib>

using namespace fenix::tasks::mpi;

namespace fenix {

void DataBuffer::resize(size_t new_size) {
  if (new_size <= alloc_size) {
    user_size = new_size;
  } else {
    shrink_to_fit();

    char* new_buf = (char*)realloc(buf, new_size);
    if (new_buf == nullptr) {
      error_print(
        "unable to resize buffer to %llu bytes", (unsigned long long)new_size
      );
      free_buf();
      abort();
    } else {
      buf       = new_buf;
      user_size = alloc_size = new_size;
    }
  }
}

void DataBuffer::shrink_to_fit() {
  if (user_size == 0) {
    free_buf();
  } else if (user_size < alloc_size) {
    char* new_buf = (char*)realloc(buf, user_size);
    fenix_assert(new_buf != nullptr);
    buf        = new_buf;
    alloc_size = user_size;
  }
}

MPITask DataBuffer::send(int dst, int tag, MPI_Comm comm) {
  return tasks::mpi::send(data(), size(), MPI_BYTE, dst, tag, comm);
}

//Recv n bytes
MPITask DataBuffer::recv(int n, int src, int tag, MPI_Comm comm) {
  reset(n);
  return tasks::mpi::recv(data(), size(), MPI_BYTE, src, tag, comm);
}

//Recv an unknown amount of data and resize to fit
MPITask DataBuffer::recv_unknown(int src, int tag, MPI_Comm comm) {
  auto status = co_await tasks::mpi::probe(src, tag, comm);
  if (MPI_SUCCESS != status) co_return status;

  int n;
  MPI_Get_count(status, MPI_BYTE, &n);
  co_return co_await recv(n, src, tag, comm);
}

} //namespace fenix
