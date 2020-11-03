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
//        Michael Heroux, and Matthew Whitloc
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/



#include "fenix_data_recovery.h"
#include "fenix_data_policy.h"
#include "fenix_opt.h"
//#include "fenix_process_recovery.h"
#include "fenix_util.h"
#include "fenix_ext.h"


/**
 * @brief           create new group or recover group data for lost processes
 * @param groud_id  
 * @param comm
 * @param time_start
 * @param depth
 */
int __fenix_group_create( int groupid, MPI_Comm comm, int timestart, int depth, int policy_name, 
        void* policy_value, int* flag) {

  int retval = -1;

  /* Retrieve the array index of the group maintained under the cover. */
  int group_index = __fenix_search_groupid( groupid, fenix.data_recovery );

  if (fenix.options.verbose == 12) {

    verbose_print("c-rank: %d, group_index: %d\n",   __fenix_get_current_rank(fenix.new_world), group_index);
  }


  /* Check the integrity of user's input */
  if ( timestart < 0 ) {

    debug_print("ERROR Fenix_Data_group_create: time_stamp <%d> must be greater than or equal to zero\n", timestart);
    retval = FENIX_ERROR_INVALID_TIMESTAMP;

  } else if ( depth < -1 ) {

    debug_print("ERROR Fenix_Data_group_create: depth <%d> must be greater than or equal to -1\n",depth);
    retval = FENIX_ERROR_INVALID_DEPTH;

  } else {

    /* This code block checks the need for data recovery.   */
    /* If so, recover the data and set the recovery         */
    /* for member recovery.                                 */

    int i;
    int remote_need_recovery;
    fenix_group_t *group;
    MPI_Status status;

    fenix_data_recovery_t *data_recovery = fenix.data_recovery;

    /* Initialize Group.  The group hasn't been created.       */
    /* I am either a brand-new process or a recovered process. */
    if (group_index == -1 ) {

      if (fenix.options.verbose == 12 &&   __fenix_get_current_rank(comm) == 0) {
         printf("this is a new group!\n"); 
      }

      /* Obtain an available group slot */
      group_index = __fenix_find_next_group_position(data_recovery);

      /* Initialize Group */
      __fenix_policy_get_group(data_recovery->group + group_index, comm, timestart, 
              depth, policy_name, policy_value, flag);
      
      //The group has filled any group-specific details, we need to fill in the core details.
      group = (data_recovery->group[ group_index ] );
      group->groupid = groupid;
      group->timestart = timestart;
      group->timestamp = -1; //indicates no commits yet
      group->depth = depth;
      group->member = __fenix_data_member_init();
      group->comm = comm;
      MPI_Comm_rank(comm, &(group->current_rank));


      //Update the count AFTER finding next group position.
      data_recovery->count++;

      if ( fenix.options.verbose == 12) {
        verbose_print(
                "c-rank: %d, g-groupid: %d, g-timestart: %d, g-depth: %d\n",
                __fenix_get_current_rank(fenix.new_world), group->groupid,
                group->timestart,
                group->depth);
      }

    } else { /* Already created. Renew the MPI communicator  */

      group = ( data_recovery->group[group_index] );
      group->comm = comm; /* Renew communicator */
      MPI_Comm_rank(comm, &(group->current_rank));


      //Reinit group metadata as needed w/ new communicator.
      group->vtbl.reinit(group, flag);
    }


    /* Global agreement among the group */
    retval = FENIX_SUCCESS;
  }
  return retval;
}

