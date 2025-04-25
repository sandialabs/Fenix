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

#include "mpi.h"
#include "fenix-config.h"
#include "fenix_ext.hpp"
#include "fenix_data_recovery.hpp"
#include "fenix_data_member.hpp"
#include "fenix_data_packet.hpp"


using namespace Fenix::Data;

/**
 * @brief
 * @param
 * @param
 */
int __fenix_search_memberid(fenix_group_t* group, int key) {
  return group->search_member(key).first;
}


fenix_member_entry_t* __fenix_data_member_add_entry(fenix_group_t* group,
        int memberid, void* data, int count, int datatype_size){
    fenix_member_entry_t mentry;
    mentry.memberid = memberid;
    mentry.state = OCCUPIED;
    mentry.user_data = data;
    mentry.current_count = count;
    mentry.datatype_size = datatype_size;
    group->members.push_back(mentry);

    return &group->members.back();
}

int __fenix_data_member_send_metadata(int groupid, int memberid, int dest_rank){
    auto [group_index, group] = find_group(groupid);
    if(!group) return FENIX_ERROR_INVALID_GROUPID;

    auto [member_index, member] = group->find_member(memberid);
    if(!member) return FENIX_ERROR_INVALID_MEMBERID;

    fenix_member_entry_packet_t packet;
    packet.memberid = member->memberid;
    packet.datatype_size = member->datatype_size;
    packet.current_count = member->current_count;

    MPI_Send(&packet, sizeof(packet), MPI_BYTE, dest_rank, RECOVER_MEMBER_ENTRY_TAG^groupid,
            group->comm);

    return FENIX_SUCCESS;
}

int __fenix_data_member_recv_metadata(int groupid, int src_rank, 
        fenix_member_entry_packet_t* packet){
    auto group = find_group(groupid).second;
    if(!group) return FENIX_ERROR_INVALID_GROUPID;

    MPI_Recv((void*)packet, sizeof(fenix_member_entry_packet_t), MPI_BYTE, src_rank,
            RECOVER_MEMBER_ENTRY_TAG^groupid, group->comm, NULL);

    return FENIX_SUCCESS;
}


/**
 * @brief
 * @param
 * @param
 */
void __fenix_data_member_reinit(fenix_group_t *group, fenix_two_container_packet_t packet,
                   enum states mystatus) {
  group->members.clear();
}
