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


#include "constants.h"
#include "data_recovery.h"
#include "opt.h"
#include "process_recovery.h"
#include "util.h"

extern int *rank_roles;
extern struct opt *options;

/**
 * @brief           create new group or recover group data for lost processes
 * @param groud_id  
 * @param comm
 * @param time_stamp
 * @param depth
 */
int group_create(int groupid, MPI_Comm comm, int timestamp, int depth) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  if (options->verbose == 12) {
    verbose_print("c-rank: %d, group_index: %d\n", get_current_rank(*__fenix_g_new_world), group_index);
  }
  if (timestamp < 0) {
    debug_print("ERROR Fenix_Data_group_create: time_stamp <%d> must be greater than or equal to zero\n", timestamp);
    retval = FENIX_ERROR_INVALID_TIMESTAMP;
  } else if (depth < -1) {
    debug_print("ERROR Fenix_Data_group_create: depth <%d> must be greater than or equal to -1\n",depth);
    retval = FENIX_ERROR_INVALID_DEPTH;
  } else {

    /* This code block checks the need for data recovery. If so, recover the data and set the recovery    */
    /* for member recovery.                                                                               */

    int i, its_new = 1, group_position;
    int remote_need_recovery;
    fenix_group_entry_t *gentry;
    MPI_Status status;

    /* If the group data is already here. */
    fenix_group_t *group = g_data_recovery;
    ensure_group_capacity(group);
    for (i = 0; i < group->count; i++) {
      if (groupid == group->group_entry[i].groupid) {
        group_position = i;
        its_new = 0; /* already created */
        break;
      }
    }


    /* Initialize Group */
    if (its_new == 1) {
      
      if (options->verbose == 12 && get_current_rank(comm) == 0) {
         printf("this is a new group!\n"); 
      }

      group->count++;
      int next_group_position = find_next_group_position(group);
      gentry = &(group->group_entry[next_group_position]);
      gentry->groupid = groupid;

      gentry->comm = comm;

      gentry->timestart = timestamp;
      gentry->timestamp = timestamp;
      gentry->depth = depth + 1;
      gentry->state = OCCUPIED;
      if (__fenix_g_role == FENIX_ROLE_RECOVERED_RANK) {
        gentry->recovered = 1;
      } else {
        gentry->recovered = 0;
      }
      if (get_world_size(*__fenix_g_new_world) / 4 > 1) {
        gentry->rank_separation = get_world_size(*__fenix_g_new_world) / 4;
      } else {
        gentry->rank_separation = 1;
      }

      if (options->verbose == 12) {
        verbose_print(
                "c-rank: %d, g-groupid: %d, g-timestamp: %d, g-depth: %d, g-state: %d\n",
                get_current_rank(*__fenix_g_new_world), gentry->groupid,
                gentry->timestamp,
                gentry->depth,
                gentry->state);
      }
    } else { /* Already created. Renew the MPI communicator  */
      gentry = &(group->group_entry[group_position]);
      gentry->comm = comm; /* Renew communicator */
    }

    gentry->current_rank = get_current_rank(gentry->comm);
    gentry->comm_size = get_world_size(gentry->comm);
    gentry->in_rank = (gentry->current_rank + gentry->comm_size - gentry->rank_separation) % gentry->comm_size;
    gentry->out_rank = (gentry->current_rank + gentry->comm_size + gentry->rank_separation) % gentry->comm_size;

    /* Check the role of the neighbor define by the group */
    MPI_Sendrecv(&(gentry->recovered), 1, MPI_INT, gentry->out_rank, PARTNER_STATUS_TAG,
                 &remote_need_recovery, 1, MPI_INT, gentry->in_rank, PARTNER_STATUS_TAG,
                 (gentry->comm), &status);

    /* Recover group information */
    if (gentry->recovered == 0 && remote_need_recovery == 1) {
      retval = _send_metadata(gentry->current_rank, gentry->in_rank, gentry->comm);
      retval = _send_group_data(gentry->current_rank, gentry->in_rank, gentry, gentry->comm);
    } else if (gentry->recovered == 1 && remote_need_recovery == 0) {
      if (options->verbose == 18) {
        verbose_print(
                "c-rank: %d receiving group data %d from rank %d\n",
                get_current_rank(*__fenix_g_new_world), gentry->groupid,
                gentry->out_rank);
      }
      retval = _recover_metadata(gentry->current_rank, gentry->out_rank, comm);
      retval = _recover_group_data(gentry->current_rank, gentry->out_rank, gentry, gentry->comm);
      /* Recovery is done. Change the flag */
      gentry->recovered = 0;
    }

    /* Need Error check for retval above */
    /* ********************************* */


    /* Global agreement among the group */
    retval = (join_group(group, gentry, comm) != 1) ? FENIX_SUCCESS
                                                    : FENIX_ERROR_GROUP_CREATE;
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
int member_create(int groupid, int memberid, void *data, int count,
                  MPI_Datatype datatype) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = search_memberid(group_index, memberid);

  if (options->verbose == 13) {
    verbose_print("c-rank: %d, group_index: %d, member_index: %d\n",
                  get_current_rank(*__fenix_g_new_world),
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

    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);

    /* Check if the entry needs to be reallocated */
    ensure_member_capacity(member);

    member->count++;

    int next_member_position = find_next_member_position(member);

    if (options->verbose == 13) {
      verbose_print("c-rank: %d, next_member_position: %d\n",
                    get_current_rank(*__fenix_g_new_world),
                    next_member_position);
    }

    fenix_member_entry_t *mentry = &(member->member_entry[next_member_position]);
    mentry->memberid = memberid;
    mentry->state = OCCUPIED;
    mentry->current_buf = data;
    mentry->current_count = count;
    mentry->current_datatype = datatype;
    int dsize;
    MPI_Type_size(datatype, &dsize);

    mentry->size_datatype = mentry->current_size = dsize;

    fenix_version_t *version = &(member->member_entry[next_member_position].version);

    /* Check the size of data in the partner */
    /* We may need to send the data type at remote rank */
    MPI_Sendrecv(&count, 1, MPI_INT, gentry->out_rank, STORE_DATA_TAG + 1,
                 &remote_count, 1, MPI_INT, gentry->in_rank, STORE_DATA_TAG + 1,
                 gentry->comm, &status);


    /* Initalize the space for every single version           */
    /* Eric, this may conflict what you have done with SUBSET */
    for (i = 0; i < gentry->depth; i++) {
      fenix_local_entry_t *lentry = &(version->local_entry[i]);
      lentry->data = (void *) s_malloc(mentry->size_datatype * count);
      lentry->count = count;
      lentry->size = mentry->size_datatype;
      lentry->datatype = datatype;
      /* Bind the user data to Fenix */
      lentry->pdata = data; /* This should be handled by member entry rathewr than version entry */

      fenix_remote_entry_t *rentry = &(version->remote_entry[i]);
      rentry->data = (void *) s_malloc(mentry->size_datatype * remote_count);
      rentry->pdata = NULL;
    }

    /* Calling global agreement */
    retval = (join_member(member, mentry, gentry->comm) != 1) ? FENIX_SUCCESS
                                                              : FENIX_ERROR_GROUP_CREATE;
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
int group_get_redundancy_policy(int group_id, int policy_name, void *policy_value,
                                int *flag) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  if (group_index == -1) {
    debug_print(
            "ERROR Fenix_Data_group_get_redundancy_policy: group_id <%d> does not exist\n",
            group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    // *policy_value = gentry->policy_value;

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
int group_set_redundancy_policy(int group_id, int policy_name, void *policy_value,
                                int *flag) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  if (group_index == -1) {
    debug_print(
            "ERROR Fenix_Data_group_set_redundancy_policy: group_id <%d> does not exist\n",
            group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    switch (policy_name) {
      case FENIX_DATA_POLICY_PEER_RANK_SEPARATION:
        /* We need an index to record the information on policy values */
        memcpy(&(gentry->rank_separation), policy_value, sizeof(int));
        retval = FENIX_SUCCESS;
        *flag = FENIX_SUCCESS;
        break;
      default:
        debug_print(
                "ERROR Fenix_Data_group_get_redundancy_policy: the specified policyis not suppored by group_id <%d>\n",
                group_id);
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
int data_wait(Fenix_Request request) {
  int retval = -1;
  int result = wait(&(request.mpi_recv_req));
  if (result != MPI_SUCCESS) {
    retval = FENIX_SUCCESS;
  } else {
    retval = FENIX_ERROR_DATA_WAIT;
  }
  result = wait(&(request.mpi_send_req));
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
int data_test(Fenix_Request request, int *flag) {
  int retval = -1;
  int result = ( _f_test(&(request.mpi_recv_req)) & _f_test(&(request.mpi_send_req))) ;

  if ( result == 1 ) {
    *flag = 1;
    retval = FENIX_SUCCESS;
  } else {
    *flag = 0 ; // incomplete error?
    retval = FENIX_ERROR_DATA_WAIT;
  }
  return retval;
}

/**
 * @brief // TODO: implement FENIX_DATA_MEMBER_ALL
 * @param group_id
 * @param member_id
 * @param subset_specifier
 * Still some bugs (9/1/16)
 */

/*
 *  note: this is covering store_v routine
 */
int member_store(int groupid, int memberid, Fenix_Data_subset specifier) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = -1;

  /* check if member id alreay exists */
  if (memberid != FENIX_DATA_MEMBER_ALL) {
    member_index = search_memberid(group_index, memberid);
  }

  if (options->verbose == 18 && g_data_recovery->group_entry[group_index].current_rank== 0 ) {
    verbose_print(
            "c-rank: %d, role: %d, group_index: %d, member_index: %d memberid: %d\n",
            get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
            member_index, memberid);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_store: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_store: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else if (specifier.specifier == __FENIX_SUBSET_EMPTY) { // no need to store anything
    retval = FENIX_SUCCESS;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    
    ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]); /* select the latest local data */
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]); /* select the latest remote data */

    /* store the local data */
    memcpy(lentry->data, lentry->pdata, (lentry->count * lentry->size));

    fenix_subset_offsets_t *loffsets = NULL, *roffsets = NULL;
    char *send_buff, *recv_buff;

    /* note: the following data exchange is not necessary when using non-v call */
    /* processes need to exchange metadata info for data recovery */

    MPI_Status status;
    member_store_packet_t lentry_packet, rentry_packet;
    lentry_packet.rank = lentry->currentrank;
    lentry_packet.datatype = lentry->datatype;
    lentry_packet.entry_count = lentry->count;
    lentry_packet.entry_size  = lentry->size;

    if(specifier.specifier == __FENIX_SUBSET_FULL) {
      lentry_packet.entry_real_count  = lentry->count;
      lentry_packet.num_blocks  = 0;  
      send_buff = (char *) lentry->data;
    } else {
      char *ldata = (char *) lentry->data; // where is this local being used?

      lentry_packet.num_blocks  = specifier.num_blocks;
      loffsets = (fenix_subset_offsets_t *) s_malloc(sizeof(fenix_subset_offsets_t) * specifier.num_blocks);

      /* count buffer storage size */
      int block_index;
      int subset_element_count = 0;
      for (block_index = 0; block_index < specifier.num_blocks; block_index++) {
        loffsets[block_index].start = specifier.start_offsets[block_index];
        loffsets[block_index].end = specifier.end_offsets[block_index];
        subset_element_count += (specifier.end_offsets[block_index] - specifier.start_offsets[block_index]) + 1;
      }      

      send_buff = (char *) s_malloc(sizeof(char) * subset_element_count * lentry->size);


      /* pack the data into buffer */
      int index;
      int offset_index = 0;
      for (index = 0; index < specifier.num_blocks; index++) {
        int n = (loffsets[index].end - loffsets[index].start) + 1;
        memcpy((void *) &send_buff[offset_index * lentry->size], (void *) &ldata[loffsets[index].start * lentry->size], n * lentry->size);
        offset_index += n;
      }      

      /*   */
      lentry_packet.entry_real_count = subset_element_count; 

    }

    int current_role = __fenix_g_role;
    MPI_Sendrecv(&lentry_packet, sizeof(member_store_packet_t), MPI_BYTE, gentry->out_rank,
               STORE_SIZE_TAG, &rentry_packet, sizeof(member_store_packet_t), MPI_BYTE,
               gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status);

    rentry->remoterank = rentry_packet.rank;
    rentry->datatype = rentry_packet.datatype;
    rentry->count = rentry_packet.entry_count;
    rentry->size = rentry_packet.entry_size;

    if (rentry->data != NULL) {
      rentry->data = s_malloc(rentry->count * rentry->size);
    }

    /* 
     * After sending member entry information, the use of subset is checked
     * case 1: Left neighbor and I use subset  
     * case 2: Left neighbor does not use subset, but I use subset
     * case 3: Left neighbor uses subset, but I do not  
     * case 4: Left neighbor is not using subset.  Me either
     */
    if(rentry_packet.num_blocks > 0 && lentry_packet.num_blocks > 0) { 
      /* exchange subset info */
      roffsets = (fenix_subset_offsets_t *)s_malloc(sizeof(fenix_subset_offsets_t) * rentry_packet.num_blocks);
      MPI_Sendrecv(loffsets, sizeof(fenix_subset_offsets_t)*lentry_packet.num_blocks,
                   MPI_BYTE, gentry->out_rank, STORE_SIZE_TAG, roffsets,
                   sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks, MPI_BYTE,
                   gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status);
      /* temporary buffer to received packed data */
      recv_buff = (char *)s_malloc(sizeof(char)* rentry_packet.entry_real_count * rentry->size);
    } else if (lentry_packet.num_blocks > 0) {
      MPI_Send(&loffsets,  sizeof(fenix_subset_offsets_t)*lentry_packet.num_blocks, 
               MPI_BYTE, gentry->out_rank, STORE_SIZE_TAG,  (gentry->comm));

      /* sender to me is not using subset */
      recv_buff = (char *) rentry->data;
    } else if (rentry_packet.num_blocks > 0) {
      roffsets = (fenix_subset_offsets_t *)s_malloc(sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks);
      MPI_Recv(&roffsets, sizeof(fenix_subset_offsets_t)*rentry_packet.num_blocks, MPI_BYTE, 
               gentry->in_rank, STORE_SIZE_TAG, (gentry->comm), &status );
      /* temporary buffer to received packed data */
      recv_buff = (char *) s_malloc(sizeof(char)* rentry_packet.entry_real_count * rentry->size );
    } else {

      /* sender to me is not using subset */
      recv_buff = rentry->data;
    }

    /* exchange the payload  */
    MPI_Sendrecv( (void *)send_buff, (lentry_packet.entry_real_count * lentry->size), MPI_BYTE, gentry->out_rank,
                  STORE_PAYLOAD_TAG, (void *) recv_buff, (rentry_packet.entry_real_count * rentry->size), MPI_BYTE,
                  gentry->in_rank, STORE_PAYLOAD_TAG, gentry->comm, &status);

    if (lentry_packet.num_blocks > 0) {
      free(loffsets); 
      free(send_buff); 
    }

    if (rentry_packet.num_blocks > 0) {
      /* re-organize received data */
      int i, j = 0;
      char * rdata = (char *) rentry->data;
      for (i = 0; i < rentry_packet.num_blocks; i++) {
        int n = (roffsets[i].end - roffsets[i].start) + 1;
        memcpy((void*) &rdata[roffsets[i].start * rentry->size], (void *) &recv_buff[j*rentry->size], n * rentry->size);
        j += n;
      }
      free(roffsets); 
      free(recv_buff); 
    }

    /* Need to update the version info */
    if (version->position < version->size - 1) {
      version->num_copies++;
      version->position++;
    } else {
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
int member_istore(int groupid, int memberid, Fenix_Data_subset specifier,
                  Fenix_Request *request) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = -1;
  if (memberid != FENIX_DATA_MEMBER_ALL) {
    member_index = search_memberid(group_index, memberid);
  }

  if (options->verbose == 18) {
    verbose_print(
            "c-rank: %d, role: %d, group_index: %d, member_index: %d memberid: %d\n",
            get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    ensure_version_capacity(member);
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
    lentry_packet.entry_size = lentry->size;
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
  return retval;
}

/*
 *
 *
 */
fenix_local_entry_t *subset_full(fenix_member_entry_t *me) {
  fenix_local_entry_t *lentry = (fenix_local_entry_t *) s_malloc(
          sizeof(fenix_local_entry_t));
  lentry->currentrank = me->currentrank;
  lentry->count = me->current_count;
  lentry->datatype = me->current_datatype;
  lentry->pdata = me->current_buf;
  lentry->size = sizeof(me->current_buf);
  lentry->data = s_malloc(
          lentry->count * lentry->size); /* Why do you allocate space here? */
  memcpy(lentry->data, lentry->pdata, (lentry->count * lentry->size));
  return lentry;
}

void subset(fenix_group_entry_t *ge, fenix_member_entry_t *me, Fenix_Data_subset *ss) {
#if 0
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

  int current_rank = get_current_rank(*__fenix_g_new_world);
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

fenix_local_entry_t *subset_variable(fenix_member_entry_t *me, Fenix_Data_subset *ss) {
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
      memcpy((ss_data) + ss_index, (me->current_buf) + data_index,
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
}



/**
 * @brief
 * @param group_id
 * @param member_id
 * @param subset_specifier
 */
int member_storev(int group_id, int member_id, Fenix_Data_subset subset_specifier) {

/*
 * Using the same routine for v and non-v routine.
 */
  int retval = -1;
  int group_index = search_groupid(group_id);
  int member_index = search_memberid(group_index, member_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_storev: group_id <%d> does not exist\n",
                group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_storev: member_id <%d> does not exist\n",
                member_id);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);
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
int member_istorev(int group_id, int member_id, Fenix_Data_subset subset_specifier,
                   Fenix_Request *request) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  int member_index = search_memberid(group_index, member_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_istorev: group_id <%d> does not exist\n",
                group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_istorev: member_id <%d> does not exist\n",
                member_id);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version->position]);
    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 * @param group_id
 * @param time_stamp
 */
int data_commit(int groupid, int *timestamp) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  if (options->verbose == 22) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n", get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
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
int data_commit_barrier(int groupid, int *timestamp) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  if (options->verbose == 23) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);

    if (timestamp != NULL) {
      *timestamp = gentry->timestamp;
    }

    fenix_member_t *member = &(gentry->member);
    int member_index;
    for (member_index = 0; member_index < member->size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      fenix_version_t *version = &(mentry->version);
      int version_index;
      for (version_index = 0; version_index < version->size; version_index++) {
        if (version_index != version->position) {
          free_local(&(version->local_entry[version_index]));
          free_remote(&(version->remote_entry[version_index]));
        }
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
int data_barrier(int group_id) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_barrier: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    int member_index;
    for (int member_index = 0; member_index < member->size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      fenix_version_t *version = &(mentry->version);
      int version_index;
      for (version_index = 0; version_index < version->size; version_index++) {
        if (version_index != version->position) {
          free_local(&(version->local_entry[version_index]));
          free_remote(&(version->remote_entry[version_index]));
        }
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
 * @param data
 * @param max_count
 * @param time_stamp
 */
int member_restore(int groupid, int memberid, void *data, int maxcount, int timestamp) {
  int retval =  FENIX_SUCCESS;
  int group_index = search_groupid(groupid);
  int member_index = search_memberid(group_index, memberid);

  if (options->verbose == 25) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_restore: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
#if 0
  } else if (member_index == -1) {
    /* Recovered Process Does not have member data at all */
    /* Need special information */
    debug_print("ERROR Fenix_Data_member_restore: member_id <%d> does not exist at %d\n",
                memberid,g_data_recovery->group_entry[group_index].current_rank);
    retval = FENIX_ERROR_INVALID_MEMBERID;
#endif
  } else {

    /* Lazy recovery is yet to be supported */
    /* Recover the data here  */
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
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
    int current_rank = get_current_rank(*__fenix_g_new_world);

    if (options->verbose == 18 && g_data_recovery->group_entry[group_index].current_rank== 0 ) {
       debug_print("mentry->state: %d; rank: %d\n", mentry->state, current_rank);
    }
    MPI_Status status;
    /* Check the role of the neighbor define by the group */
    MPI_Sendrecv(&current_status, 1, MPI_INT, gentry->out_rank, PARTNER_STATUS_TAG,
                 &remote_status, 1, MPI_INT, gentry->in_rank, PARTNER_STATUS_TAG,
                 (gentry->comm), &status);

    fenix_version_t *version = &(mentry->version);
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
                memberid,g_data_recovery->group_entry[group_index].current_rank);
       retval = FENIX_ERROR_INVALID_MEMBERID;
    }

    
    /* Get the latest consistent copy */
    if (  join_restore(gentry, version, gentry->comm) == 1 && retval == FENIX_SUCCESS) {
      //printf("POSITION %d\n",version->position - 1);
      fenix_local_entry_t *lentry = &(version->local_entry[version->position - 1]);
      lentry->pdata = data;
      mentry->current_datatype = lentry->datatype;
      mentry->current_count = lentry->count;
      mentry->current_size = lentry->size;
      memcpy(lentry->pdata, lentry->data, lentry->count * lentry->size);
      if (options->verbose == 25) {
        verbose_print("c-rank: %d, role: %d, v-pos: %d, ld-totalsize: %d\n",
                      get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                      (version->position - 1), (lentry->count * lentry->size));
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
int member_restore_from_rank(int groupid, int memberid, void *target_buffer,
                             int max_count, int time_stamp, int source_rank) {
#if 1
  int retval = -1;
  fprintf(stderr,"Member_Restore_from_Rank is not yet supported\n");
#else 
  /* Need to change                               */
  /* The data is retrieved from the specific rank */
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = search_memberid(group_index, memberid);
  int current_rank = get_current_rank(*__fenix_g_new_world);
  int total_ranks = get_world_size(*__fenix_g_new_world);

 if (options->verbose == 25) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_restore_from_rank: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else if (member_index == -1) {
    debug_print("ERROR Fenix_Data_member_restore_from_rank: member_id <%d> does not exist\n",
                memberid);
    retval = FENIX_ERROR_INVALID_MEMBERID;
  } else if (__fenix_g_role == FENIX_ROLE_INITIAL_RANK) {
    debug_print("ERROR Fenix_Data_member_restore_from_rank: must be called by RECOVERED or SURVIVOR rank %s\n", "");
    retval = -1;
  } else if (source_rank > (total_ranks - 1) || source_rank < 0) {
    debug_print("ERROR Fenix_Data_member_restore_from_rank: invalid source_rank <%d> \n", source_rank);
    retval = -1;
  } else if (current_rank == source_rank && __fenix_g_role == FENIX_ROLE_RECOVERED_RANK) {
    debug_print("ERROR Fenix_Data_member_restore_from_rank: invalid source_rank -- must be source rank must first be restored %s\n", "");
    retval = -1;
  } else if (current_rank == source_rank) {
    debug_print("ERROR Fenix_Data_member_restore_from_rank: cannot restore from own rank %s\n", "");
    retval = -1;
  } else {
    
    /* Under Construction */

    /* Recover the data here  */
    fenix_group_t *group = g_data_recovery;
    int version_num;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    fenix_member_entry_t *mentry;
    fenix_version_t *version;

    /* This part is moved to group recovery and part of group entity */
    int current_status;
    int remote_status = 0;

    if (member_index != -1) {
      mentry = &(member->member_entry[member_index]);
    } else {
      member_index = member->count;
      mentry = &(member->member_entry[member->count]);
      member->count++;
    }
    current_status = mentry->state;

    MPI_Status status;
    /* Check the role of the neighbor define by the group */
    MPI_Sendrecv(&current_status, 1, MPI_INT, gentry->out_rank, PARTNER_STATUS_TAG,
                 &remote_status, 1, MPI_INT, gentry->in_rank, PARTNER_STATUS_TAG,
                 (gentry->comm), &status);


    version = &(mentry->version);
    /* Send the member information if needed */
    if (current_status == OCCUPIED && remote_status == NEEDFIX) {
      _pc_send_member_metadata( gentry->current_rank, gentry->in_rank, mentry, gentry->comm);
      _pc_send_member_entries(  gentry->current_rank, gentry->in_rank, gentry->depth, version,
                                gentry->comm);
    } else if (current_status == NEEDFIX && remote_status == OCCUPIED) {
      //verbose_print("recieving entries! (before) %s %d from %d\n", "",current_rank,out_rank);
      _pc_recover_member_metadata( gentry->current_rank, gentry->out_rank, mentry, gentry->comm);
      _pc_recover_member_entries(  gentry->current_rank, gentry->out_rank, gentry->depth, version,
                                 gentry->comm);
    }

    /* Get the latest consistent copy */
    if (join_restore(gentry, version, gentry->comm) == 1) {
      //printf("POSITION %d\n",version->position - 1);
      fenix_local_entry_t *lentry = &(version->local_entry[version->position - 1]);
      mentry->pdata = data;
      mentry->current_datatype = lentry->datatype;
      mentry->current_count = lentry->count;
      mentry->current_size = lentry->size;
      memcpy(lentry->pdata, lentry->data, lentry->count * lentry->size);
      if (options->verbose == 25) {
        verbose_print("c-rank: %d, role: %d, v-pos: %d, ld-totalsize: %d\n",
                      get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                      (version->position - 1), (lentry->count * lentry->size));
      }
      // member->status = __FENIX_MEMBER_FINE; 
      retval = FENIX_SUCCESS;
    } else {
      retval = FENIX_ERROR_GROUP_CREATE;
    }

  }
#endif
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
int data_subset_create(int num_blocks, int start_offset, int end_offset, int stride,
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
int data_subset_createv(int num_blocks, int *array_start_offsets, int *array_end_offsets,
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

int data_subset_free(Fenix_Data_subset *subset_specifier) {
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
int data_subset_delete(Fenix_Data_subset *subset_specifier) {
  data_subset_free(subset_specifier);
  free(subset_specifier);
  return FENIX_SUCCESS;
}

/**
 * @brief
 * @param group_id
 * @param num_members
 */
int get_number_of_members(int group_id, int *num_members) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    *num_members = member->size;
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
int get_member_at_position(int group_id, int *member_id, int position) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    if (position < 0 || position > (member->size) - 1) {
      debug_print(
              "ERROR Fenix_Data_group_get_member_at_position: position <%d> must be a value between 0 and number_of_members-1 \n",
              position);
      retval = FENIX_ERROR_INVALID_POSITION;
    } else {
      int member_index = ((member->size) - 1) - position;
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
int get_number_of_snapshots(int group_id, int *num_snapshots) {
  int retval = -1;
  int group_index = search_groupid(group_id);
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", group_id);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    fenix_member_entry_t *mentry = &(member->member_entry[0]); // does not matter which member you get, every member has the same number of versions 
    fenix_version_t *version = &(mentry->version);
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
int get_snapshot_at_position(int groupid, int position, int *timestamp) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  if (options->verbose == 33) {
    verbose_print("c-rank: %d, role: %d, group_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index);
  }
  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_commit: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);

    if (position < 0 || position > (gentry->depth - 1)) {
      debug_print(
              "ERROR Fenix_Data_commit: position <%d> must be a value between 0 and number_of_snapshots-1 \n",
              position);
      retval = FENIX_ERROR_INVALID_POSITION;
    } else {

      // what are we iterating through in this method? 
      // which pos are we interested in?
      // group? member? there is no version array
      // lcoal? remote? but local and remote both do not have a timestamp attr
      int member_index;
      for (member_index = 0; member_index < member->size; member_index++) {
        fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
        fenix_version_t *version = &(mentry->version);
        int version_index = ((version->size) - 1) - position;
        fenix_local_entry_t *lentry = &(version->local_entry[version_index]);
        //timestamp = lentry->timestamp;
      }
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
 * @param source_rank
 */
int member_get_attribute(int groupid, int memberid, int attributename,
                         void *attributevalue, int *flag, int sourcerank) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = search_memberid(group_index, memberid);
  if (options->verbose == 34) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);

    switch (attributename) {
      case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
        if (lentry->size > 0 && lentry->count > 0) {
          memcpy(attributevalue, lentry->data,
                 (lentry->count * lentry->size)); // deep copy
        } else {
          attributevalue = lentry->data; // shallow copy
        }
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
        *((int *) (attributevalue)) = lentry->count;
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE:
        attributevalue = lentry->datatype;
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_SIZE:
        *((int *) (attributevalue)) = lentry->size;
        retval = FENIX_SUCCESS;
        break;
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
int member_set_attribute(int groupid, int memberid, int attributename,
                         void *attributevalue, int *flag) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = search_memberid(group_index, memberid);
  if (options->verbose == 35) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    ensure_version_capacity(member);
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    fenix_local_entry_t *lentry = &(version->local_entry[version->position]);

    switch (attributename) {
      case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
        lentry->data = attributevalue;
        if (lentry->datatype != NULL) {
          lentry->data = s_malloc(sizeof(lentry->datatype) * lentry->count);
          // lentry->data = s_malloc(lentry->size * lentry->count); 
          retval = FENIX_SUCCESS;
        } else if (lentry->size > 0) {
          lentry->data = s_malloc(lentry->size * lentry->count);
          retval = FENIX_SUCCESS;
        } else {
          debug_print(
                  "ERROR Fenix_Data_member_attr_get: Must enter _DATATYPE or _SIZE before _BUFFER\n",
                  "");
          retval = FENIX_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_COUNT:
        lentry->count = *((int *) (attributevalue));
        retval = FENIX_SUCCESS;
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE:
        if (attributevalue == MPI_INT) {
          lentry->datatype = attributevalue;
          retval = FENIX_SUCCESS;
        } else {
          debug_print(
                  "ERROR Fenix_Data_member_attr_get: Fenix currently does not support this MPI_DATATYPE; invalid attribute_value <%d>\n",
                  attributevalue);
          retval = FENIX_ERROR_INVALID_ATTRIBUTE_NAME;
        }
        break;
      case FENIX_DATA_MEMBER_ATTRIBUTE_SIZE:
        lentry->size = *((int *) (attributevalue));
        retval = FENIX_SUCCESS;
        break;
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
int snapshot_delete(int group_id, int time_stamp) {
  int retval = -1;
  int group_index = search_groupid(group_id);
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
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    int member_index;
    for (member_index = 0; member_index < member->size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      fenix_version_t *version = &(mentry->version);
      if (time_stamp == FENIX_DATA_SNAPSHOT_LATEST) {
        free_local(&(version->local_entry[version->position]));
      } else if (time_stamp == FENIX_DATA_SNAPSHOT_ALL) {
        int version_index;
        for (version_index = 0; version_index < version->size; version_index++) {
          free_local(&(version->local_entry[version_index]));
        }
      } else {
        int version_index = time_stamp - gentry->timestart;
        free_local(&(version->local_entry[version_index]));
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
int group_delete(int groupid) {
  /******************************************/
  /* Commit needs to confirm the completion */
  /* of delete. Otherwise, it is reverted.  */
  /******************************************/
  int retval = -1;
  int group_index = search_groupid(groupid);

  if (options->verbose == 37) {
    verbose_print("c-rank: %d, group_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), group_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_group_delete: group_id <%d> does not exist\n", groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    /* Delete Process */
    fenix_group_t *group = g_data_recovery;
    group->count--;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->depth = 0;
    gentry->groupid = -1;
    gentry->timestamp = -1;
    gentry->state = DELETED;

    if (options->verbose == 37) {
      verbose_print(
              "c-rank: %d, g-count: %d, g-size: %d, g-depth: %d, g-groupid: %d, g-timestamp: %d, g-state: %d\n",
              get_current_rank(*__fenix_g_new_world), group->count, group->size,
              gentry->depth, gentry->groupid,
              gentry->timestamp, gentry->state);
    }

    fenix_member_t *member = &(gentry->member);
    member->count = 0; /* Logical counter for member */

    /* Free-up members */
    int member_index;
    for (member_index = 0; member_index < member->size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      mentry->memberid = -1;
      mentry->state = DELETED;
      fenix_version_t *version = &(mentry->version);
      version->count = 0;

      if (options->verbose == 37) {
        verbose_print(
                "c-rank: %d, member[%d] m-count: %d, m-size: %d, m-memberid: %d, m-state: %d, v-count: %d\n",
                get_current_rank(*__fenix_g_new_world), member_index, member->count,
                member->size, mentry->memberid,
                mentry->state, version->count);
      }

      /* Free-up versions */
      int verison_index;
      for (verison_index = 0; verison_index < version->size; verison_index++) {
        free_local(&(version->local_entry[version->position]));
        free_remote(&(version->remote_entry[version->position]));
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
int member_delete(int groupid, int memberid) {
  int retval = -1;
  int group_index = search_groupid(groupid);
  int member_index = search_memberid(group_index, memberid);

  if (options->verbose == 38) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group_index,
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
    fenix_group_t *group = g_data_recovery;
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    fenix_member_t *member = &(gentry->member);
    member->count--;
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->state = DELETED;
    fenix_version_t *version = &(mentry->version);
    version->count = 0;

    if (options->verbose == 38) {
      verbose_print("c-rank: %d, role: %d, m-count: %d, m-state: %d, v-count: %d\n",
                    get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                    member->count, mentry->state,
                    version->count);
    }

    int verison_index;
    for (verison_index = 0; verison_index < version->size; verison_index++) {
      free_local(&(version->local_entry[verison_index]));
      free_remote(&(version->remote_entry[verison_index]));
    }
    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 */
fenix_group_t *init_group() {
  fenix_group_t *group = (fenix_group_t *)
          s_calloc(1, sizeof(fenix_group_t));
  group->count = 0;
  group->size = __FENIX_DEFAULT_GROUP_SIZE;
  group->group_entry = (fenix_group_entry_t *) s_malloc(
          __FENIX_DEFAULT_GROUP_SIZE * sizeof(fenix_group_entry_t));

  if (options->verbose == 41) {
    verbose_print("c-rank: %d, role: %d, g-count: %d, g-size: %d\n",
                  get_current_rank(*__fenix_g_world), __fenix_g_role, group->count,
                  group->size);
  }

  int group_index;
  for (group_index = 0;
       group_index < __FENIX_DEFAULT_GROUP_SIZE; group_index++) { // insert default values
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->depth = 1;
    gentry->groupid = -1;
    gentry->timestamp = 0;
    gentry->state = EMPTY;

    gentry->member = *init_member();
  }
  return group;
}

/**
 * @brief
 */
fenix_member_t *init_member() {
  fenix_member_t *member = (fenix_member_t *)
          s_calloc(1, sizeof(fenix_member_t));
  member->count = 0;
  member->size = __FENIX_DEFAULT_MEMBER_SIZE;
  member->member_entry = (fenix_member_entry_t *) s_malloc(
          __FENIX_DEFAULT_MEMBER_SIZE * sizeof(fenix_member_entry_t));

  int member_index;
  for (member_index = 0; member_index <
                         __FENIX_DEFAULT_MEMBER_SIZE; member_index++) { // insert default values
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->memberid = -1;
    mentry->state = EMPTY;

    mentry->version = *init_version();
  }
  return member;
}

/**
 * @brief
 */
fenix_version_t *init_version() {
  fenix_version_t *version = (fenix_version_t *)
          s_calloc(1, sizeof(fenix_version_t));
  version->count = 1;
  version->num_copies = 0;
  version->size = __FENIX_DEFAULT_VERSION_SIZE;
  version->position = 0;
  version->local_entry = (fenix_local_entry_t *) s_malloc(
          __FENIX_DEFAULT_VERSION_SIZE * sizeof(fenix_local_entry_t));
  version->remote_entry = (fenix_remote_entry_t *) s_malloc(
          __FENIX_DEFAULT_VERSION_SIZE * sizeof(fenix_remote_entry_t));

  int version_index;
  for (version_index = 0;
       version_index < __FENIX_DEFAULT_VERSION_SIZE; version_index++) {
    version->local_entry[version_index] = *init_local();
    version->remote_entry[version_index] = *init_remote();
  }
  return version;
}

/**
 * @brief 
 */
fenix_local_entry_t *init_local() {
  fenix_local_entry_t *local = (fenix_local_entry_t *) s_malloc(
          sizeof(fenix_local_entry_t));;
  local->currentrank = -1;
  local->pdata = NULL;
  local->data = NULL;
  local->count = 0;
  local->size = 0;
  local->datatype = NULL;

  return local;
}

/**
 * @brief
 */
fenix_remote_entry_t *init_remote() {
  fenix_remote_entry_t *remote = (fenix_remote_entry_t *) s_malloc(
          sizeof(fenix_remote_entry_t));
  remote->remoterank = -1;
  remote->pdata = NULL;
  remote->data = NULL;
  remote->count = 0;
  remote->size = 0;
  remote->datatype = NULL;

  return remote;
}

/**
 * @brief
 */
void free_local(fenix_local_entry_t *l) {
  fenix_local_entry_t *local = l;
  local->currentrank = -1;
  local->pdata = NULL;
  if (local->data != NULL) {
    free(local->data);
  }
  local->count = 0;
  local->size = 0;
  local->datatype = NULL;

  if (options->verbose == 46) {
    verbose_print(
            "c-rank: %d, role: %d, ld-currentrank: %d, ld-count: %d, ld-size: %d\n",
            get_current_rank(*__fenix_g_new_world), __fenix_g_role,
            local->currentrank, local->count, local->size);
  }

}

/**
 * @brief
 * @param
 */
void free_remote(fenix_remote_entry_t *r) {
  fenix_remote_entry_t *remote = r;
  remote->remoterank = -1;
  remote->pdata = NULL;
  if (remote->data == NULL) {
    free(remote->data);
  }
  remote->count = 0;
  remote->size = 0;
  remote->datatype = NULL;

  if (options->verbose == 47) {
    verbose_print(
            "c-rank: %d, role: %d, rd-remoterank: %d, rd-count: %d, rd-size: %d\n",
            get_current_rank(*__fenix_g_new_world), __fenix_g_role,
            remote->remoterank, remote->count, remote->size);
  }

}

/**
 * @brief
 * @param
 * @param
 */
void reinit_group(fenix_group_t *g, two_container_packet_t packet) {
  fenix_group_t *group = g;
  int start_index = group->size;
  group->count = packet.count;
  group->size = packet.size;
  group->group_entry = (fenix_group_entry_t *) s_realloc(group->group_entry,
                                                         (group->size) *
                                                         sizeof(fenix_group_entry_t));

  if (options->verbose == 48) {
    verbose_print("c-rank: %d, role: %d, g-size: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, group->size);
  }

  int group_index;
  for (group_index = start_index; group_index < group->size; group_index++) {
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->depth = 1;
    gentry->groupid = -1;
    gentry->timestamp = 0;
    gentry->state = EMPTY;

    if (options->verbose == 48) {
      verbose_print(
              "c-rank: %d, role: %d, g-depth: %d, g-groupid: %d, g-timestamp: %d, g-state: %d\n",
              get_current_rank(*__fenix_g_new_world), __fenix_g_role, gentry->depth,
              gentry->groupid, gentry->timestamp, gentry->state);
    }

    gentry->member = *init_member();
  }
}

/**
 * @brief  
 * @param
 * @param
 */
void reinit_member(fenix_member_t *m, two_container_packet_t packet,
                   enum states mystatus) {
  fenix_member_t *member = m;
  int start_index = member->size;
  member->count = 0;
  member->temp_count = packet.count;
  member->size = packet.size;
  member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                            (member->size) *
                                                            sizeof(fenix_member_entry_t));
  if (options->verbose == 50) {
    verbose_print("c-rank: %d, role: %d, m-count: %d, m-size: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                  member->count, member->size);
  }

  int member_index;
  /* Why start_index is set to the number of member entries ? */
  // for (member_index = start_index; member_index < member->size; member_index++) {
  for (member_index = 0; member_index < member->size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->memberid = -1;
    mentry->state = mystatus;
    if (options->verbose == 50) {
      verbose_print("c-rank: %d, role: %d, m-memberid: %d, m-state: %d\n",
                    get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                    mentry->memberid, mentry->state);
    }

    mentry->version = *init_version();
  }
}

/**
 * @brief
 * @param
 * @param
 */
void reinit_version(fenix_version_t *v, container_packet_t packet) {

  int first_index = v->size;
  v->num_copies = packet.num_copies;
  v->count = packet.count;
  v->size = packet.size;
  v->position = packet.position;
  v->local_entry = (fenix_local_entry_t *) realloc(v->local_entry,
                                                   (v->size) *
                                                   sizeof(fenix_local_entry_t));
  v->remote_entry = (fenix_remote_entry_t *) realloc(v->remote_entry,
                                                     (v->size) *
                                                     sizeof(fenix_remote_entry_t));

  if (options->verbose == 49) {
    verbose_print("c-rank: %d, role: %d, v-count: %d, v-size: %d, v-position: %d\n",
                  get_current_rank(*__fenix_g_new_world), __fenix_g_role, v->count,
                  v->size, v->position);
  }

/*
 * Allocate space for data entry
 */
  int version_index;
  for (version_index = first_index; version_index < v->size; version_index++) {
    v->local_entry[version_index] = *init_local();
    v->remote_entry[version_index] = *init_remote();
  }
}

/**
 * @brief
 * @param
 */
void ensure_group_capacity(fenix_group_t *g) {
  fenix_group_t *group = g;
  if (group->count >= group->size) {
    int start_index = group->size;
    group->group_entry = (fenix_group_entry_t *) s_realloc(group->group_entry,
                                                           (group->size * 2) *
                                                           sizeof(fenix_group_entry_t));
    group->size = group->size * 2;

    if (options->verbose == 51) {
      verbose_print("g-count: %d, g-size: %d\n", group->count, group->size);
    }

    int group_index;
    for (group_index = start_index; group_index < group->size; group_index++) {
      fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
      gentry->depth = 1;
      gentry->groupid = -1;
      gentry->timestamp = 0;
      gentry->state = EMPTY;

      if (options->verbose == 51) {
        verbose_print(
                "c-rank: %d, role: %d, group[%d] g-depth: %d, g-groupid: %d, g-timestamp: %d, g-state: %d\n",
                get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                group_index, gentry->depth, gentry->groupid, gentry->groupid,
                gentry->timestamp, gentry->state);
      }

      gentry->member = *init_member();
    }
  }
}

/**
 * @brief
 * @param
 */
void ensure_member_capacity(fenix_member_t *m) {
  fenix_member_t *member = m;
  if (member->count >= member->size) {
    int start_index = member->size;
    member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                              (member->size * 2) *
                                                              sizeof(fenix_member_entry_t));
    member->size = member->size * 2;

    if (options->verbose == 52) {
      verbose_print("c-rank: %d, role: %d, m-count: %d, m-size: %d\n",
                    member->count, member->size);
    }

    int member_index;
    for (member_index = start_index; member_index < member->size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      mentry->memberid = -1;
      mentry->state = EMPTY;

      if (options->verbose == 52) {
        verbose_print(
                "c-rank: %d, role: %d, member[%d] m-memberid: %d, m-state: %d\n",
                get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                member_index, mentry->memberid, mentry->state);
      }

      mentry->version = *init_version();
    }
  }
}

/**
 * @brief
 * @param
 */
void ensure_version_capacity(fenix_member_t *m) {
  fenix_member_t *member = m;
  int member_index;
  for (member_index = 0; member_index < member->size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = &(mentry->version);
    if (version->size > __FENIX_DEFAULT_VERSION_SIZE) {
      version->local_entry = (fenix_local_entry_t *) realloc(version->local_entry,
                                                             (version->size * 2) *
                                                             sizeof(fenix_local_entry_t));
      version->remote_entry = (fenix_remote_entry_t *) realloc(
              version->remote_entry,
              (version->size * 2) *
              sizeof(fenix_remote_entry_t));
      version->size = version->size * 2;

      if (options->verbose == 53) {
        verbose_print(
                "c-rank: %d, role: %d, member[%d] v-count: %d, v-size: %d\n",
                get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                member_index, version->count, version->size);
      }

    }
  }
}

/**
 * @brief
 * @param
 */
int search_groupid(int key) {
  fenix_group_t *group = g_data_recovery;
  int group_index, found = -1, index = -1;
  for (group_index = 0;
       (found != 1) && (group_index < group->size); group_index++) {
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    if (key == gentry->groupid) {
      index = group_index;
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
int search_memberid(int group_index, int key) {
  fenix_group_t *group = g_data_recovery;
  fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
  fenix_member_t *member = &(gentry->member);
  int member_index, found = -1, index = -1;
  for (member_index = 0;
       (found != 1) && (member_index < member->size); member_index++) {
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
int find_next_group_position(fenix_group_t *g) {
  fenix_group_t *group = g;
  int group_index, found = -1, index = -1;
  for (group_index = 0;
       (found != 1) && (group_index < group->size); group_index++) {
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    if (gentry->state == EMPTY || gentry->state == DELETED) {
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
int find_next_member_position(fenix_member_t *m) {
  fenix_member_t *member = m;
  int member_index, found = -1, index = -1;
  for (member_index = 0;
       (found != 1) && (member_index < member->size); member_index++) {
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
int join_group(fenix_group_t *g, fenix_group_entry_t *ge, MPI_Comm comm) {
  fenix_group_t *group = g;
  fenix_group_entry_t *gentry = ge;
  int current_rank_attributes[__GROUP_ENTRY_ATTR_SIZE];
  int other_rank_attributes[__GROUP_ENTRY_ATTR_SIZE];
  current_rank_attributes[0] = group->count;
  current_rank_attributes[1] = gentry->groupid;
  current_rank_attributes[2] = gentry->timestamp;
  current_rank_attributes[3] = gentry->depth;
  current_rank_attributes[4] = gentry->state;

  if (options->verbose == 58) {
    verbose_print(
            "c-rank: %d, g-count: %d, g-groupid: %d, g-timestamp: %d, g-depth: %d, g-state: %d\n",
            get_current_rank(*__fenix_g_new_world), group->count, gentry->groupid,
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
int join_member(fenix_member_t *m, fenix_member_entry_t *me, MPI_Comm comm) {
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
int join_restore(fenix_group_entry_t *ge, fenix_version_t *v, MPI_Comm comm) {
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
    v->position = (v->position + v->size - idiff) % (v->size);
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
int join_commit(fenix_group_entry_t *ge, fenix_version_t *v, MPI_Comm comm) {
  int found = -1;
  int min_timestamp;
  MPI_Allreduce(&(ge->timestamp), &min_timestamp, 1, MPI_INT, MPI_MIN, comm);

  int timestamp_offset = ge->timestamp - min_timestamp;
  int depth_offest = (ge->timestamp - ge->depth);
  if ((min_timestamp > depth_offest) && (timestamp_offset < v->num_copies)) {
    v->position = (v->position + (v->size - timestamp_offset)) % (v->size);
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

void store_single() {


}

/**
 *
 */
void __feninx_dr_print_store() {
  fenix_group_t *current = g_data_recovery;
  int group_count = current->count;
  for (int group = 0; group < group_count; group++) {
    int member_count = current->group_entry[group].member.count;
    for (int member = 0; member < member_count; member++) {
      int version_count = current->group_entry[group].member.member_entry[member].version.count;
      for (int version = 0; version < version_count; version++) {
        int local_data_count = current->group_entry[group].member.member_entry[member].version.local_entry[version].count;
        int *local_data = current->group_entry[group].member.member_entry[member].version.local_entry[version].data;
        for (int local = 0; local < local_data_count; local++) {
          //printf("*** store rank[%d] group[%d] member[%d] local[%d]: %d\n",
          //get_current_rank(*__fenix_g_new_world), group, member, local,
          //local_data[local]);
        }
        int remote_data_count = current->group_entry[group].member.member_entry[member].version.remote_entry[version].count;
        int *remote_data = current->group_entry[group].member.member_entry[member].version.remote_entry[version].data;
        for (int remote = 0; remote < remote_data_count; remote++) {
          printf("*** store rank[%d] group[%d] member[%d] remote[%d]: %d\n",
                 get_current_rank(*__fenix_g_new_world), group, member, remote,
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
  fenix_group_t *current = g_data_recovery;
  int group_count = current->count;
  int member_count = current->group_entry[0].member.count;
  int version_count = current->group_entry[0].member.member_entry[0].version.count;
  int local_data_count = current->group_entry[0].member.member_entry[0].version.local_entry[0].count;
  int remote_data_count = current->group_entry[0].member.member_entry[0].version.remote_entry[0].count;
  printf("*** restore rank: %d; group: %d; member: %d; local: %d; remote: %d\n",
         get_current_rank(*__fenix_g_new_world), group_count, member_count,
         local_data_count,
         remote_data_count);
}

/**
 *
 */
void __fenix_dr_print_datastructure() {
  fenix_group_t *current = g_data_recovery;

  if (!current) {
    return;
  }

  printf("\n\ncurrent_rank: %d\n", get_current_rank(*__fenix_g_new_world));
  int group_size = current->size;
  for (int group_index = 0; group_index < group_size; group_index++) {
    int depth = current->group_entry[group_index].depth;
    int groupid = current->group_entry[group_index].groupid;
    int timestamp = current->group_entry[group_index].timestamp;
    int group_state = current->group_entry[group_index].state;
    int member_size = current->group_entry[group_index].member.size;
    int member_count = current->group_entry[group_index].member.count;
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

    for (int member_index = 0; member_index < member_size; member_index++) {
      int memberid = current->group_entry[group_index].member.member_entry[member_index].memberid;
      int member_state = current->group_entry[group_index].member.member_entry[member_index].state;
      int version_size = current->group_entry[group_index].member.member_entry[member_index].version.size;
      int version_count = current->group_entry[group_index].member.member_entry[member_index].version.count;
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

      for (int verison_index = 0; verison_index < version_size; verison_index++) {
        int local_data_count = current->group_entry[group_index].member.member_entry[member_index].version.local_entry[verison_index].count;
        printf("group[%d] member[%d] version[%d] local_data.count: %d\n",
               group_index,
               member_index,
               verison_index, local_data_count);
        if (current->group_entry[group_index].member.member_entry[member_index].version.local_entry[verison_index].data !=
            NULL) {
          int *current_local_data = (int *) current->group_entry[group_index].member.member_entry[member_index].version.local_entry[verison_index].data;
          for (int local_data_index = 0;
               local_data_index < local_data_count; local_data_index++) {
            printf("group[%d] member[%d] depth[%d] local_data[%d]: %d\n",
                   group_index,
                   member_index,
                   verison_index, local_data_index,
                   current_local_data[local_data_index]);
          }
        }

        int remote_data_count = current->group_entry[group_index].member.member_entry[member_index].version.remote_entry[verison_index].count;
        printf("group[%d] member[%d] version[%d] remote_data.count: %d\n",
               group_index,
               member_index, verison_index, remote_data_count);
        if (current->group_entry[group_index].member.member_entry[member_index].version.remote_entry[verison_index].data !=
            NULL) {
          int *current_remote_data = current->group_entry[group_index].member.member_entry[member_index].version.remote_entry[verison_index].data;
          for (int remote_data_index = 0;
               remote_data_index < remote_data_count; remote_data_index++) {
            printf("group[%d] member[%d] depth[%d] remote_data[%d]: %d\n",
                   group_index,
                   member_index, verison_index, remote_data_index,
                   current_remote_data[remote_data_index]);
          }
        }
      }
    }
  }
}