int __fenix_group_get_redundancy_policy(int groupid, int* policy_name, int* policy_value, int* flag){
  int retval = -1;
  int group_index = __fenix_search_groupid( groupid, fenix.data_recovery );
  
  if(group_index == -1){
    debug_print("ERROR Fenix_Data_member_create: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t* group = fenix.data_recovery->group[group_index];
    retval = group->vtbl.get_redundant_policy(group, policy_name, policy_value, flag);
  }

  return retval;
}


/**
 * @brief
 * @param group_id
 * @param member_id
 * @param data
 * @param count
 * @param data_type
 */
int __fenix_member_create(int groupid, int memberid, void *data, int count, MPI_Datatype datatype ) {

  int retval = -1;
  int group_index = __fenix_search_groupid( groupid, fenix.data_recovery );
  int member_index = -1;
  if(group_index != -1) member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid );

  if (fenix.options.verbose == 13) {
    verbose_print("c-rank: %d, group_index: %d, member_index: %d\n",
                   __fenix_get_current_rank(fenix.new_world),
                  group_index, member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_create: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index != -1) {
    debug_print("ERROR Fenix_Data_member_create: member_id <%d> already exists\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;

  } else {

    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    fenix_member_t *member = group->member;

    //First, we'll make a fenix-core member entry, then pass that info to
    //the specific data policy.
    int member_index = __fenix_find_next_member_position(member);
    fenix_member_entry_t* mentry;
    mentry = __fenix_data_member_add_entry(member, memberid, data, count, datatype);

    //Pass the info along to the policy
    retval = group->vtbl.member_create(group, mentry);

  }
  return retval;
  /* No Potential Bug in 2/10/17 */
}


/**
 * @brief
 * @param request
 */
int __fenix_data_wait( Fenix_Request request ) {
  int retval = -1;
  int result = __fenix_mpi_wait(&(request.mpi_recv_req));

  if (result != MPI_SUCCESS) {
    retval = FENIX_SUCCESS;
  } else {
    retval = FENIX_ERROR_DATA_WAIT;
  }

  result = __fenix_mpi_wait(&(request.mpi_send_req));

  if (result != MPI_SUCCESS) {
    retval = FENIX_SUCCESS;
  } else {
    retval = FENIX_ERROR_DATA_WAIT;
  }

  return retval;
}

/**
 * @brief
 * @param request
 * @param flag
 */
int __fenix_data_test(Fenix_Request request, int *flag) {
  int retval = -1;
  int result = ( __fenix_mpi_test(&(request.mpi_recv_req)) & __fenix_mpi_test(&(request.mpi_send_req))) ;

  if ( result == 1 ) {
    *flag = 1;
    retval = FENIX_SUCCESS;
  } else {
    *flag = 0 ; // incomplete error?
    retval = FENIX_ERROR_DATA_WAIT;
  }
  return retval;
  /* Good 2/10/17 */
}

/**
 * @brief // TODO: implement FENIX_DATA_MEMBER_ALL
 * @param group_id
 * @param member_id
 * @param subset_specifier
 *
 */

int __fenix_member_store(int groupid, int memberid, Fenix_Data_subset specifier) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  int member_index = -1;

  /* Check if the member id already exists. If so, the index of the storage space is assigned */
  if (group_index !=-1 && memberid != FENIX_DATA_MEMBER_ALL) {
    member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid );
  }

  if (fenix.options.verbose == 18 && fenix.data_recovery->group[group_index]->current_rank== 0 ) {
    verbose_print(
            "c-rank: %d, role: %d, group_index: %d, member_index: %d memberid: %d\n",
              __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
            member_index, memberid);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_store: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_store: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->vtbl.member_store(group, memberid, specifier);
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param subset_specifier
 * @param request
 */
int __fenix_member_istore(int groupid, int memberid, Fenix_Data_subset specifier,
                  Fenix_Request *request) {

  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  int member_index = -1;

  /* Check if the member id already exists. If so, the index of the storage space is assigned */
  if (group_index !=-1 && memberid != FENIX_DATA_MEMBER_ALL) {
    member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid );
  }

  if (fenix.options.verbose == 18 && fenix.data_recovery->group[group_index]->current_rank== 0 ) {
    verbose_print(
            "c-rank: %d, role: %d, group_index: %d, member_index: %d memberid: %d\n",
              __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
            member_index, memberid);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_store: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_store: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->vtbl.member_istore(group, memberid, specifier, request);
  }
  return retval;
}



void __fenix_subset(fenix_group_t *group, fenix_member_entry_t *me, Fenix_Data_subset *ss) {
#if 1
  fprintf(stderr,"ERROR Fenix_Subset is not currently supported\n");

#else
  fenix_version_t *version = &(me->version);
  fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
  fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);

  int i; 
  MPI_Status status;
  
  /* Store the local data */
  /* This version does not apply any storage saving scheme */
  memcpy(lentry->data, lentry->pdata, (lentry->count * lentry->size));

  /* Check the subset */
  int subset_total_size = 0;
  for( i = 0; i < ss->num_blocks; i++ ) {
    int subset_start = ss->start_offset[i];
    int subset_end = ss->start_offset[i];
    int subset_stride = ss->start_offset[i];
   
  }
  subset_total_size = ss->num_blocks * ss->fblk_size;

  /* Create a buffer for sending data  (lentry->size is a size of single element ) */
  void *subset_data = (void *) s_malloc(me->datatype_size *  subset_total_size );
   

  /* This data exchange is not necessary when using non-v call */ 
  member_store_packet_t lentry_packet, rentry_packet;
  lentry_packet.rank = lentry->currentrank;
  lentry_packet.datatype = lentry->datatype;
  lentry_packet.entry_count = lentry->count;
  lentry_packet.entry_size = subset_total_size;

  int current_rank =   __fenix_get_current_rank(fenix.new_world);
  int current_role = fenix.role;

  MPI_Sendrecv(&lentry_packet, sizeof(member_store_packet_t), MPI_BYTE, ge->out_rank,
                 STORE_SIZE_TAG, &rentry_packet, sizeof(member_store_packet_t), MPI_BYTE,
                 ge->in_rank, STORE_SIZE_TAG, (ge->comm), &status);
  
  rentry->remoterank = rentry_packet.rank;
  rentry->datatype = rentry_packet.datatype;
  rentry->count = rentry_packet.entry_count;
  rentry->size = rentry_packet.entry_size;

  if (rentry->data != NULL) {
      rentry->data = s_malloc(rentry->count * rentry->size);
  }

  /* Partner is sending subset */
  if( rentry->size != rentry->count  ) {
    /* Receive # of blocks */

  } 
  /* Handle Subset */
  int subset_num_blocks = ss->num_blocks;
  int subset_start = ss->start_offsets[0];
  int subset_end = ss->end_offsets[0];
  int subset_stride = ss->stride;
  int subset_diff = subset_end - subset_start;
  int subset_count = subset_num_blocks * subset_diff;

  int subset_block = 0;
  int subset_index = 0;
  void *subset_data = (void *) s_malloc(sizeof(void) * me->current_count);

  int data_index;
  int data_steps = 0;
  int data_count = me->current_count;
  for (data_index = subset_start; (subset_block != subset_num_blocks - 1) &&
                       (data_index < data_count); data_index++) {
    if (data_steps != subset_diff) {
      MPI_Sendrecv((lentry->data) + data_index, (1 * lentry->size), MPI_BYTE, ge->out_rank,
                   STORE_DATA_TAG, (rentry->data) + data_index, (1 * rentry->size), MPI_BYTE,
                   ge->in_rank, STORE_DATA_TAG, ge->comm, &status);
      // memcpy((subset_data) + data_index, (me->current_buf) + data_index, sizeof(me->current_datatype));
      data_steps = data_steps + 1;
    } else if (data_steps == subset_diff) {
      data_steps = 0;
      subset_block = subset_block + 1;
      data_index = data_index + subset_stride - 1;
    }
  } 

  /* Need to update the version info */
  if (version->position < version->size - 1) {
      version->num_copies++;
      version->position++;
    } else { /* Back to 0 */
        version->position = 0;
    }
#endif
}


