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

#ifndef FENIX_DATA_BUFFER_HPP
#define FENIX_DATA_BUFFER_HPP

#include <memory>
#include <vector>

#include <mpi.h>
#include "fenix/mpixx/tasks.hpp"

namespace fenix::data::util {

class DataBuffer {
 public:
  using MPITask = mpixx::MPITask;

  DataBuffer() = default;
  explicit DataBuffer(size_t init_size) { resize(init_size); }

  ~DataBuffer() { free_buf(); }

  DataBuffer(const DataBuffer& o) = delete;

  DataBuffer(DataBuffer&& o) { *this = std::move(o); }
  DataBuffer& operator=(DataBuffer&& o) {
    if (&o == this) return *this;
    free_buf();
    *this = o;
    o.release_buf();
    return *this;
  }

  void take_ownership(char* new_buf, size_t new_size) {
    free_buf();
    buf        = new_buf;
    user_size  = new_size;
    alloc_size = new_size;
  }

  void take_ownership_mmapped(char* new_buf, size_t new_size) {
    free_buf();
    buf         = new_buf;
    user_size   = new_size;
    alloc_size  = new_size;
    mmap_buffer = true;
  }

  // Simple resize without overallocating.
  // No ammortized growth cost, but we don't need it for our usage
  void resize(size_t new_size);
  void shrink_to_fit();
  void clear() { resize(0); }

  // Set to new size, discarding old data if reallocation is needed
  void reset(size_t new_size = 0) {
    resize(0);
    resize(new_size);
  }

  void reserve(size_t new_size) {
    size_t old_size = user_size;
    resize(new_size);
    resize(old_size);
  }

  char* data() { return buf; }
  const char* data() const { return buf; }
  size_t size() const { return user_size; }

  MPITask send(int dst, int tag, MPI_Comm comm);

  // Recv n bytes
  MPITask recv(int n, int src, int tag, MPI_Comm comm);

  // Recv an unknown amount of data and resize to fit
  MPITask recv_unknown(int src, int tag, MPI_Comm comm);

 private:
  char* buf         = nullptr;
  size_t user_size  = 0;
  size_t alloc_size = 0;
  bool mmap_buffer  = false;

  DataBuffer& operator=(const DataBuffer& o) {
    this->buf         = o.buf;
    this->user_size   = o.user_size;
    this->alloc_size  = o.alloc_size;
    this->mmap_buffer = o.mmap_buffer;
    return *this;
  }
  void free_buf();
  void release_buf() {
    buf         = nullptr;
    user_size   = 0;
    alloc_size  = 0;
    mmap_buffer = false;
  }
  void realloc_buf(size_t new_size);
};

} // namespace fenix::data::util

#endif //FENIX_DATA_BUFFER_HPP
