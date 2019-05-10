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
//        and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/
#ifndef __FENIX_DATA_GROUP_H__
#define __FENIX_DATA_GROUP_H__

#include <mpi.h>
#include "fenix_data_member.h"
#include "fenix_data_packet.h"
#include "fenix_util.h"


#define __FENIX_DEFAULT_GROUP_SIZE 32

typedef struct __fenix_group_vtbl fenix_group_vtbl_t;
typedef struct __fenix_group fenix_group_t;

//This defines the functions which must be implemented by the group
typedef struct __fenix_group_vtbl {
   int (*group_delete)(fenix_group_t* group);
   
   int (*member_create)(fenix_group_t* group, fenix_member_entry_t* mentry);
   
   int (*member_delete)(fenix_group_t* group, int member_id);
   
   int (*get_redundant_policy)(fenix_group_t*, int* policy_name, 
           void* policy_value, int* flag);

   int (*member_store)(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier);

   int (*member_storev)(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier);

   int (*member_istore)(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier, Fenix_Request *request);

   int (*member_istorev)(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier, Fenix_Request *request);

   int (*commit)(fenix_group_t* group);

   int (*snapshot_delete)(fenix_group_t* group, int time_stamp);

   int (*barrier)(fenix_group_t* group);

   int (*member_restore)(fenix_group_t* group, int member_id,
           void* target_buffer, int max_count, int time_stamp);

   int (*member_restore_from_rank)(fenix_group_t* group, int member_id,
           void* target_buffer, int max_count, int time_stamp, 
           int source_rank);

   int (*get_number_of_snapshots)(fenix_group_t* group, 
           int* number_of_snapshots);

   int (*get_snapshot_at_position)(fenix_group_t* group, int position,
           int* time_stamp);

   int (*reinit)(fenix_group_t* group);

   int (*member_get_attribute)(fenix_group_t* group, fenix_member_t* member, 
           int attributename, void* attributevalue, int* flag, int sourcerank);
   
   int (*member_set_attribute)(fenix_group_t* group, fenix_member_t* member, 
           int attributename, void* attributevalue, int* flag);

} fenix_group_vtbl_t;

//We keep basic bookkeeping info here, policy specific
//information is kept by the policy's data type.
typedef struct __fenix_group {
    fenix_group_vtbl_t vtbl;
    int groupid;
    MPI_Comm comm;
    int comm_size;
    int current_rank;
    int timestart;
    int timestamp;
    int depth;
    int policy_name;
    fenix_member_t *member;
} fenix_group_t;

typedef struct __fenix_data_recovery {
    size_t count;
    size_t total_size;
    fenix_group_t **group;
} fenix_data_recovery_t;

typedef struct __group_entry_packet {
    int groupid;
    int timestamp;
    int depth;
    int rank_separation;
} fenix_group_entry_packet_t;

fenix_data_recovery_t * __fenix_data_recovery_init();

int __fenix_group_delete(int groupid);

int __fenix_member_delete(int groupid, int memberid);

void __fenix_data_recovery_destroy( fenix_data_recovery_t *fx_data_recovery );

void __fenix_data_recovery_reinit( fenix_data_recovery_t *dr, fenix_two_container_packet_t packet);

void __fenix_ensure_data_recovery_capacity( fenix_data_recovery_t *dr);

int __fenix_search_groupid( int key, fenix_data_recovery_t *dr );

int __fenix_find_next_group_position( fenix_data_recovery_t *dr );

#endif // FENIX_DATA_GROUP_H
