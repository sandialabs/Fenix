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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <getopt.h>

// FENIX_ABORT kills whole MPI job if MPI visible in current file, else just
// aborts this process
// Prefer fenix_assert or fatal_print instead of using this directly
#ifdef MPI_VERSION
    #define FENIX_ABORT() \
        do { \
            int mpi_is_init;                                                   \
            MPI_Initialized(&mpi_is_init);                                     \
            if(mpi_is_init) MPI_Abort(MPI_COMM_WORLD, 1);                      \
            abort();                                                           \
        } while(0)
#else
    #define FENIX_ABORT() abort()
#endif

// Helpers needing to support printing w/o any user-supplied format args
// Supports up to 10 args
// Functions should be named base_name_s for 1 args or base_name_a otherwise
#define FN_SUFF_I(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define FN_SUFF(...) FN_SUFF_I(__VA_ARGS__,_a,_a,_a,_a,_a,_a,_a,_a,_a,_s)
#define FN_SUFF_MERGE_IMPL(fn, suff) fn ## suff
#define FN_SUFF_MERGE(fn, suff) FN_SUFF_MERGE_IMPL(fn, suff)
#define FN_NAME(base_name, ...) FN_SUFF_MERGE(base_name, FN_SUFF(__VA_ARGS__))

#define TRACE_PRINT_FMT "%s:%d %s(): "
#define TRACE_PRINT_ARG __FILE__, __LINE__, __func__

#define traced_print_s(file, fmt) \
    fprintf(file, TRACE_PRINT_FMT fmt "\n", TRACE_PRINT_ARG)
#define traced_print_a(file, fmt, ...) \
    fprintf(file, TRACE_PRINT_FMT fmt "\n", TRACE_PRINT_ARG, __VA_ARGS__)
#define traced_print(file, ...) FN_NAME(traced_print, __VA_ARGS__)(file, __VA_ARGS__)

#define debug_print(...) traced_print(stderr, __VA_ARGS__)
#define verbose_print(...) traced_print(stdout, __VA_ARGS__)

//Multi-line macro functions wrapped in do-while to maintain correct behavior
//regardless of what surrouding code is
#define fatal_print(...) \
    do {                                                                       \
        traced_print(stderr, __VA_ARGS__);                                     \
        traced_print(stderr, "Fenix aborting due to fatal error!");            \
        FENIX_ABORT();                                                         \
    } while(0)

#define fenix_assert_a(predicate, ...) \
    do{if( !(predicate) ){ fatal_print(__VA_ARGS__); }} while(0)
#define fenix_assert_s(predicate) \
    fenix_assert_a(predicate, "internal error, failed assertion (" #predicate ")" );

#ifdef NDEBUG
    //Disable assertions when NDEBUG
    #define fenix_assert(...) do { } while(0)
#else
    #define fenix_assert(...) FN_NAME(fenix_assert, __VA_ARGS__)(__VA_ARGS__)
#endif

typedef struct __fenix_debug_opt_t {
    int verbose = -1;
} fenix_debug_opt_t;

void __fenix_init_opt(int argc, char **argv);

#endif