#if 0
/**
 * @brief
 * @param group_id
 * @param member_id
 * @param subset_specifier
 */
int __fenix_member_storev(int group_id, int member_id, Fenix_Data_subset subset_specifier) {

/*
 * Using the same routine for v and non-v routine.
 */
  int retval = -1;
  int group_index = __fenix_search_groupid( group_id, fenix.data_recovery );
  int member_index = __fenix_search_memberid(group_index, member_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_storev: group_id <%d> does not exist\n",
                group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_storev: member_id <%d> does not exist\n",
                member_id);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = fenix.data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    __fenix_ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);
    retval = FENIX_SUCCESS;
  }
  return retval;

}
#endif

#if 0
/**
 * @brief
 * @param group_id
 * @param member_id
 * @param subset_specifier
 * @param request 
 */
int __fenix_member_istorev(int group_id, int member_id, Fenix_Data_subset subset_specifier,
                   Fenix_Request *request) {

  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, __fenixi_g_data_recovery );
  int member_index = __fenix_search_memberid(group_index, member_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_istorev: group_id <%d> does not exist\n",
                group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_istorev: member_id <%d> does not exist\n",
                member_id);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = fenix.data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    __fenix_ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);
    retval = FENIX_SUCCESS;
  }

  return retval;
}
#endif

