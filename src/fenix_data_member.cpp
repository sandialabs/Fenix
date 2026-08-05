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

#include "fenix_util.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"

namespace fenix::data {

fenix_member_entry_packet_t fenix_member_entry_t::to_packet() {
  fenix_member_entry_packet_t to_ret;
  to_ret.memberid      = memberid;
  to_ret.datatype_size = datatype_size;
  to_ret.current_count = elm_count();
  return to_ret;
}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* data, int count, MPI_Datatype datatype
) : fenix_member_entry_t(id, data, count, __fenix_get_size(datatype)) {}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* data, int count, MPI_Datatype datatype, SerializeFunc& s
) : fenix_member_entry_t(id, data, count, __fenix_get_size(datatype), s) {}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* d, int c, MPI_Datatype dt, std::optional<SerializeFunc> s
) : fenix_member_entry_t(id, d, c, __fenix_get_size(dt), s) {}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* data, int count, int dsize, SerializeFunc& s
) : fenix_member_entry_t(id, data, count, dsize) {
  ser_func = s;
}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* data, int count, int dsize, std::optional<SerializeFunc> s
) : fenix_member_entry_t(id, data, count, dsize) {
  ser_func = s;
}

fenix_member_entry_t::fenix_member_entry_t(
  int id, void* data, int count, int dsize
) : memberid(id), datatype_size(dsize) {
  if (count == FENIX_RESIZEABLE) user_data = DataRef((char*)data);
  else user_data = DataRef((char*)data, count * dsize);
};

int fenix_member_entry_t::elm_count() {
  if (!user_data.is_bounded()) return FENIX_RESIZEABLE;

  fenix_assert(user_data.size() % datatype_size == 0);
  return user_data.size() / datatype_size;
}

void fenix_member_entry_t::serialize(
  const DataSubset& subset, DataBuffer& buf
) {
  subset.copy_data(create_serializer(ser_func, subset, buf));
}

void fenix_member_entry_t::deserialize(
  const DataSubset& subset, DataBuffer& buf, const DataRef& dst
) {
  subset.copy_data(create_deserializer(subset, buf, dst));
}

fenix::data::util::Serializer fenix_member_entry_t::create_serializer(
  std::optional<SerializeFunc>& sf, const DataSubset& subset, DataBuffer& buf
) {
  if (open_serializer) FENIX_THROW(FENIX_ERROR_MEMBER_STAGING);

  DataRef output = user_data;
  if (subset.is_bounded()) {
    output = output.bounded(subset.max_count() * datatype_size);
  }

  if (output.is_bounded()) {
    if (buf.size() < output.size()) buf.resize(output.size());
  } else if (!sf) {
    FENIX_THROW(FENIX_ERROR_INVALID_SUBSET);
  }

  return Serializer(buf, sf, output, FENIX_SERIALIZE, datatype_size);
}

fenix::data::util::Serializer fenix_member_entry_t::create_deserializer(
  const DataSubset& subset, DataBuffer& buf, const DataRef& dst
) {
  if (open_serializer) FENIX_THROW(FENIX_ERROR_MEMBER_STAGING);
  return Serializer(buf, ser_func, dst, FENIX_DESERIALIZE, datatype_size);
}

void fenix_member_entry_t::stage_begin(FILE** fp, DataBuffer& buf) {
  std::optional<SerializeFunc> sf = SerializeFileFunc{};
  buf.resize(0);
  open_serializer.emplace(create_serializer(sf, SUBSET_FULL, buf));
  *fp = open_serializer->get_file();
}

void fenix_member_entry_t::stage_begin(std::iostream** strm, DataBuffer& buf) {
  std::optional<SerializeFunc> sf = SerializeStreamFunc{};
  buf.resize(0);
  open_serializer.emplace(create_serializer(sf, SUBSET_FULL, buf));
  *strm = open_serializer->get_stream();
}

void fenix_member_entry_t::stage_end() {
  if (!open_serializer) FENIX_THROW(FENIX_ERROR_INVALID_LOGIC_CALL);
  open_serializer.reset();
}

} //namespace fenix::data
