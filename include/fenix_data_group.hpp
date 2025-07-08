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
#ifndef __FENIX_DATA_GROUP_H__
#define __FENIX_DATA_GROUP_H__

#include <map>

#include <mpi.h>
#include "fenix.h"
#include "fenix_data_member.hpp"
#include "fenix_data_packet.hpp"
#include "fenix_util.hpp"
#include "fenix_data_subset.hpp"

#define __FENIX_DEFAULT_GROUP_SIZE 32

namespace Fenix::Data {

using member_iterator = std::pair<int, fenix_member_entry_t*>;

//We keep basic bookkeeping info here, policy specific
//information is kept by the policy's data type.
struct fenix_group_t {
    int groupid;
    MPI_Comm comm;
    int comm_size;
    int current_rank;
    int timestart;
    int timestamp;
    int depth;
    int policy_name;
    std::map<int, fenix_member_entry_t> members;

    //Search for id, returning {-1, nullptr} if not found.
    Fenix::Data::member_iterator search_member(int id);
    //As search_member, but print an error message if id not found.
    Fenix::Data::member_iterator find_member(int id);
    
    virtual int group_delete() = 0;
    virtual int member_create(fenix_member_entry_t* member) = 0;
    virtual int member_delete(int memberid) = 0;
    virtual int get_redundant_policy(int* name, void* value, int* flag) = 0;
    virtual int member_store(int memberid, const DataSubset& subset) = 0;
    virtual int member_storev(int memberid, const DataSubset& subset) = 0;
    virtual int member_istore(int memberid, const DataSubset& subset, Fenix_Request* req) = 0;
    virtual int member_istorev(int memberid, const DataSubset& subset, Fenix_Request* req) = 0;
    virtual int commit() = 0;
    virtual int snapshot_delete(int timestamp) = 0;
    virtual int barrier() = 0;
    virtual int member_restore(int member_id, void* target_bugger, int max, int timestamp, DataSubset& data_found) = 0;
    virtual int member_lrestore(int member_id, void* target_bugger, int max, int timestamp, DataSubset& data_found) = 0;
    virtual int member_restore_from_rank(int member_id, void* target_bugger, int max, int timestamp, int source_rank) = 0;
    virtual int get_number_of_snapshots(int* num) = 0;
    virtual int get_snapshot_at_position(int position, int* timestamp) = 0;
    virtual int reinit(int* flag) = 0;
    virtual int member_get_attribute(fenix_member_entry_t* mentry, int name, void* value, int* flag, int sourcerank) = 0;
    virtual int member_set_attribute(fenix_member_entry_t* mentry, int name, void* value, int* flag) = 0;
};

typedef struct __fenix_data_recovery {
    size_t count;
    size_t total_size;
    Fenix::Data::fenix_group_t **group;
} fenix_data_recovery_t;

typedef struct __group_entry_packet {
    int groupid;
    int timestamp;
    int depth;
    int rank_separation;
} fenix_group_entry_packet_t;

fenix_data_recovery_t * __fenix_data_recovery_init();

void __fenix_data_recovery_destroy( fenix_data_recovery_t *fx_data_recovery );

void __fenix_data_recovery_reinit( fenix_data_recovery_t *dr, fenix_two_container_packet_t packet);

void __fenix_ensure_data_recovery_capacity( fenix_data_recovery_t *dr);

int __fenix_search_groupid( int key, fenix_data_recovery_t *dr);

int __fenix_find_next_group_position( fenix_data_recovery_t *dr );

using group_iterator = std::pair<int, fenix_group_t*>;

group_iterator find_group(int id);
group_iterator find_group(int id, fenix_data_recovery_t *dr);

} //end namespace Fenix::Data

#endif // FENIX_DATA_GROUP_H
