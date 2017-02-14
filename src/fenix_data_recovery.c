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


#include "fenix_constants.h"
#include "fenix_data_recovery.h"
#include "fenix_opt.h"
//#include "fenix_process_recovery.h"
#include "fenix_util.h"
#include "fenix_ext.h"
#include "fenix_data_recovery_ext.h"
#include "fenix_metadata.h"


/**
 * @brief           create new group or recover group data for lost processes
 * @param groud_id  
 * @param comm
 * @param time_stamp
 * @param depth
 */
int __fenix_group_create( int groupid, MPI_Comm comm, int timestamp, int depth ) {

  int retval = -1;

  /* Retrieve the array index of the group maintained under the cover. */
  int group_index = __fenix_search_groupid( groupid, __fenix_g_data_recovery );

  if (__fenix_options.verbose == 12) {

    verbose_print("c-rank: %d, group_index: %d\n",   __fenix_get_current_rank(*__fenix_g_new_world), group_index);

  }

  /* Check the integrity of user's input */
  if (timestamp < 0) {

    debug_print("ERROR Fenix_Data_group_create: time_stamp <%d> must be greater than or equal to zero\n", timestamp);
    retval = FENIX_ERROR_INVALID_TIMESTAMP;

  } else if (depth < -1) {

    debug_print("ERROR Fenix_Data_group_create: depth <%d> must be greater than or equal to -1\n",depth);
    retval = FENIX_ERROR_INVALID_DEPTH;

  } else {

#if 1
    /* This code block checks the need for data recovery.   */
    /* If so, recover the data and set the recovery         */
    /* for member recovery.                                 */

    int i, its_new = 1, group_position;
    int remote_need_recovery;
    fenix_group_entry_t *gentry;
    MPI_Status status;

    /* If the group data is already created. */
    fenix_group_t *group = __fenix_g_data_recovery;
    __fenix_ensure_group_capacity( group );

#if 0
    for (i = 0; i < group->count; i++) {
      if (groupid == group->group_entry[i].groupid) {
        group_position = i;
        its_new = 0; /* already created */
        break;
      }
    }
#endif
    its_new = ( group_index != -1 ) ? 0 : 1;


    /* Initialize Group.  The group hasn't been created.       */
    /* I am either a brand-new process or a recovered process. =*/
    if ( its_new == 1 ) {

      if (__fenix_options.verbose == 12 &&   __fenix_get_current_rank(comm) == 0) {
         printf("this is a new group!\n"); 
      }

      group->count++;
      /* Obtain an avaialble group slot */
      gentry = &(group->group_entry[ __fenix_find_next_group_position(group) ] );

      /* Initialize Group Meta Data */
      __fenix_init_group_metadata ( gentry, groupid, comm, timestamp, depth );

      /* fenix_g_role is kept until Fenix_finalize() */
      /* The group needs to keep the information.    */

      if ( __fenix_g_role == FENIX_ROLE_RECOVERED_RANK  ) {
        /* This flag is necessary */
        gentry->recovered = 1;

      } else {
        /* This flag indicates I am new */
        gentry->recovered = 0;

      }

      /**********************************************************/
      /* Initalize rank sepration.                              */
      /* This is only used for the current version of Fenix     */
      /* Future version will use TPL for distributed replica    */
      /* management.                                            */
      /**********************************************************/
      if ( __fenix_get_world_size( *__fenix_g_new_world ) / 4 > 1) {
        gentry->rank_separation =  __fenix_get_world_size( *__fenix_g_new_world) / 4;
      } else {
        gentry->rank_separation = 1;
      }

      if ( __fenix_options.verbose == 12) {
        verbose_print(
                "c-rank: %d, g-groupid: %d, g-timestamp: %d, g-depth: %d, g-state: %d\n",
                __fenix_get_current_rank(*__fenix_g_new_world), gentry->groupid,
                gentry->timestamp,
                gentry->depth,
                gentry->state);
      }

    } else { /* Already created. Renew the MPI communicator  */

      gentry = &( group->group_entry[group_position] );
      gentry->comm = comm; /* Renew communicator */

    }


    /* Re-iniitalize Group MetaData */
    __fenix_reinit_group_metadata ( gentry );

    /* Check the role of the neighbor define by the group */
    MPI_Sendrecv(&(gentry->recovered), 1, MPI_INT, gentry->out_rank, PARTNER_STATUS_TAG,
                 &remote_need_recovery, 1, MPI_INT, gentry->in_rank, PARTNER_STATUS_TAG,
                 gentry->comm, &status);

    /* Recover group information */
    if ( gentry->recovered == 0 && remote_need_recovery == 1) {

       if (__fenix_options.verbose == 18) {
        verbose_print(
                "c-rank: %d receiving group data %d from rank %d: remote data needs fix\n",
                __fenix_get_current_rank(*__fenix_g_new_world), gentry->groupid,
                gentry->out_rank);
      }

      /* Remote peer needs the data */
      retval = _send_metadata(gentry->current_rank, gentry->in_rank, gentry->comm);
      retval = _send_group_data(gentry->current_rank, gentry->in_rank, gentry, gentry->comm);

    } else if ( gentry->recovered == 1 && remote_need_recovery == 0) {
      if (__fenix_options.verbose == 18) {
        verbose_print(
                "c-rank: %d receiving group data %d from rank %d: I need to fix\n",
                 __fenix_get_current_rank(*__fenix_g_new_world), gentry->groupid,
                gentry->out_rank);
      }

      /* I need fix */
      retval = _recover_metadata(gentry->current_rank, gentry->out_rank, comm);
      retval = _recover_group_data(gentry->current_rank, gentry->out_rank, gentry, gentry->comm);

      /* Recovery is done. Change the flag */
      gentry->recovered = 0;

    } else if ( gentry->recovered == 1 && remote_need_recovery == 1 ) {
      /* I need recovery and you need recovery         */
      /* Curret Fenix cannot figure out how to recover */
      /* Thrown an error message                       */
      if (__fenix_options.verbose == 18) {
        verbose_print(
                "c-rank: %d receiving group data %d from rank %d: two ranks needing recovery cannot find the source of the original data\n",
                __fenix_get_current_rank(*__fenix_g_new_world), gentry->groupid,
                gentry->out_rank);
      }
      retval = FENIX_ERROR_NODATA_FOUND;
    }

    /* Need Error check for retval above */
    /* ********************************* */
#else
    /* Future Version will use other schemes to recover group data */

#endif

    /* Global agreement among the group */
    retval = ( __fenix_join_group(group, gentry, comm) != 1) ? FENIX_SUCCESS
                                                    : FENIX_ERROR_GROUP_CREATE;
  }
  return retval;
  /* As of 2/9. Cannot find any potential bugs */
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
  int group_index = __fenix_search_groupid( groupid, __fenix_g_data_recovery );
  int member_index = __fenix_search_memberid( group_index, memberid );

  if (__fenix_options.verbose == 13) {
    verbose_print("c-rank: %d, group_index: %d, member_index: %d\n",
                   __fenix_get_current_rank(*__fenix_g_new_world),
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

    int i;
    int remote_count;
    MPI_Status status;

    fenix_group_entry_t *gentry = &(__fenix_g_data_recovery->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    fenix_member_entry_t *mentry = NULL;
    fenix_version_t *version = NULL;

    /* Check if the entry needs to be reallocated */
    __fenix_ensure_member_capacity(member);


    member->count++;

    int next_member_position = __fenix_find_next_member_position(member);

    if (__fenix_options.verbose == 13) {
      verbose_print("c-rank: %d, next_member_position: %d\n",
                      __fenix_get_current_rank(*__fenix_g_new_world),
                    next_member_position);
    }

    mentry = &(member->member_entry[next_member_position]);

    __fenix_data_member_init_metadata( mentry, memberid, data, count, datatype );


//    version = &(member->member_entry[next_member_position].version);


    /* Check the size of data in the partner */
    /* We may need to send the data type at remote rank */
    MPI_Sendrecv( &count, 1, MPI_INT, gentry->out_rank, STORE_DATA_TAG + 1,
                  &remote_count, 1, MPI_INT, gentry->in_rank, STORE_DATA_TAG + 1,
                  gentry->comm, &status);

    /* Assging the address of the user data */
    mentry->user_data = data;

    version = mentry->version;
    /* Initalize the space for every single version           */
    /* No log-based storage method.                           */
    for (i = 0; i < gentry->depth; i++) {
      fenix_local_entry_t *lentry = &(version->local_entry[i]);
      fenix_remote_entry_t *rentry = &(version->remote_entry[i]);

      lentry->data = (void *) s_malloc(mentry->datatype_size * count);
      lentry->count = count;
      lentry->datatype_size = mentry->datatype_size;
      lentry->datatype = datatype;

      /* Bind the user data to Fenix */
      lentry->pdata = data; /* This should be handled by member entry rather than version entry */

      rentry->data = (void *) s_malloc(mentry->datatype_size * remote_count);
      rentry->pdata = NULL;
    }

    /* Calling global agreement */
    retval = ( __fenix_join_member(member, mentry, gentry->comm) != 1 ) ? FENIX_SUCCESS
                                                              : FENIX_ERROR_GROUP_CREATE;
  }
  return retval;
  /* No Potential Bug in 2/10/17 */
}

/**
 * @brief
 * @param group_id
 * @param policy_name
 * @param policy_value
 * @param flag
 */
int __fenix_group_get_redundancy_policy(int group_id, int policy_name, void *policy_value, int *flag) {

  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, __fenix_g_data_recovery );

  if (group_index == -1) {
    debug_print( "ERROR Fenix_Data_group_get_redundancy_policy: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);

    /* Use swtich statment to find policy values */
    switch (policy_name) {
      case FENIX_DATA_POLICY_PEER_RANK_SEPARATION:
        /* We need an index to record the information on policy values */
        memcpy(policy_value, &(gentry->rank_separation), sizeof(int));
        retval = FENIX_SUCCESS;
        break;
      default:
        debug_print(
                "ERROR Fenix_Data_group_get_redundancy_policy: the specified policyis not suppored by group_id <%d>\n",
                group_id);
        retval = -1;
        break;
    }
    *flag = FENIX_SUCCESS;
  }
  return retval;
}


/**
 * @brief
 * @param group_id
 * @param policy_name
 * @param policy_value
 * @param flag
 */
int __fenix_group_set_redundancy_policy(int group_id, int policy_name, void *policy_value,
                                int *flag) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, __fenix_g_data_recovery );

  if (group_index == -1) {

    debug_print( "ERROR Fenix_Data_group_set_redundancy_policy: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;

  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);

    switch (policy_name) {
      case FENIX_DATA_POLICY_PEER_RANK_SEPARATION:
        /* We need an index to record the information on policy values */
        memcpy(&(gentry->rank_separation), policy_value, sizeof(int));
        retval = FENIX_SUCCESS;
        *flag = FENIX_SUCCESS;
        break;
      default:
        debug_print( "ERROR Fenix_Data_group_get_redundancy_policy: the specified policyis not suppored by group_id <%d>\n", group_id);
        retval = -1;
        *flag = -1;
        break;
    }
  }
  return retval;
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
  /*
   *  This is covering store_v routine.
   */
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  int member_index = -1;

  /* Check the member id alreay exit. If so, the idex of the storage space is assigned */
  if (memberid != FENIX_DATA_MEMBER_ALL) {
    member_index = __fenix_search_memberid(group_index, memberid);
  }

  if (__fenix_options.verbose == 18 && __fenix_g_data_recovery->group_entry[group_index].current_rank== 0 ) {
    verbose_print(
            "c-rank: %d, role: %d, group_index: %d, member_index: %d memberid: %d\n",
              __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    /* When Subset specifier says EMPTY, the rank does not move any data, but need to keep the data from the partner. */
    /* This part should be sepaeated as a single function call */

    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;

    fenix_subset_offsets_t *loffsets = NULL, *roffsets = NULL;
    char *send_buff, *recv_buff;

    __fenix_ensure_version_capacity(member);

    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = mentry->version;
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);

    MPI_Status status;
    /* This data exchange is not necessary when using non-v call */
    fenix_member_store_packet_t lentry_packet, rentry_packet;
    /* Store the local data */
    memcpy(lentry->data, mentry->user_data, (lentry->count * lentry->datatype_size));
    if( specifier.specifier == __FENIX_SUBSET_FULL ) {

      __fenix_data_member_init_store_packet ( &lentry_packet, lentry, 0 );
      send_buff = (char *)lentry->data;
    } else if ( specifier.specifier == __FENIX_SUBSET_EMPTY ) {

      __fenix_data_member_init_store_packet ( &lentry_packet, lentry, 1 );
      send_buff = (char *)lentry->data;

    } else {

      char *ldata = (char *)lentry->data;
      lentry_packet.rank = lentry->currentrank;
      lentry_packet.datatype = lentry->datatype;
      lentry_packet.entry_count = lentry->count;
      lentry_packet.entry_size  = lentry->datatype_size;
      lentry_packet.num_blocks  = specifier.num_blocks;

      /* Create an array blocks defined by subset specifier */
      loffsets = (fenix_subset_offsets_t *) s_malloc(sizeof(fenix_subset_offsets_t) * specifier.num_blocks);

      /* Count the buffer storage size  */
      int index;
      int local_buf_count = 0;
      for (index = 0; index < specifier.num_blocks; index++) {
        loffsets[index].start = specifier.start_offsets[index];
        loffsets[index].end = specifier.end_offsets[index];
        local_buf_count += (specifier.end_offsets[index] - specifier.start_offsets[index]) + 1;
      }      

      lentry_packet.entry_real_count = local_buf_count; /* Tell how many elements is sent */

      /* Send buffer is temporary created */
      send_buff = (char *) s_malloc(sizeof(char) * local_buf_count * lentry->datatype_size);

      /* Pack the data into buffer */
      int offset_index = 0;
      for (index = 0; index < specifier.num_blocks; index++) {
        int n = (loffsets[index].end - loffsets[index].start) + 1;
        memcpy((void *) &send_buff[offset_index * lentry->datatype_size], (void *) &ldata[loffsets[index].start * lentry->datatype_size], n * lentry->datatype_size);
        offset_index += n;
      }      

    }

    int current_role = __fenix_g_role;
    MPI_Sendrecv(&lentry_packet, sizeof(fenix_member_store_packet_t), MPI_BYTE, gentry->out_rank,
               STORE_SIZE_TAG, &rentry_packet, sizeof(fenix_member_store_packet_t), MPI_BYTE,
               gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status);

    rentry->remoterank = rentry_packet.rank;
    rentry->datatype = rentry_packet.datatype;
    rentry->count = rentry_packet.entry_count;
    rentry->datatype_size = rentry_packet.entry_size;


    /* If it is not NULL, do not allocate */
    if (rentry->data != NULL) {
      rentry->data = s_malloc(rentry->count * rentry->datatype_size);
    }

    /* 
     * After sending member entry information, the use of subset is checked.
     * Case 1: Left neighbor and I use subset.  
     * Case 2: Left neighbor does not use subset, but I use subset.  
     * Case 3: Left neighbor uses subset, but I do not.  
     * Case 4: Left neighbor is not using subset.  Me either. 
     */
    if( rentry_packet.num_blocks > 0 && lentry_packet.num_blocks > 0) { 
      /* Exchange subset info */
      roffsets = (fenix_subset_offsets_t *)s_malloc(sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks);
      MPI_Sendrecv(loffsets, sizeof(fenix_subset_offsets_t)*lentry_packet.num_blocks,
                   MPI_BYTE, gentry->out_rank, STORE_SIZE_TAG, roffsets,
                   sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks, MPI_BYTE,
                   gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status);
      /* temporary buffer to received packed data */
      recv_buff = (char *)s_malloc(sizeof(char)* rentry_packet.entry_real_count * rentry->datatype_size );
    } else if ( lentry_packet.num_blocks > 0 ) {
      MPI_Send(&loffsets,  sizeof(fenix_subset_offsets_t)*lentry_packet.num_blocks, 
               MPI_BYTE, gentry->out_rank, STORE_SIZE_TAG,  (gentry->comm)  );

      /* The sender to me is not using subset */
      recv_buff = (char *) rentry->data;
    } else if ( rentry_packet.num_blocks > 0 ) {
      roffsets = (fenix_subset_offsets_t *)s_malloc(sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks);
      MPI_Recv(&roffsets, sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks, MPI_BYTE, 
               gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status );
      /* temporary buffer to received packed data */
      recv_buff = (char *)s_malloc(sizeof(char)* rentry_packet.entry_real_count * rentry->datatype_size );
    } else {

      /* The sender to me is not using subset */
      recv_buff = rentry->data;
    }

    /* Exchange the payload  */
    MPI_Sendrecv( (void *)send_buff, (lentry_packet.entry_real_count * lentry->datatype_size), MPI_BYTE, gentry->out_rank,
                  STORE_PAYLOAD_TAG, (void *) recv_buff, (rentry_packet.entry_real_count * rentry->datatype_size), MPI_BYTE,
                  gentry->in_rank, STORE_PAYLOAD_TAG, gentry->comm, &status);

    if ( lentry_packet.num_blocks > 0 ) {
      free( loffsets ); 
      free( send_buff ); 
    }

    if( rentry_packet.entry_real_count !=  rentry->count ) {
      /* Copy the data in the previous version */
      if ( gentry->depth > 1 ) { /* If the depth is one, the same rentry buffer is kept */
        memcpy( rentry->data, version->remote_entry[(version->position+gentry->depth-1)%gentry->depth].data, rentry->count );
      }

      /* Overright the rdata */
      if ( rentry_packet.num_blocks > 0 ) {
      /* Reorg received data */
        int i, j = 0;
        char * rdata = (char *) rentry->data;
        for ( i = 0; i < rentry_packet.num_blocks; i++ ) {
        int n = (roffsets[i].end - roffsets[i].start) + 1;
          memcpy((void*) &rdata[roffsets[i].start * rentry->datatype_size], (void *) &recv_buff[j*rentry->datatype_size], n * rentry->datatype_size);
          j += n;
        }
        free(roffsets);
        free(recv_buff);
      }
    }


    /* Need to update the version info */
    if (version->position < version->total_size - 1) {
      version->num_copies++;
      version->position++;
    } else { /* The position count is rewound to 0 */
      version->position = 0;
    }

    retval = FENIX_SUCCESS;
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
  #if 0
  int group_index = __fenix_search_groupid( groupid,  __fenix_g_data_recovery);
  int member_index = -1;
  if (memberid != FENIX_DATA_MEMBER_ALL) {
    member_index = __fenix_search_memberid(group_index, memberid);
  }

  if (__fenix_options.verbose == 18) {
    verbose_print(
            "c-rank: %d, role: %d, group_index: %d, member_index: %d memberid: %d\n",
              __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
            member_index, memberid);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_istore: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_istore: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else if (specifier.specifier == __FENIX_SUBSET_EMPTY) { // no need to store anything
    retval = FENIX_SUCCESS;
  } else {
    /* This part should be sepaeated as a sngle function call */
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    __fenix_ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);

    /* Make local copy                                         */
    /* Subset version will be supported in the future release  */
    memcpy(lentry->data, lentry->pdata, (lentry->count * lentry->size));

    MPI_Status status;
    /* This data exchange is not necessary when using non-v call */
    member_store_packet_t lentry_packet, rentry_packet;
    lentry_packet.rank = lentry->currentrank;
    lentry_packet.datatype = lentry->datatype;
    lentry_packet.entry_count = lentry->count;
    lentry_packet.entry_size = lentry->datatype_size;
    MPI_Sendrecv(&lentry_packet, sizeof(member_store_packet_t), MPI_BYTE, gentry->out_rank,
                 STORE_SIZE_TAG, &rentry_packet, sizeof(member_store_packet_t), MPI_BYTE,
                 gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status);
    rentry->remoterank = rentry_packet.rank;
    rentry->datatype = rentry_packet.datatype;
    rentry->count = rentry_packet.entry_count;
    rentry->size = rentry_packet.entry_size;

    /* If it is not NULL, do not allocate */
    if (rentry->data != NULL) {
      rentry->data = s_malloc(rentry->count * rentry->size);
    }

    MPI_Irecv( rentry->data, (rentry->count * rentry->size), MPI_BYTE,
               gentry->in_rank, STORE_DATA_TAG, gentry->comm, &(request->mpi_recv_req) );
    MPI_Isend( lentry->data, (lentry->count * lentry->size), MPI_BYTE, gentry->out_rank,
               STORE_DATA_TAG, gentry->comm, &(request->mpi_send_req));
  
    /* Need to update the version info */
    if (version->position < version->size - 1) {
      version->num_copies++;
      version->position++;
    } else { /* Back to 0 */
      version->position = 0;
    }

    retval = FENIX_SUCCESS;
  }
#endif
  return retval;
}



void __fenix_subset(fenix_group_entry_t *ge, fenix_member_entry_t *me, Fenix_Data_subset *ss) {
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

  int current_rank =   __fenix_get_current_rank(*__fenix_g_new_world);
  int current_role = __fenix_g_role;

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

fenix_local_entry_t *__fenix_subset_variable(fenix_member_entry_t *me, Fenix_Data_subset *ss) {
#if 1
  fprintf(stderr,"ERROR Fenix_Subset is not currently supported\n");

#else
  int ss_num_blocks = ss->num_blocks;
  int *ss_start = (int *) s_malloc(ss_num_blocks * sizeof(int));
  int *ss_end = (int *) s_malloc(ss_num_blocks * sizeof(int));;
  memcpy(ss_start, ss->start_offsets, (ss_num_blocks * sizeof(int)));
  memcpy(ss_end, ss->end_offsets, (ss_num_blocks * sizeof(int)));

  int index;
  int ss_count = 0;
  for (index = 0; index < ss_num_blocks; index++) {
    ss_count = ss_count + (ss_end[index] - ss_start[index]);
  }

  int offset_index = 0;
  int ss_block = 0;
  int ss_index = 0;
  int ss_diff = ss_end[0] - ss_start[0]; // init diff
  void *ss_data = (void *) s_malloc(sizeof(void) * ss_count);

  int data_index;
  int data_steps = 0;
  int data_count = me->current_count;
  for (data_index = 0;
       (ss_block != ss_num_blocks) && (data_index < data_count); data_index++) {
    if (data_index >= ss_start[offset_index] && data_index <= ss_end[offset_index]) {
      ss_diff = ss_end[offset_index] - ss_start[offset_index];
      memcpy((ss_data) + ss_index, (me->user_data) + data_index,
             sizeof(me->current_datatype));
      ss_index = ss_index + 1;
      data_steps = data_steps + 1;
    }
    if (data_steps == ss_diff) {
      data_steps = 0;
      offset_index = offset_index + 1;
      ss_block = ss_block + 1;
    }
  }

  fenix_local_entry_t *lentry = (fenix_local_entry_t *) s_malloc(
          sizeof(fenix_local_entry_t));
  lentry->currentrank = me->currentrank;
  lentry->count = ss_count;
  lentry->datatype = me->current_datatype;
  lentry->pdata = ss_data;
  lentry->size = sizeof(ss_data);
  lentry->data = s_malloc(lentry->count * lentry->size);
  memcpy(lentry->data, lentry->pdata, (lentry->count * lentry->size));

  return lentry;
  #endif


  return NULL;
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
  int group_index = __fenix_search_groupid( group_id, __fenix_g_data_recovery );
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
    fenix_group_t *group = __fenix_g_data_recovery;
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
    fenix_group_t *group = __fenix_g_data_recovery;
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
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  if (__fenix_options.verbose == 22) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",   __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->timestamp++;

    if (timestamp != NULL) {
      *timestamp = gentry->timestamp;
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
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  if (__fenix_options.verbose == 23) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    int min_timestamp;
    MPI_Allreduce( &(gentry->timestamp), &min_timestamp, 1, MPI_INT, MPI_MIN,  gentry->comm );

    gentry->timestamp = min_timestamp+1;
    if (timestamp != NULL) {
      *timestamp = gentry->timestamp;
    }

    fenix_member_t *member = gentry->member;
    int member_index;
    /* Iterate over current active members */
    for (member_index = 0; member_index < member->count; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      fenix_version_t *version = mentry->version;
      int version_index;
      #if 0
      /* Delete storage older than current timestamp + depth */
      for (version_index = 0; version_index < version->total_size; version_index++) {
        if (version_index != gentry->depth) {
          free_local(&(version->local_entry[version_index]));
          free_remote(&(version->remote_entry[version_index]));
        }
      }
      #endif
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
int __fenix_data_commit_cleanup(int groupid, int *timestamp) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  if (__fenix_options.verbose == 23) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);

    gentry->timestamp ++;
    if (timestamp != NULL) {
      *timestamp = gentry->timestamp;
    }

    fenix_member_t *member = gentry->member;
    int member_index;
    /* Iterate over current active members */
    for (member_index = 0; member_index < member->count; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      fenix_version_t *version = mentry->version;
      int version_index;
      #if 0
      /* Delete storage older than current timestamp + depth */
      for (version_index = 0; version_index < version->total_size; version_index++) {
        if (version_index != gentry->depth) {
          free_local(&(version->local_entry[version_index]));
          free_remote(&(version->remote_entry[version_index]));
        }
      }
      #endif
    }
    retval = FENIX_SUCCESS;
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
int __fenix_member_restore(int groupid, int memberid, void *data, int maxcount, int timestamp) {

  int retval =  FENIX_SUCCESS;
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery);
  int member_index = __fenix_search_memberid(group_index, memberid);

  if (__fenix_options.verbose == 25) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_restore: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {

    /* Lazy recovery is yet to be supported */
    /* Recover the data here  */
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    fenix_member_entry_t *mentry;

    /* This part is moved to group recovery and part of group entity */

    if (member_index != -1) { /* Member enrty is avalable */
      mentry = &(member->member_entry[member_index]);
    } else { /* Member entry is missing */
      member_index = member->count;
      mentry = &(member->member_entry[member->count]);
      member->count++;
    }

    int remote_status = 0;
    int current_status = mentry->state;
    int current_rank =   __fenix_get_current_rank(*__fenix_g_new_world);

    if (__fenix_options.verbose == 18 && __fenix_g_data_recovery->group_entry[group_index].current_rank== 0 ) {
       debug_print("mentry->state: %d; rank: %d\n", mentry->state, current_rank);
    }
    MPI_Status status;
    /* Check the role of the neighbor define by the group */
    MPI_Sendrecv(&current_status, 1, MPI_INT, gentry->out_rank, PARTNER_STATUS_TAG,
                 &remote_status, 1, MPI_INT, gentry->in_rank, PARTNER_STATUS_TAG,
                 (gentry->comm), &status);

    fenix_version_t *version = mentry->version;
    /* Send the member information if needed */
    if (current_status == OCCUPIED && remote_status == NEEDFIX) {
      _pc_send_member_metadata(gentry->current_rank, gentry->in_rank, mentry, gentry->comm);
      _pc_send_member_entries(gentry->current_rank, gentry->in_rank, gentry->depth, version,
                                gentry->comm);
    } else if (current_status == NEEDFIX && remote_status == OCCUPIED) {
      _pc_recover_member_metadata(gentry->current_rank, gentry->out_rank, mentry, gentry->comm);
      _pc_recover_member_entries(gentry->current_rank, gentry->out_rank, gentry->depth, version,
                                 gentry->comm);
    } else if ( current_status == NEEDFIX && remote_status == NEEDFIX) {
       debug_print("ERROR Fenix_Data_member_restore: member_id <%d> does not exist at %d\n",
                memberid, __fenix_g_data_recovery->group_entry[group_index].current_rank);
       retval = FENIX_ERROR_INVALID_MEMBERID;
    }

    
    /* Get the latest consistent copy */
    if (  __fenix_join_restore(gentry, version, gentry->comm) == 1 && retval == FENIX_SUCCESS) {
      //printf("POSITION %d\n",version->position - 1);
      fenix_local_entry_t *lentry = &(version->local_entry[version->position - 1]);
      lentry->pdata = data;
      mentry->current_datatype = lentry->datatype;
      mentry->current_count = lentry->count;
      mentry->current_size = lentry->datatype_size;
      memcpy(lentry->pdata, lentry->data, lentry->count * lentry->datatype_size);
      if (__fenix_options.verbose == 25) {
        verbose_print("c-rank: %d, role: %d, v-pos: %d, ld-totalsize: %d\n",
                        __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                      (version->position - 1), (lentry->count * lentry->datatype_size));
      }
      // member->status = __FENIX_MEMBER_FINE; 
      retval = FENIX_SUCCESS;
    } else {
      retval = FENIX_ERROR_GROUP_CREATE;
    }
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
  int retval = -1;
  fprintf(stderr,"Member_Restore_from_Rank is not yet supported\n");
  return retval;
}

/**
 * @brief
 * @param num_blocks
 * @param start_offset
 * @param end_offset
 * @param stride
 * @param subset_specifier
 *
 * This routine creates 
 */
int __fenix_data_subset_create(int num_blocks, int start_offset, int end_offset, int stride,
                       Fenix_Data_subset *subset_specifier) {
  int retval = -1;
  if (num_blocks <= 0) {
    debug_print("ERROR Fenix_Data_subset_create: num_blocks <%d> must be positive\n",
                num_blocks);
    retval = FENIX_ERROR_SUBSET_NUM_BLOCKS;
  } else if (start_offset < 0) {
    debug_print("ERROR Fenix_Data_subset_create: start_offset <%d> must be positive\n",
                start_offset);
    retval = FENIX_ERROR_SUBSET_START_OFFSET;
  } else if (end_offset <= 0) {
    debug_print("ERROR Fenix_Data_subset_create: end_offset <%d> must be positive\n",
                end_offset);
    retval = FENIX_ERROR_SUBSET_END_OFFSET;
  } else if (stride <= 0) {
    debug_print("ERROR Fenix_Data_subset_create: stride <%d> must be positive\n", stride);
    retval = FENIX_ERROR_SUBSET_STRIDE;
  } else {
    subset_specifier->start_offsets = (int *) s_malloc(sizeof(int));
    subset_specifier->end_offsets = (int *) s_malloc(sizeof(int));
    subset_specifier->num_blocks = num_blocks;
    subset_specifier->start_offsets[0] = start_offset;
    subset_specifier->end_offsets[0] = end_offset;
    subset_specifier->stride = stride;
    subset_specifier->specifier = __FENIX_SUBSET_CREATE;
    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 * @param num_blocks
 * @param array_start_offsets
 * @param array_end_offsets
 * @param subset_specifier
 */
int __fenix_data_subset_createv(int num_blocks, int *array_start_offsets, int *array_end_offsets,
                        Fenix_Data_subset *subset_specifier) {

  int retval = -1;
  if (num_blocks <= 0) {
    debug_print("ERROR Fenix_Data_subset_createv: num_blocks <%d> must be positive\n",
                num_blocks);
    retval = FENIX_ERROR_SUBSET_NUM_BLOCKS;
  } else if (array_start_offsets == NULL) {
    debug_print(
            "ERROR Fenix_Data_subset_createv: array_start_offsets %s must be at least of size 1\n",
            "");
    retval = FENIX_ERROR_SUBSET_START_OFFSET;
  } else if (array_end_offsets == NULL) {
    debug_print(
            "ERROR Fenix_Data_subset_createv: array_end_offsets %s must at least of size 1\n",
            "");
    retval = FENIX_ERROR_SUBSET_END_OFFSET;
  } else {

    // first check that the start offsets and end offsets are valid
    int index;
    int invalid_index = -1;
    int found_invalid_index = 0;
    for (index = 0; found_invalid_index != 1 && (index < num_blocks); index++) {
      if (array_start_offsets[index] > array_end_offsets[index]) {
        invalid_index = index;
        found_invalid_index = 1;
      }
    }

    if (found_invalid_index != 1) { // if not true (!= 1)
      subset_specifier->num_blocks = num_blocks;

      subset_specifier->start_offsets = (int *)s_malloc(sizeof(int)* num_blocks);
      memcpy(subset_specifier->start_offsets, array_start_offsets, ( num_blocks * sizeof(int))); // deep copy

      subset_specifier->end_offsets = (int *)s_malloc(sizeof(int)* num_blocks);
      memcpy(subset_specifier->end_offsets, array_end_offsets, ( num_blocks * sizeof(int))); // deep copy

      subset_specifier->specifier = __FENIX_SUBSET_CREATEV; // 
      retval = FENIX_SUCCESS;
    } else {
      debug_print(
              "ERROR Fenix_Data_subset_createv: array_end_offsets[%d] must be less than array_start_offsets[%d]\n",
              invalid_index, invalid_index);
      retval = FENIX_ERROR_SUBSET_END_OFFSET;
    }
  }
  return retval;
}

int __fenix_data_subset_free( Fenix_Data_subset *subset_specifier ) {
  int  retval = FENIX_SUCCESS;;
  free( subset_specifier->start_offsets );
  free( subset_specifier->end_offsets );
  subset_specifier->specifier = __FENIX_SUBSET_UNDEFINED;
  return retval;
}

/**
 * @brief
 * @param subset_specifier
 */
int __fenix_data_subset_delete( Fenix_Data_subset *subset_specifier ) {
  __fenix_data_subset_free(subset_specifier);
  free(subset_specifier);
  return FENIX_SUCCESS;
}

/**
 * @brief
 * @param group_id
 * @param num_members
 */
int __fenix_get_number_of_members(int group_id, int *num_members) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, __fenix_g_data_recovery );
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    *num_members = gentry->member->count;
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
  int group_index = __fenix_search_groupid(group_id, __fenix_g_data_recovery);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
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
  int group_index = __fenix_search_groupid(group_id, __fenix_g_data_recovery );
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    fenix_member_entry_t *mentry = &(member->member_entry[0]); // does not matter which member you get, every member has the same number of versions 
    fenix_version_t *version = mentry->version;
    *num_snapshots = version->count;
    retval = FENIX_SUCCESS;
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
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  if (__fenix_options.verbose == 33) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    *timestamp = gentry->timestamp - position;
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
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  int member_index = __fenix_search_memberid(group_index, memberid);
  if (__fenix_options.verbose == 34) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    __fenix_ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = mentry->version;
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);

    switch (attributename) {
      case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
        attributevalue = mentry->user_data;
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
        *((int *) (attributevalue)) = mentry->current_count;
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE:
        *((MPI_Datatype *)attributevalue) = mentry->current_datatype;
        retval = FENIX_SUCCESS;
        break;
#if 0
      case FENIX_DATA_MEMBER_ATTRIBUTE_SIZE:
        *((int *) (attributevalue)) = lentry->size;
        retval = FENIX_SUCCESS;
        break;
#endif
      default:
        debug_print("ERROR Fenix_Data_member_attr_get: invalid attribute_name <%d>\n",
                    attributename);
        retval = FENIX_ERROR_INVALID_ATTRIBUTE_NAME;
        break;
    }

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
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  int member_index = __fenix_search_memberid(group_index, memberid);
  if (__fenix_options.verbose == 35) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
  } else if (__fenix_g_role == 0) {
    debug_print("ERROR Fenix_Data_member_attr_set: cannot be called on role: <%s> \n",
                "FENIX_ROLE_INITIAL_RANK");
    retval = FENIX_ERROR_INVALID_LOGIC_CALL;
  } else {
    int my_datatype_size;
    int myerr;
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    __fenix_ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = mentry->version;
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);

    switch (attributename) {
      case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
        mentry->user_data = attributevalue;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
        mentry->current_count = *((int *) (attributevalue));
        lentry->count = *((int *) (attributevalue));
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
        lentry->datatype = *((MPI_Datatype *)(attributevalue));
        mentry->datatype_size = my_datatype_size;
        lentry->datatype_size = my_datatype_size;
        retval = FENIX_SUCCESS;
        break;
#if 0
        case FENIX_DATA_MEMBER_ATTRIBUTE_SIZE:
        lentry->size = *((int *) (attributevalue));
        retval = FENIX_SUCCESS;
        break;
#endif
      default:
        debug_print("ERROR Fenix_Data_member_attr_get: invalid attribute_name <%d>\n",
                    attributename);
        retval = FENIX_ERROR_INVALID_ATTRIBUTE_NAME;
        break;
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
int __fenix_snapshot_delete(int group_id, int time_stamp) {
  int retval = -1;
  int group_index = __fenix_search_groupid(group_id, __fenix_g_data_recovery );
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
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    int member_index;
    for (member_index = 0; member_index < member->count; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      fenix_version_t *version = mentry->version;

      /* Do not delete physically */
      if (time_stamp == FENIX_DATA_SNAPSHOT_LATEST) {
#if 1
        version->position = (version->position + gentry->depth -1 )% gentry->depth ;
        version->num_copies--;
        version->count--;
#else
        __fenix_data_version_shalow_delete_latest( version );
#endif

        /* free_local(&(version->local_entry[version->position])); */
      } else if (time_stamp == FENIX_DATA_SNAPSHOT_ALL) {
#if 1
        version->position = 0;
        version->num_copies = 0;
        version->count = 0;
     /*
         int version_index;
        for (version_index = 0; version_index < version->size; version_index++) {
          free_local(&(version->local_entry[version_index]));
        }
     */
#else
        __fenix_data_version_shalow_delete_all( version );
#endif
      } else {
        int version_index = time_stamp - gentry->timestart;
        __fenix_free_local(&(version->local_entry[version_index]));
      }
    }
    retval = FENIX_SUCCESS;
  }
  return retval;
}


/**
 * @brief
 * @param group_id
 */
int __fenix_group_delete(int groupid) {
  /******************************************/
  /* Commit needs to confirm the completion */
  /* of delete. Otherwise, it is reverted.  */
  /******************************************/
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );

  if (__fenix_options.verbose == 37) {
    verbose_print("c-rank: %d, group_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), group_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_group_delete: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    /* Delete Process */
    fenix_group_t *group = __fenix_g_data_recovery;
    group->count--;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->depth = 0;
    gentry->groupid = -1;
    gentry->timestamp = -1;
    gentry->state = DELETED;

    if (__fenix_options.verbose == 37) {
      verbose_print(
              "c-rank: %d, g-count: %d, g-size: %d, g-depth: %d, g-groupid: %d, g-timestamp: %d, g-state: %d\n",
                __fenix_get_current_rank(*__fenix_g_new_world), group->count, group->total_size,
              gentry->depth, gentry->groupid,
              gentry->timestamp, gentry->state);
    }

    fenix_member_t *member = gentry->member;
    member->count = 0; /* Logical counter for member */

    /* Free-up members */
    int member_index;
    for (member_index = 0; member_index < member->total_size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      mentry->memberid = -1;
      mentry->state = DELETED;
      fenix_version_t *version = mentry->version;
      version->count = 0;

      if (__fenix_options.verbose == 37) {
        verbose_print(
                "c-rank: %d, member[%d] m-count: %d, m-size: %d, m-memberid: %d, m-state: %d, v-count: %d\n",
                  __fenix_get_current_rank(*__fenix_g_new_world), member_index, member->count,
                member->total_size, mentry->memberid,
                mentry->state, version->count);
      }

      /* Free-up versions */
      int verison_index;
      for (verison_index = 0; verison_index < version->total_size; verison_index++) {
        __fenix_free_local(&(version->local_entry[version->position]));
        __fenix_free_remote(&(version->remote_entry[version->position]));
      }
    }
    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 */
int __fenix_member_delete(int groupid, int memberid) {
  int retval = -1;
  int group_index = __fenix_search_groupid(groupid, __fenix_g_data_recovery );
  int member_index = __fenix_search_memberid(group_index, memberid);

  if (__fenix_options.verbose == 38) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    fenix_group_t *group = __fenix_g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = gentry->member;
    member->count--;
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->state = DELETED;
    fenix_version_t *version = mentry->version;
    version->count = 0;

    if (__fenix_options.verbose == 38) {
      verbose_print("c-rank: %d, role: %d, m-count: %d, m-state: %d, v-count: %d\n",
                      __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                    member->count, mentry->state,
                    version->count);
    }

    int verison_index;
    for (verison_index = 0; verison_index < version->total_size; verison_index++) {
      __fenix_free_local(&(version->local_entry[verison_index]));
      __fenix_free_remote(&(version->remote_entry[verison_index]));
    }
    retval = FENIX_SUCCESS;
  }
  return retval;
}






/**
 * @brief
 */
void __fenix_free_local(fenix_local_entry_t *l) {
  fenix_local_entry_t *local = l;
  local->currentrank = -1;
  local->pdata = NULL;
  if (local->data != NULL) {
    free(local->data);
  }
  local->count = 0;
  local->datatype_size = 0;
  local->datatype = NULL;

  if (__fenix_options.verbose == 46) {
    verbose_print(
            "c-rank: %d, role: %d, ld-currentrank: %d, ld-count: %d, ld-size: %d\n",
              __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
            local->currentrank, local->count, local->datatype_size);
  }

}

/**
 * @brief
 * @param
 */
void __fenix_free_remote(fenix_remote_entry_t *r) {
  fenix_remote_entry_t *remote = r;
  remote->remoterank = -1;
  remote->pdata = NULL;
  if (remote->data == NULL) {
    free(remote->data);
  }
  remote->count = 0;
  remote->datatype_size = 0;
  remote->datatype = NULL;

  if (__fenix_options.verbose == 47) {
    verbose_print(
            "c-rank: %d, role: %d, rd-remoterank: %d, rd-count: %d, rd-size: %d\n",
              __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
            remote->remoterank, remote->count, remote->datatype_size);
  }

}










/**
 * @brief
 * @param
 * @param
 */
int __fenix_search_memberid(int group_index, int key) {
  fenix_group_t *group = __fenix_g_data_recovery;
  fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
  fenix_member_t *member = gentry->member;
  int member_index, found = -1, index = -1;
  for (member_index = 0;
       (found != 1) && (member_index < member->total_size); member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    if (key == mentry->memberid) {
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
int __fenix_find_next_member_position(fenix_member_t *m) {
  fenix_member_t *member = m;
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

/**
 * @brief
 * @param
 * @param
 */
int __fenix_join_group(fenix_group_t *g, fenix_group_entry_t *ge, MPI_Comm comm) {
  fenix_group_t *group = g;
  fenix_group_entry_t *gentry = ge;
  int current_rank_attributes[__GROUP_ENTRY_ATTR_SIZE];
  int other_rank_attributes[__GROUP_ENTRY_ATTR_SIZE];
  current_rank_attributes[0] = group->count;
  current_rank_attributes[1] = gentry->groupid;
  current_rank_attributes[2] = gentry->timestamp;
  current_rank_attributes[3] = gentry->depth;
  current_rank_attributes[4] = gentry->state;

  if (__fenix_options.verbose == 58) {
    verbose_print(
            "c-rank: %d, g-count: %d, g-groupid: %d, g-timestamp: %d, g-depth: %d, g-state: %d\n",
              __fenix_get_current_rank(*__fenix_g_new_world), group->count, gentry->groupid,
            gentry->timestamp, gentry->depth, gentry->state);
  }

  MPI_Allreduce(current_rank_attributes, other_rank_attributes, __GROUP_ENTRY_ATTR_SIZE,
                MPI_INT, __fenix_g_agree_op, comm);

  int found = -1, index;
  for (index = 0; found != 1 && (index < __GROUP_ENTRY_ATTR_SIZE); index++) {
    if (current_rank_attributes[index] !=
        other_rank_attributes[index]) { // all ranks did not agree! 
      switch (index) {
        case 0:
          debug_print("ERROR ranks did not agree on g-count: %d\n",
                      current_rank_attributes[0]);
          break;
        case 1:
          debug_print("ERROR ranks did not agree on g-groupid: %d\n",
                      current_rank_attributes[1]);
          break;
        case 2:
          debug_print("ERROR ranks did not agree on g-timestamp: %d\n",
                      current_rank_attributes[2]);
          break;
        case 3:
          debug_print("ERROR ranks did not agree on g-depth: %d\n",
                      current_rank_attributes[3]);
          break;
        case 4:
          debug_print("ERROR ranks did not agree on g-state: %d\n",
                      current_rank_attributes[4]);
          break;
        default:
          break;
      }
      found = 1;
    }
  }
  return found;
}

/**
 * @brief
 * @param
 * @param
 */
int __fenix_join_member(fenix_member_t *m, fenix_member_entry_t *me, MPI_Comm comm) {
  fenix_member_t *member = m;
  fenix_member_entry_t *mentry = me;
  int current_rank_attributes[__NUM_MEMBER_ATTR_SIZE];
  current_rank_attributes[0] = member->count;
  current_rank_attributes[1] = mentry->memberid;
  current_rank_attributes[2] = mentry->state;
  int other_rank_attributes[__NUM_MEMBER_ATTR_SIZE];
  MPI_Allreduce(current_rank_attributes, other_rank_attributes, __NUM_MEMBER_ATTR_SIZE,
                MPI_INT, __fenix_g_agree_op, comm);
  int found = -1, index;
  for (index = 0; found != 1 && (index < __NUM_MEMBER_ATTR_SIZE); index++) {
    if (current_rank_attributes[index] != other_rank_attributes[index]) {
      switch (index) {
        case 0:
          debug_print("ERROR ranks did not agree on m-count: %d\n",
                      current_rank_attributes[0]);
          break;
        case 1:
          debug_print("ERROR ranks did not agree on m-memberid: %d\n",
                      current_rank_attributes[1]);
          break;
        case 2:
          debug_print("ERROR ranks did not agree on m-state: %d\n",
                      current_rank_attributes[2]);
          break;
        default:
          break;
      }
      found = 1;
    }
  }
  return found;
}

/**
 * @brief
 * @param
 * @param
 */
int __fenix_join_restore(fenix_group_entry_t *ge, fenix_version_t *v, MPI_Comm comm) {
  int current_rank_attributes[2];
  int other_rank_attributes[2];
  int found = -1;

/* Find the minimum timestamp among the ranks */
  int min_timestamp, idiff;
  MPI_Allreduce(&(ge->timestamp), &min_timestamp, 1, MPI_INT, MPI_MIN, comm);

  idiff = ge->timestamp - min_timestamp;
  if ((min_timestamp > (ge->timestamp - ge->depth))
      && (idiff < v->num_copies)) {
    /* Shift the position of the latest version */
    v->position = (v->position + v->total_size - idiff) % (v->total_size);
    found = 1;
  } else {
    found = -1;
  }

  /* Check if every rank finds the copy */
  int result;
  MPI_Allreduce(&found, &result, 1, MPI_INT, MPI_MIN, comm);

  found = result;
  return found;
}

/**
 * @brief
 * @param
 * @param
 */
int __fenix_join_commit( fenix_group_entry_t *ge, fenix_version_t *v, MPI_Comm comm) {
  int found = -1;
  int min_timestamp;
  MPI_Allreduce(&(ge->timestamp), &min_timestamp, 1, MPI_INT, MPI_MIN, comm);

  int timestamp_offset = ge->timestamp - min_timestamp;
  int depth_offest = (ge->timestamp - ge->depth);
  if ((min_timestamp > depth_offest) && (timestamp_offset < v->num_copies)) {
    v->position = (v->position + (v->total_size - timestamp_offset)) % (v->total_size);
    found = 1;
  } else {
    found = -1;
  }

  int result;
  MPI_Allreduce(&found, &result, 1, MPI_INT, MPI_MIN, comm);
  found = result;
  return found;
}

///////////////////////////////////////////////////// TODO //

void __fenix_store_single() {


}

/**
 *
 */
void __feninx_dr_print_store() {
  int group, member, version, local, remote;
  fenix_group_t *current = __fenix_g_data_recovery;
  int group_count = current->count;
  for (group = 0; group < group_count; group++) {
    int member_count = current->group_entry[group].member->count;
    for (member = 0; member < member_count; member++) {
      int version_count = current->group_entry[group].member->member_entry[member].version->count;
      for (version = 0; version < version_count; version++) {
        int local_data_count = current->group_entry[group].member->member_entry[member].version->local_entry[version].count;
        int *local_data = current->group_entry[group].member->member_entry[member].version->local_entry[version].data;
        for (local = 0; local < local_data_count; local++) {
          //printf("*** store rank[%d] group[%d] member[%d] local[%d]: %d\n",
          //get_current_rank(*__fenix_g_new_world), group, member, local,
          //local_data[local]);
        }
        int remote_data_count = current->group_entry[group].member->member_entry[member].version->remote_entry[version].count;
        int *remote_data = current->group_entry[group].member->member_entry[member].version->remote_entry[version].data;
        for (remote = 0; remote < remote_data_count; remote++) {
          printf("*** store rank[%d] group[%d] member[%d] remote[%d]: %d\n",
                   __fenix_get_current_rank(*__fenix_g_new_world), group, member, remote,
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
  fenix_group_t *current = __fenix_g_data_recovery;
  int group_count = current->count;
  int member_count = current->group_entry[0].member->count;
  int version_count = current->group_entry[0].member->member_entry[0].version->count;
  int local_data_count = current->group_entry[0].member->member_entry[0].version->local_entry[0].count;
  int remote_data_count = current->group_entry[0].member->member_entry[0].version->remote_entry[0].count;
  printf("*** restore rank: %d; group: %d; member: %d; local: %d; remote: %d\n",
           __fenix_get_current_rank(*__fenix_g_new_world), group_count, member_count,
         local_data_count,
         remote_data_count);
}

/**
 *
 */
void __fenix_dr_print_datastructure() {
  int group_index, member_index, version_index, remote_data_index, local_data_index;
  fenix_group_t *current = __fenix_g_data_recovery;

  if (!current) {
    return;
  }

  printf("\n\ncurrent_rank: %d\n",   __fenix_get_current_rank(*__fenix_g_new_world));
  int group_size = current->total_size;
  for (group_index = 0; group_index < group_size; group_index++) {
    int depth = current->group_entry[group_index].depth;
    int groupid = current->group_entry[group_index].groupid;
    int timestamp = current->group_entry[group_index].timestamp;
    int group_state = current->group_entry[group_index].state;
    int member_size = current->group_entry[group_index].member->total_size;
    int member_count = current->group_entry[group_index].member->count;
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
      int memberid = current->group_entry[group_index].member->member_entry[member_index].memberid;
      int member_state = current->group_entry[group_index].member->member_entry[member_index].state;
      int version_size = current->group_entry[group_index].member->member_entry[member_index].version->total_size;
      int version_count = current->group_entry[group_index].member->member_entry[member_index].version->count;
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
        int local_data_count = current->group_entry[group_index].member->member_entry[member_index].version->local_entry[version_index].count;
        printf("group[%d] member[%d] version[%d] local_data.count: %d\n",
               group_index,
               member_index,
               version_index, local_data_count);
        if (current->group_entry[group_index].member->member_entry[member_index].version->local_entry[version_index].data !=
            NULL) {
          int *current_local_data = (int *) current->group_entry[group_index].member->member_entry[member_index].version->local_entry[version_index].data;
          for (local_data_index = 0; local_data_index < local_data_count; local_data_index++) {
            printf("group[%d] member[%d] depth[%d] local_data[%d]: %d\n",
                   group_index,
                   member_index,
                   version_index, local_data_index,
                   current_local_data[local_data_index]);
          }
        }

        int remote_data_count = current->group_entry[group_index].member->member_entry[member_index].version->remote_entry[version_index].count;
        printf("group[%d] member[%d] version[%d] remote_data.count: %d\n",
               group_index,
               member_index, version_index, remote_data_count);
        if (current->group_entry[group_index].member->member_entry[member_index].version->remote_entry[version_index].data !=
            NULL) {
          int *current_remote_data = current->group_entry[group_index].member->member_entry[member_index].version->remote_entry[version_index].data;
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