/**
 * @brief
 * @param group_id
 * @param time_stamp
 */
int __fenix_data_commit(int groupid, int *timestamp) {
  /* No communication is performed */
  /* Return the new timestamp      */
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  if (fenix.options.verbose == 22) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",   __fenix_get_current_rank(fenix.new_world), fenix.role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    
    group->vtbl.commit(group);

    if (group->timestamp +1 -1) group->timestamp++;
    else group->timestamp = group->timestart;
    
    if (timestamp != NULL) {
      *timestamp = group->timestamp;
    }

    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param time_stamp
 */
int __fenix_data_commit_barrier(int groupid, int *timestamp) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  if (fenix.options.verbose == 23) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    
    retval = group->vtbl.commit(group);
    
    int min_timestamp;
    MPI_Allreduce( &(group->timestamp), &min_timestamp, 1, MPI_INT, MPI_MIN,  group->comm );

    if (timestamp != NULL) {
      *timestamp = group->timestamp;
    }
  }
  return retval;
}


/**
 * @brief
 * @param group_id
 * @param member_id
 * @param data
 * @param max_count
 * @param time_stamp
 */
int __fenix_member_restore(int groupid, int memberid, void *data, int maxcount, int timestamp, Fenix_Data_subset* data_found) {

  int retval =  FENIX_SUCCESS;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery);
  int member_index = -1;

  if(group_index != -1) member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid);


  if (fenix.options.verbose == 25) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_restore: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->vtbl.member_restore(group, memberid, data, maxcount, timestamp, data_found);
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param target_buffer
 * @param max_count
 * @param time_stamp
 * @param source_rank
 */
int __fenix_member_restore_from_rank(int groupid, int memberid, void *target_buffer,
                             int max_count, int time_stamp, int source_rank) {
  int retval =  FENIX_SUCCESS;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery);
  int member_index = -1;
  
  if(group_index != -1) member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid);

  if (fenix.options.verbose == 25) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_restore: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->vtbl.member_restore_from_rank(group, memberid, target_buffer, 
            max_count, time_stamp, source_rank);
  }
  return retval;
}



/**
 * @brief
 * @param group_id
 * @param num_members
 */
int __fenix_get_number_of_members(int group_id, int *num_members) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, fenix.data_recovery );
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    *num_members = group->member->count;
    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param position
 */
int __fenix_get_member_at_position(int group_id, int *member_id, int position) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, fenix.data_recovery);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    fenix_member_t *member = group->member;
    if (position < 0 || position > (member->total_size) - 1) {
      debug_print(
              "ERROR Fenix_Data_group_get_member_at_position: position <%d> must be a value between 0 and number_of_members-1 \n",
              position);
      retval = FENIX_ERROR_INVALID_POSITION;
    } else {
      int member_index = ((member->total_size) - 1) - position;
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      *member_id = mentry->memberid;
      retval = FENIX_SUCCESS;
    }
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param num_snapshots
 */
int __fenix_get_number_of_snapshots(int group_id, int *num_snapshots) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, fenix.data_recovery );
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->vtbl.get_number_of_snapshots(group, num_snapshots);
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param position
 * @param time_stamp
 */
int __fenix_get_snapshot_at_position(int groupid, int position, int *timestamp) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  if (fenix.options.verbose == 33) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    *timestamp = group->timestamp - position;
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param attribute_name
 * @param attribute_value
 * @param flag
 * @param source_rank
 */
int __fenix_member_get_attribute(int groupid, int memberid, int attributename,
                         void *attributevalue, int *flag, int sourcerank) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  int member_index = -1;

  if(group_index != -1){
    member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid);
  }

  if (fenix.options.verbose == 34) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_attr_get: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_attr_get: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    fenix_member_t *member = group->member;
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);

    int retval = group->vtbl.member_get_attribute(group, mentry, attributename,
            attributevalue, flag, sourcerank);

  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param attribute_name
 * @param attribute_value
 * @param flag
 */
