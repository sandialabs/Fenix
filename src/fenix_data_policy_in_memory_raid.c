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

#include <mpi.h>
#include "fenix.h"
#include "fenix_data_policy.h"
#include "fenix_data_group.h"
#include "fenix_data_member.h"

int __imr_group_delete(fenix_group_t* group);
int __imr_member_create(fenix_group_t* group, int member_id, 
        void* source_buffer, int count, MPI_Datatype datatype);
int __imr_member_delete(fenix_group_t* group, int member_id);
int __imr_get_redundant_policy(fenix_group_t*, int* policy_name, 
        void* policy_value, int* flag);
int __imr_member_store(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier);
int __imr_member_storev(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier);
int __imr_member_istore(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier, Fenix_Request *request);
int __imr_member_istorev(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier, Fenix_Request *request);
int __imr_commit(fenix_group_t* group);
int __imr_snapshot_delete(fenix_group_t* group, int time_stamp);
int __imr_barrier(fenix_group_t* group);
int __imr_member_restore(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp);
int __imr_member_restore_from_rank(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp, 
        int source_rank);
int __imr_member_get_attribute(fenix_group_t* group, fenix_member_t* member, 
        int attributename, void* attributevalue, int* flag, int sourcerank);
int __imr_member_set_attribute(fenix_group_t* group, fenix_member_t* member, 
           int attributename, void* attributevalue, int* flag);
int __imr_get_number_of_snapshots(fenix_group_t* group, 
        int* number_of_snapshots);
int __imr_get_snapshot_at_position(fenix_group_t* group, int position,
        int* time_stamp);
int __imr_reinit(fenix_group_t* group);

typedef struct __fenix_imr_group{
   fenix_group_t base;
   int raid_mode;
   int rank_separation;
   
} fenix_imr_group_t;

void __fenix_policy_in_memory_raid_get_group(fenix_group_t** group, MPI_Comm comm, 
      int timestart, int depth, void* policy_value, int* flag){
   *group = (fenix_group_t *)malloc(sizeof(fenix_imr_group_t));
   fenix_imr_group_t *new_group = (fenix_imr_group_t *)(*group);
   new_group->base.vtbl.group_delete = *__imr_group_delete;
   new_group->base.vtbl.member_create = *__imr_member_create;
   new_group->base.vtbl.member_delete = *__imr_member_delete;
   new_group->base.vtbl.get_redundant_policy = *__imr_get_redundant_policy;
   new_group->base.vtbl.member_store = *__imr_member_store;
   new_group->base.vtbl.member_storev = *__imr_member_storev;
   new_group->base.vtbl.member_istore = *__imr_member_istore;
   new_group->base.vtbl.member_istorev = *__imr_member_istorev;
   new_group->base.vtbl.commit = *__imr_commit;
   new_group->base.vtbl.snapshot_delete = *__imr_snapshot_delete;
   new_group->base.vtbl.barrier = *__imr_barrier;
   new_group->base.vtbl.member_restore = *__imr_member_restore;
   new_group->base.vtbl.member_restore_from_rank = *__imr_member_restore_from_rank;
   new_group->base.vtbl.member_get_attribute = *__imr_member_get_attribute;
   new_group->base.vtbl.member_set_attribute = *__imr_member_set_attribute;
   new_group->base.vtbl.get_number_of_snapshots = *__imr_get_number_of_snapshots;
   new_group->base.vtbl.get_snapshot_at_position = *__imr_get_snapshot_at_position;
   new_group->base.vtbl.reinit = *__imr_reinit;

   int* policy_vals = (int*)policy_value;
   new_group->raid_mode = policy_vals[0];
   new_group->rank_separation = policy_vals[1];

   *flag = FENIX_SUCCESS;
}

int __imr_member_create(fenix_group_t* group, int member_id, 
        void* source_buffer, int count, MPI_Datatype datatype){return 0;}
int __imr_member_delete(fenix_group_t* group, int member_id){return 0;}
int __imr_member_store(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier){return 0;}
int __imr_member_storev(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier){return 0;}
int __imr_member_istore(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier, Fenix_Request *request){return 0;}
int __imr_member_istorev(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier, Fenix_Request *request){return 0;}
int __imr_commit(fenix_group_t* group){return 0;}
int __imr_snapshot_delete(fenix_group_t* group, int time_stamp){return 0;}
int __imr_barrier(fenix_group_t* group){return 0;}
int __imr_member_restore(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp){return 0;}
int __imr_member_restore_from_rank(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp, 
        int source_rank){return 0;}
int __imr_member_get_attribute(fenix_group_t* group, fenix_member_t* member, 
        int attributename, void* attributevalue, int* flag, int sourcerank){return 0;}
int __imr_member_set_attribute(fenix_group_t* group, fenix_member_t* member, 
           int attributename, void* attributevalue, int* flag){return 0;}
int __imr_get_number_of_snapshots(fenix_group_t* group, 
        int* number_of_snapshots){return 0;}
int __imr_get_snapshot_at_position(fenix_group_t* group, int position,
        int* time_stamp){return 0;}
int __imr_reinit(fenix_group_t* group){return 0;}

int __imr_get_redundant_policy(fenix_group_t* group, int* policy_name, 
        void* policy_value, int* flag){
   int retval = FENIX_SUCCESS;
   *policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID;
   
   fenix_imr_group_t* full_group = (fenix_imr_group_t *)group;
   int* policy_vals = (int*) policy_value;
   policy_vals[0] = full_group->raid_mode;
   policy_vals[1] = full_group->rank_separation;

   *flag = FENIX_SUCCESS;
   return retval;   
}

int __imr_group_delete(fenix_group_t* g){
   fenix_imr_group_t* group = (fenix_imr_group_t*) g;
   //We don't (currently) need to do anything special
   //for deletion. Just delete the member struct, then free
   //memory.
   __fenix_data_member_destroy(group->base.member);

   free(group);
   return FENIX_SUCCESS;
}
