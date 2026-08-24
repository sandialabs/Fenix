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
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY and SANDIA CORPORATION
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RUTGERS
// UNIVERISY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <fenix.hpp>
#include <fenix_data_member.hpp>
#include <fenix_data_subset.hpp>
#include <fenix/data/util/data_ref.hpp>
#include <fenix/data/util/serializer.hpp>

#include "subset_common.hpp"

using namespace fenix;
using namespace fenix::data;
using namespace fenix::data::util;

void file_serializer(FILE* fp, int direction, void* b, int offset, int count) {
  int* buf = (int*)b;
  if (direction == FENIX_SERIALIZE) {
    int wcount = fwrite(buf + offset, sizeof(int), count, fp);
    fenix_require(wcount == count);
  } else {
    int rcount = fread(buf + offset, sizeof(int), count, fp);
    fenix_require(rcount == count);
  }
}

void stream_serializer(
  std::iostream& s, int direction, void* b, int offset, int count
) {
  int* buf = (int*)b;
  if (direction == FENIX_SERIALIZE) {
    s.write((char*)(buf + offset), sizeof(int) * count);
  } else {
    s.read((char*)(buf + offset), sizeof(int) * count);
  }
}

bool test_copy(
  const DataSubset& a, std::optional<SerializeFunc> s, const int default_inval,
  const int default_outval
) {
  size_t count = a.is_bounded() ? a.max_count() : 1000;

  std::vector<int> in, out;
  in.resize(count);
  out.resize(count);

  DataBuffer b(count);

  for (int& i : in) i = default_inval;
  for (int& i : out) i = default_outval;

  DataMember member(0, in.data(), count, sizeof(int), 0, s);

  member.serialize(a, b);
  member.deserialize(a, b, out);

  for (int i = 0; i < count; i++) {
    if (a.includes(i) && out[i] != default_inval) {
      fprintf(
        stderr,
        "Failed to transfer index %d with subset %s (%d != expected %d)\n", i,
        a.str().c_str(), out[i], default_inval
      );
      return false;
    } else if (!a.includes(i) && out[i] != default_outval) {
      fprintf(
        stderr,
        "Incorrectly transferred index %d with subset %s (%d != expected %d)\n",
        i, a.str().c_str(), out[i], default_outval
      );
      return false;
    }
  }

  return true;
}

int main(int argc, char** argv) {
  bool success = true;

  auto subsets = get_subsets();
  fprintf(stderr, "Testing default serializer\n");
  for (const auto& a : subsets) {
    success &= test_copy(a, {}, 0xAAAAAAAA, 0x55555555);
    success &= test_copy(a, {}, 0x55555555, 0xAAAAAAAA);
  }

  fprintf(stderr, "Testing file serializer\n");
  for (const auto& a : subsets) {
    success &= test_copy(a, file_serializer, 0xAAAAAAAA, 0x55555555);
    success &= test_copy(a, file_serializer, 0x55555555, 0xAAAAAAAA);
  }

  fprintf(stderr, "Testing stream serializer\n");
  for (const auto& a : subsets) {
    success &= test_copy(a, stream_serializer, 0xAAAAAAAA, 0x55555555);
    success &= test_copy(a, stream_serializer, 0x55555555, 0xAAAAAAAA);
  }

  return success ? 0 : 1;
}
