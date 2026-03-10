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
// Questions? Contact Matthew Whitlock (mwhitlo@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef __FENIX_MPI_UTIL__
#define __FENIX_MPI_UTIL__

#include <mpi.h>
#include <tuple>

namespace fenix::util {

inline int comm_size(MPI_Comm c) {
  int ret;
  MPI_Comm_size(c, &ret);
  return ret;
}

inline int comm_rank(MPI_Comm c) {
  int ret;
  MPI_Comm_rank(c, &ret);
  return ret;
}

static inline int type_size(MPI_Datatype d) {
  if (d == MPI_DATATYPE_NULL) return 0;
  int size;
  MPI_Type_size(d, &size);
  return size;
}

static inline bool mpi_finalized() {
  int flag;
  MPI_Finalized(&flag);
  return flag;
}

class Status {
 public:
  int return_value;
  MPI_Status status;

  Status() = default;
  Status(int r) : return_value(r) {}
  auto operator=(int r) {
    return_value = r;
    return *this;
  }

  operator bool() const { return return_value == MPI_SUCCESS; }

  operator int() const { return return_value; }
  bool operator==(int r) const { return return_value == r; }

  operator MPI_Status() const { return status; }
  operator MPI_Status*() { return &status; }

  // to support structured unbinding
  template <size_t I>
  auto&& get() && {
    if constexpr (I == 0) return std::move(return_value);
    if constexpr (I == 1) return std::move(status);
  }
};

} // namespace fenix::util

// Supporting structured unbinding for Status
namespace std {
template <>
struct tuple_size<fenix::util::Status> : std::integral_constant<size_t, 2> {};

template <>
struct tuple_element<0, fenix::util::Status> {
  using type = int;
};
template <>
struct tuple_element<1, fenix::util::Status> {
  using type = MPI_Status;
};
}

#endif