int __fenix_member_set_attribute(int groupid, int memberid, int attributename,
                         void *attributevalue, int *flag) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery );
  int member_index = -1;

  if(group_index != -1){
    member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index]->member, memberid);
  }
  
  if (fenix.options.verbose == 35) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }
  
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_attr_set: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_attr_set: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    int my_datatype_size;
    int myerr;
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    fenix_member_t *member = group->member;
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);

    //Always pass attribute changes along to group - they might have unknown attributes
    //or side-effects to handle from changes. They get change info before
    //changes are made, in case they need prior state.
    retval = group->vtbl.member_set_attribute(group, mentry, attributename,
            attributevalue, flag);
    
    switch (attributename) {
      case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
        mentry->user_data = attributevalue;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
        mentry->current_count = *((int *) (attributevalue));
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE:

        myerr = MPI_Type_size(*((MPI_Datatype *)(attributevalue)), &my_datatype_size);

        if( myerr ) {
          debug_print(
                  "ERROR Fenix_Data_member_attr_get: Fenix currently does not support this MPI_DATATYPE; invalid attribute_value <%d>\n",
                  attributevalue);
          retval = FENIX_ERROR_INVALID_ATTRIBUTE_NAME;
        }

        mentry->current_datatype = *((MPI_Datatype *)(attributevalue));
        mentry->datatype_size = my_datatype_size;
        retval = FENIX_SUCCESS;
        break;
      
      default:
        //Only an issue if the policy also doesn't have this attribute.
        if(retval){
          debug_print("ERROR Fenix_Data_member_attr_get: invalid attribute_name <%d>\n",
                      attributename);
          retval = FENIX_ERROR_INVALID_ATTRIBUTE_NAME;
        }
        break;
    }
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param time_stamp
 */
int __fenix_snapshot_delete(int group_id, int time_stamp) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, fenix.data_recovery );
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_snapshot_delete: group_id <%d> does not exist\n",
                group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (time_stamp < 0) {
    debug_print(
            "ERROR Fenix_Data_snapshot_delete: time_stamp <%d> must be greater than zero\n",
            time_stamp);
    retval = FENIX_ERROR_INVALID_TIMESTAMP;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->vtbl.snapshot_delete(group, time_stamp);
  }
  return retval;
}

///////////////////////////////////////////////////// TODO //

void __fenix_store_single() {


}

#if 0 //This needs to be reworked for the new data redundancy framework.
      //Lots of info about member versions etc. has been moved to policy-specific
      //data.
/**
 *
 */
void __feninx_dr_print_store() {
  int group, member, version, local, remote;
  fenix_data_recovery_t *current = fenix.data_recovery;
  int group_count = current->count;
  for (group = 0; group < group_count; group++) {
    int member_count = current->group[group]->member->count;
    for (member = 0; member < member_count; member++) {
      int version_count = current->group[group]->member->member_entry[member].version->count;
      for (version = 0; version < version_count; version++) {
        int local_data_count = current->group[group]->member->member_entry[member].version->local_entry[version].count;
        int *local_data = current->group[group]->member->member_entry[member].version->local_entry[version].data;
        for (local = 0; local < local_data_count; local++) {
          //printf("*** store rank[%d] group[%d] member[%d] local[%d]: %d\n",
          //get_current_rank(fenix.new_world), group, member, local,
          //local_data[local]);
        }
        int remote_data_count = current->group[group]->member->member_entry[member].version->remote_entry[version].count;
        int *remote_data = current->group[group]->member->member_entry[member].version->remote_entry[version].data;
        for (remote = 0; remote < remote_data_count; remote++) {
          printf("*** store rank[%d] group[%d] member[%d] remote[%d]: %d\n",
                   __fenix_get_current_rank(fenix.new_world), group, member, remote,
                 remote_data[remote]);
        }
      }
    }
  }
}

/**
 *
 */

void __fenix_dr_print_restore() {
  fenix_data_recovery_t *current = fenix.data_recovery;
  int group_count = current->count;
  int member_count = current->group[0]->member->count;
  int version_count = current->group[0]->member->member_entry[0].version->count;
  int local_data_count = current->group[0]->member->member_entry[0].version->local_entry[0].count;
  int remote_data_count = current->group[0]->member->member_entry[0].version->remote_entry[0].count;
  printf("*** restore rank: %d; group: %d; member: %d; local: %d; remote: %d\n",
           __fenix_get_current_rank(fenix.new_world), group_count, member_count,
         local_data_count,
         remote_data_count);
}

