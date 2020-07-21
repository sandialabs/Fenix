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
#include "fenix_data_group.h"
#include "fenix_data_member.h"
#include "fenix_data_packet.h"



/**
 * @brief
 */
fenix_data_recovery_t * __fenix_data_recovery_init() {
  fenix_data_recovery_t *data_recovery = (fenix_data_recovery_t *)
          s_calloc(1, sizeof(fenix_data_recovery_t));

  data_recovery->count = 0;
  data_recovery->total_size = __FENIX_DEFAULT_GROUP_SIZE;
  data_recovery->group = (fenix_group_t **) s_malloc(
          __FENIX_DEFAULT_GROUP_SIZE * sizeof(fenix_group_t *));

  if (fenix.options.verbose == 41) {
    verbose_print("c-rank: %d, role: %d, g-count: %zu, g-size: %zu\n",
                    __fenix_get_current_rank(fenix.world), fenix.role, data_recovery->count,
                  data_recovery->total_size);
  }

  return data_recovery;
}

int __fenix_member_delete(int groupid, int memberid) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  int member_index = -1;
  if(group_index !=-1){
    member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid);
  }

  if (fenix.options.verbose == 38) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_delete: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_delete: memberid <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_data_recovery_t *data_recovery = fenix.data_recovery;
    fenix_group_t *group = (data_recovery->group[group_index]);
    
    retval = group->vtbl.member_delete(group, memberid);
    
    if(retval == FENIX_SUCCESS){
      fenix_member_t *member = group->member;
      member->count--;
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      mentry->state = DELETED;
    }

    if (fenix.options.verbose == 38) {
      fenix_member_t *member = group->member;
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      
      verbose_print("c-rank: %d, role: %d, m-count: %zu, m-state: %d",
                      __fenix_get_current_rank(fenix.new_world), fenix.role,
                    member->count, mentry->state);
    }

    retval = FENIX_SUCCESS;
  }
  return retval;
}


int __fenix_group_delete_direct(fenix_group_t* group){
    //We need this function to remove any allocated pointers
    //that the fenix core adds before called the policy-specific
    //group delete. This lets us update fenix core's group struct
    //without having to potentially update the deletion process 
    //for each and every policy.
    //
    //We'll leave responsibility for calling 
    //__fenix_data_member_destroy()
    //to the policy, as it may be using that as a reference for
    //knowing how many members there are during its own deletion
    //process.
    
    return group->vtbl.group_delete(group);
}

int __fenix_data_recovery_remove_group(fenix_data_recovery_t* data_recovery, int group_index){
    int retval = !FENIX_SUCCESS;
    if(group_index != -1){
        for(int index = group_index; index < data_recovery->count-1; index++){
            data_recovery->group[index] = data_recovery->group[index+1];
        }
        data_recovery->count--;
        retval = FENIX_SUCCESS;
    }
    return retval;
}

/**
 * @brief
 * @param group_id
 */
int __fenix_group_delete(int groupid) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );

  if (fenix.options.verbose == 37) {
    verbose_print("c-rank: %d, group_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), group_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_group_delete: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    /* Delete Process */
    fenix_data_recovery_t *data_recovery = fenix.data_recovery;
    fenix_group_t *group = (data_recovery->group[group_index]);
    retval = __fenix_group_delete_direct(group);
    
    if(retval == FENIX_SUCCESS){ 
        retval = __fenix_data_recovery_remove_group(data_recovery, group_index);
    }

  }
  return retval;
}


void __fenix_data_recovery_destroy( fenix_data_recovery_t *data_recovery )  {
  int group_index;
  for ( group_index = 0; group_index < data_recovery->count; group_index++ ) {
      fenix_group_t* group = (data_recovery->group[group_index]);
      
      //Specific data policy function frees any data policy constructs
      __fenix_group_delete_direct(group);
  }
  free( data_recovery->group );
  free( data_recovery );
}

/**
 * @brief
 * @param
 * @param
 */
void __fenix_data_recovery_reinit(fenix_data_recovery_t *data_recovery, 
        fenix_two_container_packet_t packet) {
  int start_index = data_recovery->total_size;
  data_recovery->count = packet.count;
  data_recovery->total_size = packet.total_size;
  data_recovery->group = (fenix_group_t **) s_realloc(data_recovery->group,
                                                         (data_recovery->total_size) *
                                                         sizeof(fenix_group_t *));

  if (fenix.options.verbose == 48) {
    verbose_print("c-rank: %d, role: %d, g-size: %zu\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, 
                    data_recovery->total_size);
  }
}

/**
 * @brief
 * @param
 */
void __fenix_ensure_data_recovery_capacity(fenix_data_recovery_t* data_recovery) {
  //If we're ensuring there is space for a new group, we need to check that count+1 is < size
  if (data_recovery->count +1 >= data_recovery->total_size) {
    int start_index = data_recovery->total_size;
    data_recovery->group = (fenix_group_t **) s_realloc(data_recovery->group,
                                                           (data_recovery->total_size * 2) *
                                                           sizeof(fenix_group_t *));
    data_recovery->total_size = data_recovery->total_size * 2;

    if (fenix.options.verbose == 51) {
      verbose_print("g-count: %zu, g-size: %zu\n", data_recovery->count, data_recovery->total_size);
    }
  }
}

/**
 * @brief
 * @param
 */
int __fenix_search_groupid(int key, fenix_data_recovery_t *data_recovery) {
  int group_index, found = -1, index = -1;
  for (group_index = 0;
       (found != 1) && (group_index < data_recovery->count); group_index++) {
    fenix_group_t *group = (data_recovery->group[group_index]);
    if (key == group->groupid) {
      index = group_index;
      found = 1;
    }
  }
  return index;
}

/**
 * @brief
 * @param
 */
int __fenix_find_next_group_position( fenix_data_recovery_t *data_recovery ) {
  //Ensure that we have space.
  __fenix_ensure_data_recovery_capacity(data_recovery);
  return data_recovery->count;
}
