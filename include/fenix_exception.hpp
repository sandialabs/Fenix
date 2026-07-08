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
//        Rob Van der Wijngaart, Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#ifndef FENIX_EXCEPTION_HPP
#define FENIX_EXCEPTION_HPP

#include <mpi.h>
#include <exception>
#include <source_location>
#include <string_view>
#include <string>

namespace fenix {

struct CommException : public std::exception {
  CommException(MPI_Comm comm, int fenix_error, int mpi_error)
    : repaired_comm(comm), fenix_err(fenix_error), mpi_err(mpi_error) {};

  MPI_Comm repaired_comm;
  const int fenix_err;
  const int mpi_err;
};

struct RuntimeException : public std::exception {
  using Location = std::source_location;

  RuntimeException(Location l = Location::current())
    : RuntimeException(FENIX_ERROR_NOCATEGORY, "FENIX_ERROR_NOCATEGORY", l) {}

  RuntimeException(const char* e_str, Location l = Location::current())
    : RuntimeException(FENIX_ERROR_NOCATEGORY, e_str, l) {};

  // Helper for preprocessor macro
  RuntimeException(
    const char* e_str, const char*, Location l = Location::current()
  ) : RuntimeException(FENIX_ERROR_NOCATEGORY, e_str, l) {};

  RuntimeException(int e, const char* e_str, Location l = Location::current())
    : error(e), error_string(e_str), location(l) {};

  // file:line function error_string
  std::string to_string(int depth = 0) const noexcept {
    std::string_view f = location.file_name();
    std::string l = std::to_string(location.line());
    std::string_view fn = location.function_name();

    std::string ret;
    ret.reserve(
      3 + depth * 2 + f.size() + l.size() + fn.size() + error_string.size()
    );
    ret.assign(" ", depth * 2);
    ret += f;  ret += ":";
    ret += l;  ret += " ";
    ret += fn; ret += " ";
    ret += error_string;
    return ret;
  }

  const char* what() const noexcept override {
    m_string = to_string();
    return m_string.c_str();
  }

  const int error;
  const std::string_view error_string;
  const std::source_location location;

 private:
  mutable std::string m_string;
};

} // namespace fenix

// err can be a FENIX_ERROR_* value, or any string
#define FENIX_THROW(err) throw fenix::RuntimeException(err, #err)
#define FENIX_THROW_FROM(err, loc) throw fenix::RuntimeException(err, #err, loc)

#endif
