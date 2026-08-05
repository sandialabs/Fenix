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
#ifndef __FENIX_DATA_MEMBER_H__
#define __FENIX_DATA_MEMBER_H__

#include <optional>

#include "fenix_data_subset.hpp"
#include "fenix_data_buffer.hpp"
#include "fenix/data/util/data_ref.hpp"
#include "fenix/data/util/serializer.hpp"

namespace fenix::data {

struct fenix_group_t;

struct fenix_member_entry_packet_t {
  int memberid;
  int datatype_size;
  int current_count;
};

class fenix_member_entry_t {
 public:
  using Serializer = util::Serializer;

  fenix_member_entry_t() = default;

  fenix_member_entry_t(int id, void* data, int count, MPI_Datatype datatype);
  fenix_member_entry_t(int id, void* data, int count, int datatype_size);

  fenix_member_entry_t(
    int id, void* data, int count, MPI_Datatype datatype, SerializeFunc& s
  );
  fenix_member_entry_t(
    int id, void* data, int count, int datatype_size, SerializeFunc& s
  );

  fenix_member_entry_t(
    int id, void* data, int count, MPI_Datatype datatype,
    std::optional<SerializeFunc> s
  );
  fenix_member_entry_t(
    int id, void* data, int count, int datatype_size,
    std::optional<SerializeFunc> s
  );

  fenix_member_entry_packet_t to_packet();

  int memberid = -1;
  int datatype_size;
  DataRef user_data;

  int elm_count();

  void stage_begin(FILE** fp, DataBuffer& buf);
  void stage_begin(std::iostream** fp, DataBuffer& buf);

  void stage_end();

  std::optional<SerializeFunc> ser_func;

  // Set iff stage_begin called with no matching stage_end
  std::optional<Serializer> open_serializer;

  // Serialize user_data into buf
  void serialize(const DataSubset& subset, DataBuffer& buf);

  // Deserialize buf into dst
  void deserialize(
    const DataSubset& subset, DataBuffer& buf, const DataRef& dst
  );

 private:
  // Note that Serializers aren't guaranteed to have written their data to the
  // buffer until their destructor is called. So these should usually only be
  // used to construct temporaries that go to a subset's serialize call
  Serializer create_serializer(
    std::optional<SerializeFunc>& sf, const DataSubset& s, DataBuffer& b
  );
  Serializer create_deserializer(
    const DataSubset& subset, DataBuffer& buf, const DataRef& dst
  );
};

} // namespace fenix::data
#endif // FENIX_DATA_MEMBER_H
