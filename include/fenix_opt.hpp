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

#ifndef __FENIX_OPT__
#define __FENIX_OPT__

#include <cstdio>
#include <cstdlib>

// FENIX_ABORT kills whole MPI job if MPI visible in current file, else just
// aborts this process
// Prefer fenix_assert or fatal_print instead of using this directly
#ifdef MPI_VERSION
#define FENIX_ABORT()                                                          \
  do {                                                                         \
    int mpi_is_init_;                                                          \
    MPI_Initialized(&mpi_is_init_);                                            \
    if (mpi_is_init_) MPI_Abort(MPI_COMM_WORLD, 1);                            \
    abort();                                                                   \
  } while (0)
#else
#define FENIX_ABORT() abort()
#endif

#define traced_print_impl(file, fmt, ...)                                      \
  fprintf(                                                                     \
    file, "%s:%d %s(): " fmt "\n", __FILE__, __LINE__,                         \
    __func__ __VA_OPT__(, ) __VA_ARGS__                                        \
  )

#ifdef MPI_VERSION
#define traced_print(f, fmt, ...)                                              \
  do {                                                                         \
    int mpi_is_init_;                                                          \
    MPI_Initialized(&mpi_is_init_);                                            \
    if (mpi_is_init_) {                                                        \
      int rank_;                                                               \
      MPI_Comm_rank(MPI_COMM_WORLD, &rank_);                                   \
      traced_print_impl(f, "rank %d: " fmt, rank_ __VA_OPT__(, ) __VA_ARGS__); \
    } else {                                                                   \
      traced_print_impl(f, fmt __VA_OPT__(, ) __VA_ARGS__);                    \
    }                                                                          \
  } while (0)
#else
#define traced_print(...) traced_print_impl(__VA_ARGS__)
#endif

#define error_print(...) traced_print(stderr, __VA_ARGS__)
#define debug_print(...) traced_print(stderr, __VA_ARGS__)
#define verbose_print(...) traced_print(stdout, __VA_ARGS__)

//Multi-line macro functions wrapped in do-while to maintain correct behavior
//regardless of what surrounding code is
#define fatal_print(...)                                                       \
  do {                                                                         \
    __VA_OPT__(error_print(__VA_ARGS__);)                                      \
    error_print("Fenix aborting due to fatal error!");                         \
    FENIX_ABORT();                                                             \
  } while (0)

// Checks even during release builds. Generally used in our ci tests or examples
#define fenix_require(predicate, ...)                                          \
  do {                                                                         \
    if (!(predicate)) {                                                        \
      error_print("internal error, failed assertion (%s)", #predicate);        \
      __VA_OPT__(error_print(__VA_ARGS__);)                                    \
      fatal_print();                                                           \
    }                                                                          \
  } while (0)

#ifdef NDEBUG
//Disable normal assertions when NDEBUG
#define fenix_assert(...)                                                      \
  do {                                                                         \
  } while (0)
#else
#define fenix_assert(...) fenix_require(__VA_ARGS__)
#endif

typedef struct __fenix_debug_opt_t {
  int verbose = -1;
} fenix_debug_opt_t;

void __fenix_init_opt(int argc, char** argv);

#endif
