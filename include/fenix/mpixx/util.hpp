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

#ifndef FENIX_MPIXX_UTIL_HPP
#define FENIX_MPIXX_UTIL_HPP

#include <mpi.h>
#include <string>

namespace fenix::tags {

// Ensure non-conflicting tags across Fenix.
// In particular, it is important that no other operations use the
// DETECT_FAILURES_TAG, to prevent delayed failure detection.
enum Tag {
  DETECT_FAILURES_TAG = 1000,

  FENIX_TAG_MAX
};

// MPI Standard guarantees tags below 2^15 are valid
static_assert(FENIX_TAG_MAX < (1 << 15));

} // namespace fenix::tags

namespace fenix::mpixx {

inline std::string mpi_error_string(int errcode) {
  std::string ret;
  ret.resize(MPI_MAX_ERROR_STRING + 1);
  int len;
  MPI_Error_string(errcode, &ret[0], &len);
  ret.resize(len + 1);
  return ret;
}

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

// C++ type corresponding to MPI_Datatype index pairs
template <typename T>
struct Indexed {
  static_assert(std::is_trivially_copyable_v<T>);
  T value;
  int index;
};

// Internal macro, undefined before end of this file
#define MPI_TASK_TYPE(u, r, ...)                                               \
  if constexpr (std::is_same_v<u, __VA_ARGS__>) return r;

// Helpers for getting an MPI_Datatype and count from some number of a c++ type
template <typename T>
MPI_Datatype datatype(){
  using U = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;
  static_assert(std::is_trivially_copyable_v<U>);
  // clang-format off
  MPI_TASK_TYPE(U, MPI_CHAR,            char);
  MPI_TASK_TYPE(U, MPI_FLOAT,           float);
  MPI_TASK_TYPE(U, MPI_DOUBLE,          double);
  MPI_TASK_TYPE(U, MPI_SHORT,           short);
  MPI_TASK_TYPE(U, MPI_UNSIGNED_SHORT,  unsigned short);
  MPI_TASK_TYPE(U, MPI_INT,             int);
  MPI_TASK_TYPE(U, MPI_UNSIGNED,        unsigned int);
  MPI_TASK_TYPE(U, MPI_LONG,            long);
  MPI_TASK_TYPE(U, MPI_UNSIGNED_LONG,   unsigned long);
  MPI_TASK_TYPE(U, MPI_LOGICAL,         bool);
  MPI_TASK_TYPE(U, MPI_FLOAT_INT,       Indexed<float>);
  MPI_TASK_TYPE(U, MPI_DOUBLE_INT,      Indexed<double>);
  MPI_TASK_TYPE(U, MPI_LONG_INT,        Indexed<long>);
  MPI_TASK_TYPE(U, MPI_2INT,            Indexed<int>);
  MPI_TASK_TYPE(U, MPI_SHORT_INT,       Indexed<short>);
  MPI_TASK_TYPE(U, MPI_LONG_DOUBLE_INT, Indexed<long double>);
  // clang-format on

  // Technically sketch to just make this MPI_BYTE, but only when heterogenenous
  // so we'll cross that bridge when we get there. Convenient for trivial custom
  // types for now
  return MPI_BYTE;
}

#undef MPI_TASK_TYPE

template <typename T>
MPI_Datatype datatype(T&& t) {
  return datatype<T>();
}

template <typename T>
constexpr int datatype_count(T&& t, int in_count){
  if (datatype<T>() == MPI_BYTE) {
    return in_count * sizeof(std::remove_pointer_t<std::decay_t<T>>);
  }
  return in_count;
}

} // namespace fenix::mpixx

#endif // FENIX_MPIXX_UTIL_HPP
