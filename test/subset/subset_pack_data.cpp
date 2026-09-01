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

#include <fenix.h>
using fenix::data::util::DataBuffer;
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "fenix/data/subset.hpp"
#include "fenix/data/snapshot.hpp"
#include "fenix/data/util/data_ref.hpp"
#include <fenix/data/util/serializer.hpp>
using fenix::data::util::DataBuffer;

#include "subset_common.hpp"

using namespace fenix;
using namespace fenix::data;

bool test_pack_data(const DataSubset& a) {
  size_t count = a.max_count();
  if (count == 0) count = 1000;

  std::vector<int> in, out;
  in.resize(count);
  out.resize(count);

  for (int& i : in) i = 1;
  for (int& i : out) i = 0;

  DataBuffer packed_buf;
  //fenix::data::util::DataRef in_ref((char*)in.data(), count * sizeof(int));
  //fenix::data::util::DataRef out_ref((char*)out.data(), count * sizeof(int));
  std::optional<SerializeFunc> no_func;

  // Create snapshots for testing
  DataSnapshot in_snap(sizeof(int), count);
  DataSnapshot out_snap(sizeof(int), count);

  // Data to in_buf via snapshot
  a.copy_data(in_snap.create_serializer(in, no_func, a));

  // Pack in_snap.buf() into packed_buf
  a.pack_data(sizeof(int), in_snap.buf(), packed_buf);

  // Unpack back to out_snap.buf()
  out_snap.buf().reset(sizeof(int) * count);
  a.unpack_data(sizeof(int), packed_buf, out_snap.buf());

  // Data from out_snap to out
  a.copy_data(out_snap.create_deserializer(out, no_func, a));

  for (int i = 0; i < count; i++) {
    if (a.includes(i) && out[i] != 1) {
      printf("Failed to transfer index %d\n", i);
      return false;
    } else if (!a.includes(i) && out[i] != 0) {
      printf("Incorrectly transferred index %d\n", i);
      return false;
    }
  }

  return true;
}

int main(int argc, char** argv) {
  bool success = true;

  auto subsets = get_expanded_subsets();
  for (const auto& a : subsets) {
    success &= test_pack_data(a);
  }

  return success ? 0 : 1;
}
