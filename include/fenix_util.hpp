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

#ifndef __FENIX_UTIL__
#define __FENIX_UTIL__

#include <mpi.h>
#include "fenix.hpp"
#include "fenix_opt.hpp"

extern char* logname;

void __fenix_ranks_agree(int*, int*, int*, MPI_Datatype*);

int __fenix_binary_search(int*, int, int);

int __fenix_comparator(const void*, const void*);

int __fenix_get_size(MPI_Datatype);

int __fenix_get_current_rank(MPI_Comm);

int __fenix_get_world_size(MPI_Comm);

void* s_calloc(int count, size_t size);

void* s_malloc(size_t size);

void* s_realloc(void* mem, size_t size);

namespace fenix::util {

int resume_application(bool new_exception = false);

// ScopedOptions hold the old option and revert to it in their destructors, so
// changes revert even when exceptions are thrown.
struct ScopedOption {
  ScopedOption(SettingName m_setting, int new_option) : setting(m_setting) {
    set_option(setting, new_option);
  }
  ~ScopedOption() {
    if (fenix::initialized()) set_option(setting, old);
  }

  // No moving or copying scoped options, things would get complicated.
  ScopedOption(const ScopedOption&) = delete;
  ScopedOption(ScopedOption&&)      = delete;

  const SettingName setting;
  const int old = get_option(setting);
};

// Default error handling for inside the Fenix runtime.
struct ScopedDefaultRuntimeOptions {
  ScopedOption resume{RESUME_MODE, THROW}, unhandled{UNHANDLED_MODE, ABORT};
};

// Helper for MPI_ERRORS_RETURN-like error handling
struct ScopedIgnoreAndReturn {
  ScopedOption recovery{RECOVERY_MODE, IGNORE}, resume{RESUME_MODE, RETURN};
};

struct ScopedActiveMlog {
  ScopedActiveMlog(int id) : old_mlog(mlog::active()) { mlog::activate(id); }
  ~ScopedActiveMlog() {
    if (fenix::initialized()) mlog::activate(old_mlog);
  }
  const int old_mlog;
  const bool old_inline_recovery = old_mlog != FENIX_MLOG_NONE &&
    get_option(MLOG_RECOVERY_MODE) != MANUAL &&
    get_option(RECOVERY_MODE) != IGNORE;
};

} // namespace fenix::util

// clang-format off
#define RUNTIME_EXCEPTION_HANDLER                                              \
  } catch (const fenix::RuntimeException& e) {                                 \
    debug_print("%s\n", e.what());                                             \
    return e.error;

#define COMM_EXCEPTION_HANDLER                                                 \
  } catch (const fenix::CommException& e) {                                    \
    return fenix::util::resume_application();                                  \
  }

#define FENIX_CPP_API_BEGIN                                                    \
  try {                                                                        \
    fenix::util::ScopedDefaultRuntimeOptions _scoped_dro;                      \
    if (!fenix::initialized()) FENIX_THROW(FENIX_ERROR_UNINITIALIZED);

// clang-format on

#define FENIX_C_API_BEGIN FENIX_CPP_API_BEGIN

#ifdef FENIX_C_CATCH_RUNTIME_EXCEPTIONS
#define FENIX_C_API_END RUNTIME_EXCEPTION_HANDLER COMM_EXCEPTION_HANDLER
#else
#define FENIX_C_API_END COMM_EXCEPTION_HANDLER
#endif

#ifdef FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS
#define FENIX_CPP_API_END RUNTIME_EXCEPTION_HANDLER COMM_EXCEPTION_HANDLER
#else
#define FENIX_CPP_API_END COMM_EXCEPTION_HANDLER
#endif

// Local functions are ones that can be called before Fenix is initialized
#define FENIX_LOCAL_CPP_API_BEGIN try {
#define FENIX_LOCAL_CPP_API_END FENIX_CPP_API_END

#define FENIX_LOCAL_C_API_BEGIN FENIX_LOCAL_CPP_API_BEGIN
#define FENIX_LOCAL_C_API_END FENIX_C_API_END

#endif
