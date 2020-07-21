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
#include "fenix_ext.h"
#include "fenix_data_recovery.h"
#include "fenix_data_member.h"
#include "fenix_data_packet.h"


/**
 * @brief
 */
fenix_member_t *__fenix_data_member_init() {
  fenix_member_t *member = (fenix_member_t *)
          s_calloc(1, sizeof(fenix_member_t));
  member->count = 0;
  member->total_size = __FENIX_DEFAULT_MEMBER_SIZE;
  member->member_entry = (fenix_member_entry_t *) s_malloc(
          __FENIX_DEFAULT_MEMBER_SIZE * sizeof(fenix_member_entry_t));

  if (fenix.options.verbose == 42) {
    verbose_print("c-rank: %d, role: %d, m-count: %zu, m-size: %zu\n",
                    __fenix_get_current_rank(fenix.world), fenix.role, member->count,
                  member->total_size);
  }

  int member_index;
  for (member_index = 0; member_index <
                         __FENIX_DEFAULT_MEMBER_SIZE; member_index++) { // insert default values
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->memberid = -1;
    mentry->state = EMPTY;

    if (fenix.options.verbose == 42) {
      verbose_print("c-rank: %d, role: %d, m-memberid: %d, m-state: %d\n",
                      __fenix_get_current_rank(fenix.world), fenix.role,
                    mentry->memberid, mentry->state);
    }
  }
  return member;
}

void __fenix_data_member_destroy( fenix_member_t *member ) {
  free( member->member_entry );
  free( member );
}

/**
 * @brief
 * @param
 * @param
 */
int __fenix_search_memberid(fenix_member_t* member, int key) {
  fenix_data_recovery_t *data_recovery = fenix.data_recovery;
  int member_index, found = -1, index = -1;
  for (member_index = 0;
       (found != 1) && (member_index < member->total_size); member_index++) {
    
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    if (!(mentry->state == EMPTY || mentry->state == DELETED) && key == mentry->memberid) {
      index = member_index;
      found = 1;
    }
  }
  return index;
}


/**
 * @brief
 * @param
 */
int __fenix_find_next_member_position(fenix_member_t *member) {
  __fenix_ensure_member_capacity(member);
  
  int member_index, found = -1, index = -1;
  for (member_index = 0;
       (found != 1) && (member_index < member->total_size); member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    if (mentry->state == EMPTY || mentry->state == DELETED) {
      index = member_index;
      found = 1;
    }
  }
  return index;
}

fenix_member_entry_t* __fenix_data_member_add_entry(fenix_member_t* member, 
        int memberid, void* data, int count, MPI_Datatype datatype){
    
    int member_index = __fenix_find_next_member_position(member);
    fenix_member_entry_t* mentry = member->member_entry + member_index;
    
    mentry->memberid = memberid;
    mentry->state = OCCUPIED;
    mentry->user_data = data;
    mentry->current_count = count;
    mentry->current_datatype = datatype;
    
    int dsize;
    MPI_Type_size(datatype, &dsize);
    mentry->datatype_size = dsize;

    member->count++;

    return mentry;
}

/**
 * @brief
 * @param
 */
void __fenix_ensure_member_capacity(fenix_member_t *m) {
  fenix_member_t *member = m;
  if (member->count +1 >= member->total_size) {
    int start_index = member->total_size;
    member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                              (member->total_size * 2) *
                                                              sizeof(fenix_member_entry_t));
    member->total_size = member->total_size * 2;

    if (fenix.options.verbose == 52) {
      verbose_print("c-rank: %d, role: %d, m-count: %zu, m-size: %zu\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role,
                    member->count, member->total_size);
    }

    int member_index;
    for (member_index = start_index; member_index < member->total_size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      mentry->memberid = -1;
      mentry->state = EMPTY;

      if (fenix.options.verbose == 52) {
        verbose_print(
                "c-rank: %d, role: %d, member[%d] m-memberid: %d, m-state: %d\n",
                  __fenix_get_current_rank(fenix.new_world), fenix.role,
                member_index, mentry->memberid, mentry->state);
      }
    }
  }
}


int __fenix_data_member_send_metadata(int groupid, int memberid, int dest_rank){
    int retval = -1;
    
    fenix_data_recovery_t* data_recovery = fenix.data_recovery;
    int group_index = __fenix_search_groupid(groupid, data_recovery);
    int member_index;
    if(group_index != -1){
        member_index = __fenix_search_memberid(
                data_recovery->group[group_index]->member, memberid);
    }
    
    if(group_index == -1){
        debug_print("ERROR Fenix_Data_member_delete: group_id <%d> does not exist\n",
                    groupid);
        retval = FENIX_ERROR_INVALID_GROUPID; 
    } else if(member_index == -1){
        debug_print("ERROR Fenix_Data_member_delete: memberid <%d> does not exist\n",
                    memberid);
        retval = FENIX_ERROR_INVALID_MEMBERID; 
    } else {
        fenix_group_t *group = data_recovery->group[group_index];
        fenix_member_entry_t mentry = group->member->member_entry[member_index];
        
        fenix_member_entry_packet_t packet;
        packet.memberid = mentry.memberid;
        packet.current_datatype = mentry.current_datatype;
        packet.datatype_size = mentry.datatype_size;
        packet.current_count = mentry.current_count;

        MPI_Send(&packet, sizeof(packet), MPI_BYTE, dest_rank, RECOVER_MEMBER_ENTRY_TAG^groupid,
                group->comm);

        retval = FENIX_SUCCESS;
    }

    return retval;
}

int __fenix_data_member_recv_metadata(int groupid, int src_rank, 
        fenix_member_entry_packet_t* packet){
    int retval = -1;
    
    fenix_data_recovery_t* data_recovery = fenix.data_recovery;
    int group_index = __fenix_search_groupid(groupid, data_recovery);
    
    if(group_index == -1){
        debug_print("ERROR Fenix_Data_member_delete: group_id <%d> does not exist\n",
                    groupid);
        retval = FENIX_ERROR_INVALID_GROUPID; 
    } else {
        fenix_group_t* group = data_recovery->group[group_index];

        MPI_Recv((void*)packet, sizeof(fenix_member_entry_packet_t), MPI_BYTE, src_rank, 
                RECOVER_MEMBER_ENTRY_TAG^groupid, group->comm, NULL);

        retval = FENIX_SUCCESS;
    }
    

    return retval;
}


/**
 * @brief
 * @param
 * @param
 */
void __fenix_data_member_reinit(fenix_member_t *m, fenix_two_container_packet_t packet,
                   enum states mystatus) {
  fenix_member_t *member = m;
  int start_index = member->total_size;
  member->count = 0;
  member->total_size = packet.total_size;
  member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                            (member->total_size) *
                                                            sizeof(fenix_member_entry_t));
  if (fenix.options.verbose == 50) {
    verbose_print("c-rank: %d, role: %d, m-count: %zu, m-size: %zu\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role,
                  member->count, member->total_size);
  }

  int member_index;
  /* Why start_index is set to the number of member entries ? */
  // for (member_index = start_index; member_index < member->size; member_index++) {
  for (member_index = 0; member_index < member->total_size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->memberid = -1;
    mentry->state = mystatus;
    if (fenix.options.verbose == 50) {
      verbose_print("c-rank: %d, role: %d, m-memberid: %d, m-state: %d\n",
                      __fenix_get_current_rank(fenix.new_world), fenix.role,
                    mentry->memberid, mentry->state);
    }
  }
}