/**
 *
 */
void __fenix_dr_print_datastructure() {
  int group_index, member_index, version_index, remote_data_index, local_data_index;
  fenix_data_recovery_t *current = fenix.data_recovery;

  if (!current) {
    return;
  }

  printf("\n\ncurrent_rank: %d\n",   __fenix_get_current_rank(fenix.new_world));
  int group_size = current->total_size;
  for (group_index = 0; group_index < group_size; group_index++) {
    int depth = current->group[group_index]->depth;
    int groupid = current->group[group_index]->groupid;
    int timestamp = current->group[group_index]->timestamp;
    int group_state = current->group[group_index]->state;
    int member_size = current->group[group_index]->member->total_size;
    int member_count = current->group[group_index]->member->count;
    switch (group_state) {
      case EMPTY:
        printf("group[%d] depth: %d groupid: %d timestamp: %d state: %s member.size: %d member.count: %d\n",
               group_index, depth, groupid, timestamp, "EMPTY", member_size,
               member_count);
        break;
      case OCCUPIED:
        printf("group[%d] depth: %d groupid: %d timestamp: %d state: %s member.size: %d member.count: %d\n",
               group_index, depth, groupid, timestamp, "OCCUPIED", member_size,
               member_count);
        break;
      case DELETED:
        printf("group[%d] depth: %d groupid: %d timestamp: %d state: %s member.size: %d member.count: %d\n",
               group_index, depth, groupid, timestamp, "DELETED", member_size,
               member_count);
        break;
      default:
        break;
    }

    for (member_index = 0; member_index < member_size; member_index++) {
      int memberid = current->group[group_index]->member->member_entry[member_index].memberid;
      int member_state = current->group[group_index]->member->member_entry[member_index].state;
      int version_size = current->group[group_index]->member->member_entry[member_index].version->total_size;
      int version_count = current->group[group_index]->member->member_entry[member_index].version->count;
      switch (member_state) {
        case EMPTY:
          printf("group[%d] member[%d] memberid: %d state: %s depth.size: %d depth.count: %d\n",
                 group_index, member_index, memberid, "EMPTY", version_size,
                 version_count);
          break;
        case OCCUPIED:
          printf("group[%d] member[%d] memberid: %d state: %s depth.size: %d depth.count: %d\n",
                 group_index, member_index, memberid, "OCCUPIED", version_size,
                 version_count);
          break;
        case DELETED:
          printf("group[%d] member[%d] memberid: %d state: %s depth.size: %d depth.count: %d\n",
                 group_index, member_index, memberid, "DELETED", version_size,
                 version_count);
          break;
        default:
          break;
      }

      for (version_index = 0; version_index < version_size; version_index++) {
        int local_data_count = current->group[group_index]->member->member_entry[member_index].version->local_entry[version_index].count;
        printf("group[%d] member[%d] version[%d] local_data.count: %d\n",
               group_index,
               member_index,
               version_index, local_data_count);
        if (current->group[group_index]->member->member_entry[member_index].version->local_entry[version_index].data !=
            NULL) {
          int *current_local_data = (int *) current->group[group_index]->member->member_entry[member_index].version->local_entry[version_index].data;
          for (local_data_index = 0; local_data_index < local_data_count; local_data_index++) {
            printf("group[%d] member[%d] depth[%d] local_data[%d]: %d\n",
                   group_index,
                   member_index,
                   version_index, local_data_index,
                   current_local_data[local_data_index]);
          }
        }

        int remote_data_count = current->group[group_index]->member->member_entry[member_index].version->remote_entry[version_index].count;
        printf("group[%d] member[%d] version[%d] remote_data.count: %d\n",
               group_index,
               member_index, version_index, remote_data_count);
        if (current->group[group_index]->member->member_entry[member_index].version->remote_entry[version_index].data !=
            NULL) {
          int *current_remote_data = current->group[group_index]->member->member_entry[member_index].version->remote_entry[version_index].data;
          for (remote_data_index = 0; remote_data_index < remote_data_count; remote_data_index++) {
            printf("group[%d] member[%d] depth[%d] remote_data[%d]: %d\n",
                   group_index,
                   member_index, version_index, remote_data_index,
                   current_remote_data[remote_data_index]);
          }
        }
      }
    }
  }
}
#endif
