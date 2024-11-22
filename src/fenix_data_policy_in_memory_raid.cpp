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

#include <mpi.h>
#include "fenix.h"
#include "fenix_ext.h"
#include "fenix_opt.h"
#include "fenix_data_subset.h"
#include "fenix_data_recovery.h"
#include "fenix_data_policy.h"
#include "fenix_data_group.h"
#include "fenix_data_member.h"

#define __FENIX_IMR_DEFAULT_MENTRY_NUM 10
#define __FENIX_IMR_NO_MEMBERS 16000
#define __IMR_RECOVER_DATA_REGION_TAG 97854

#define STORE_PAYLOAD_TAG 2004

int __imr_group_delete(fenix_group_t* group);
int __imr_member_create(fenix_group_t* group, fenix_member_entry_t* mentry);
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
        void* target_buffer, int max_count, int time_stamp,
        Fenix_Data_subset* data_found);
int __imr_member_lrestore(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp,
        Fenix_Data_subset* data_found);
int __imr_member_restore_from_rank(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp, 
        int source_rank);
int __imr_member_get_attribute(fenix_group_t* group, fenix_member_entry_t* member, 
        int attributename, void* attributevalue, int* flag, int sourcerank);
int __imr_member_set_attribute(fenix_group_t* group, fenix_member_entry_t* member, 
           int attributename, void* attributevalue, int* flag);
int __imr_get_number_of_snapshots(fenix_group_t* group, 
        int* number_of_snapshots);
int __imr_get_snapshot_at_position(fenix_group_t* group, int position,
        int* time_stamp);
int __imr_reinit(fenix_group_t* group, int* flag);

typedef struct __fenix_imr_mentry{
   void** data;
   Fenix_Data_subset* data_regions;
   int* timestamp;
   int current_head;
   int memberid;
} fenix_imr_mentry_t;

typedef struct __fenix_imr_group{
   fenix_group_t base;
   int raid_mode;
   int rank_separation;
   int* partners;
   int set_size;
   MPI_Comm set_comm;
   int entries_size;
   int entries_count;
   fenix_imr_mentry_t* entries;
   int num_snapshots;
   int* timestamps;
} fenix_imr_group_t;

typedef struct __fenix_imr_undo_log{
  int groupid, memberid;
} fenix_imr_undo_log_t;

void __imr_sync_timestamps(fenix_imr_group_t* group);

void __imr_undo_restore(MPI_Comm comm, int err, void* data){
  fenix_imr_undo_log_t* undo_log = (fenix_imr_undo_log_t*)data;

  Fenix_Data_member_delete(undo_log->groupid, undo_log->memberid);
  
  free(data);
  Fenix_Callback_pop(); //Should be this callback itself.
}


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
   new_group->base.vtbl.member_lrestore = *__imr_member_lrestore;
   new_group->base.vtbl.member_restore_from_rank = *__imr_member_restore_from_rank;
   new_group->base.vtbl.member_get_attribute = *__imr_member_get_attribute;
   new_group->base.vtbl.member_set_attribute = *__imr_member_set_attribute;
   new_group->base.vtbl.get_number_of_snapshots = *__imr_get_number_of_snapshots;
   new_group->base.vtbl.get_snapshot_at_position = *__imr_get_snapshot_at_position;
   new_group->base.vtbl.reinit = *__imr_reinit;

   int* policy_vals = (int*)policy_value;
   new_group->raid_mode = policy_vals[0];
   new_group->rank_separation = policy_vals[1];

   int my_rank, comm_size;
   MPI_Comm_size(comm, &comm_size);
   MPI_Comm_rank(comm, &my_rank);

   if(new_group->raid_mode == 1){
      //Set up the person who's data I am storing as partner 0
      //Set up the person who is storing my data as partner 1
      new_group->partners = (int*) malloc(sizeof(int) * 2);
     
      //odd-sized groups take some extra handling.
      bool isOdd = ((comm_size%2) != 0);
      
      int remaining_size = comm_size;
      if(isOdd) remaining_size -= 3;
      
      //We want to form groups of rank_separation*2 to pair within
      int n_full_groups = remaining_size / (new_group->rank_separation*2);
     
      //We don't always get what we want though, one group may need to be smaller.
      int mini_group_size = (remaining_size - n_full_groups*new_group->rank_separation*2)/2;
      

      int start_rank = mini_group_size + (isOdd?1:0);
      int mid_rank = comm_size/2; //Only used when isOdd
      
      int end_mini_group_start = comm_size-mini_group_size-(isOdd?1:0);
      int start_mini_group_start = (isOdd?1:0);
      bool in_start_mini=false, in_end_mini=false;

      if(my_rank >= start_mini_group_start && my_rank < start_mini_group_start+mini_group_size){
         in_start_mini = true; 
      } else if(my_rank >= end_mini_group_start && my_rank < comm_size-(isOdd?1:0)){
          in_end_mini = true;
      }

      //Allocate the "normal" ranks
      if(my_rank >= start_rank && my_rank < end_mini_group_start && (!isOdd || my_rank != mid_rank)){
         //"effective" rank for determining which group I'm in and if I look forward or backward for a partner.
         int e_rank = my_rank - start_rank;
         if(isOdd && my_rank > mid_rank) --e_rank; //We skip the middle rank when isOdd

         int my_partner;
         if(((e_rank/new_group->rank_separation)%2) == 0){
            //Look forward for partner.
            my_partner = my_rank + new_group->rank_separation;
            if(isOdd && my_rank < mid_rank && my_partner >= mid_rank) ++my_partner;
         } else {
            my_partner = my_rank - new_group->rank_separation;
            if(isOdd && my_rank > mid_rank && my_partner <= mid_rank) --my_partner;
         }

         new_group->partners[0] = my_partner;
         new_group->partners[1] = my_partner;
      } else if(in_start_mini) {
         int e_rank = my_rank - start_mini_group_start;
         int partner = end_mini_group_start + e_rank;
         new_group->partners[0] = partner;
         new_group->partners[1] = partner;
      } else if(in_end_mini) {
         int e_rank = my_rank - end_mini_group_start;
         int partner = start_mini_group_start + e_rank;
         new_group->partners[0] = partner;
         new_group->partners[1] = partner;
      } else { //Only things left are the three ranks that must be paired to handle odd-sized comms
         if(my_rank == 0){
            new_group->partners[0] = comm_size-1;
            new_group->partners[1] = mid_rank; 
         } else if(my_rank == mid_rank){
            new_group->partners[0] = 0;
            new_group->partners[1] = comm_size-1; 
         } else if(my_rank == comm_size-1){
            new_group->partners[0] = mid_rank;
            new_group->partners[1] = 0; 
         } else {
            fprintf(stderr, "FENIX_IMR Fatal error: Rank <%d> no partner assigned, this is a bug in IMR!\n", my_rank);
            *flag = FENIX_ERROR_GROUP_CREATE;
            return;
         }
      }


   } else if(new_group->raid_mode == 5){
      new_group->set_size = policy_vals[2];
      new_group->partners = (int*) malloc(sizeof(int) * new_group->set_size);

      //User is responsible for giving values that "make sense" for set size and rank separation given a comm size.
      int my_set_pos = (my_rank/new_group->rank_separation)%new_group->set_size;
      for(int index = 0; index < new_group->set_size; index++){
        new_group->partners[index] = (comm_size + my_rank - (new_group->rank_separation * (my_set_pos-index)))%comm_size;
      }

      //Build a comm to use for all of the set's reductions we'll need to do for RAID 5.
      MPI_Group comm_group, set_group;
      MPI_Comm_group(comm, &comm_group);
      MPI_Group_incl(comm_group, new_group->set_size, new_group->partners, &set_group);
      MPI_Comm_create_group(comm, set_group, 0, &(new_group->set_comm));

   }

   new_group->entries_size = __FENIX_IMR_DEFAULT_MENTRY_NUM;
   new_group->entries_count = 0;
   new_group->entries = 
      (fenix_imr_mentry_t*) malloc(sizeof(fenix_imr_mentry_t) * __FENIX_IMR_DEFAULT_MENTRY_NUM);
   new_group->num_snapshots = 0;
   new_group->timestamps = (int*)malloc(sizeof(int)*depth);
 
   new_group->base.comm = comm;
   new_group->base.current_rank = my_rank;
   __imr_sync_timestamps(new_group);
   *flag = FENIX_SUCCESS;
}

//Sets mentry to point to the right index for a given memberid
//If there are no members, the mentry pointer will be invalid and __FENIX_IMR_NO_MEMBERS will be returned.
//If the given memberid is not found, points to the closest and returns anything but FENIX_SUCCESS.
int __imr_find_mentry(fenix_imr_group_t* group, int memberid, fenix_imr_mentry_t** mentry){
   //List is sorted by member id, do binary search.
   int retval = -1;
   unsigned lower_bound = 0;
   unsigned upper_bound = group->entries_count - 1;


   if(group->entries_count == 0){
      upper_bound = 0;
      retval = __FENIX_IMR_NO_MEMBERS;
   }
   
   while(lower_bound != upper_bound){
      unsigned to_check = (lower_bound + upper_bound)>>1;

      if(group->entries[to_check].memberid == memberid){
         lower_bound = upper_bound = to_check;
      } else if(group->entries[to_check].memberid < memberid){
         lower_bound = to_check + 1;
         if(lower_bound > upper_bound) lower_bound = upper_bound;
      } else {
         upper_bound = to_check - 1;
         if(lower_bound > upper_bound) upper_bound = lower_bound;
      }
   }

   *mentry = group->entries + lower_bound;
   if(retval != __FENIX_IMR_NO_MEMBERS && (*mentry)->memberid == memberid){
      retval = FENIX_SUCCESS;
   }
   return retval;
}

void __imr_alloc_data_region(void** region, int raid_mode, int local_data_size, int set_size){
   if(raid_mode == 1){
      *region = (void*) malloc(2*local_data_size);
   } else if(raid_mode == 5){
      //We need space for our own local data, as well as space for the parity data
      //We add two just in case the data size isn't evenly divisible by set_size-1
      //  3 is needed because making the parity one larger on some nodes requires 
      //  extra bits of "data" on the other nodes
      *region = (void*) malloc(local_data_size + local_data_size/(set_size - 1) + 3);
   } else {
      debug_print("Error: raid mode <%d> not supported\n", raid_mode);
   }
}

int __imr_member_create(fenix_group_t* g, fenix_member_entry_t* mentry){
   fenix_imr_group_t* group = (fenix_imr_group_t*)g;
   int retval = -1;

   fenix_imr_mentry_t* closest_imr_mentry;
   int found_memberid = __imr_find_mentry(group, mentry->memberid, &closest_imr_mentry);

   if(found_memberid == FENIX_SUCCESS){
      debug_print("Error Fenix_Data_member_create: member_id <%d> already exists in this policy\n",
            mentry->memberid);
   } else {
      //Double check that we have room for the member.
      if(group->entries_count >= group->entries_size){
         group->entries = (fenix_imr_mentry_t*) s_realloc(group->entries,
               group->entries_size * 2 * sizeof(fenix_imr_mentry_t));
         group->entries_size *= 2;
      }

      fenix_imr_mentry_t* new_imr_mentry;
      if(found_memberid == __FENIX_IMR_NO_MEMBERS){
         //This is the first member, it goes at the beginning.
         new_imr_mentry = group->entries;
      } else {
         int closest_index = closest_imr_mentry - group->entries;
         //Do we want to place this new member before or after
         //the closest member?
         if(mentry->memberid > closest_imr_mentry->memberid){
            //Move all entries after the closest  to one farther to right, b/c I belong
            //right after the closest.
            memmove(group->entries+closest_index +1, group->entries+closest_index +2, 
                  group->entries_size - closest_index+1);
            new_imr_mentry = group->entries+closest_index+1;
         } else {
            //Move all entries starting w/ closest  to one farther to right, b/c I belong
            //right before the closest.
            memmove(group->entries+closest_index, group->entries+closest_index +1, 
                  group->entries_size - closest_index);
            new_imr_mentry = group->entries+closest_index;
         }
      }

      //Now I've got the location to store this member,
      //so I just need to actually fill in the data.
      new_imr_mentry->current_head = 0;
      new_imr_mentry->memberid = mentry->memberid;
      
      new_imr_mentry->data = (void**) malloc( (group->base.depth+2) * sizeof(void*));
      int local_data_size = mentry->datatype_size * mentry->current_count;
      new_imr_mentry->data_regions = 
         (Fenix_Data_subset *)malloc(sizeof(Fenix_Data_subset) * (group->base.depth+2) );
      new_imr_mentry->timestamp = (int*) malloc(sizeof(int) * (group->base.depth + 2));
      
      for(int i = 0; i < group->base.depth + 2; i++){
         __imr_alloc_data_region(new_imr_mentry->data + i, group->raid_mode, local_data_size, group->set_size);

         //Initialize to smallest # blocks allowed.
         __fenix_data_subset_init(1, new_imr_mentry->data_regions + i);
         new_imr_mentry->data_regions[i].specifier = __FENIX_SUBSET_EMPTY;
      }

      group->entries_count++;

      retval = FENIX_SUCCESS;
   }

   return retval;
}

void __imr_member_free(fenix_imr_mentry_t* mentry, int depth){
  //Start by clearing out the mentry's data pointers.
  for(int i = 0; i < depth + 2; i++){
     __fenix_data_subset_free(mentry->data_regions + i);
     free(mentry->data[i]);
  }

  free(mentry->data);
  free(mentry->data_regions);
  free(mentry->timestamp);
}

int __imr_member_delete(fenix_group_t* g, int member_id){
   int retval = FENIX_SUCCESS;
   fenix_imr_group_t* group = (fenix_imr_group_t*)g;
   //Find the member first
   fenix_imr_mentry_t *mentry;
   int found_member = __imr_find_mentry(group, member_id, &mentry);
   
   if(found_member != FENIX_SUCCESS){
      debug_print("ERROR Fenix_Data_member_delete: member_id <%d> does not exist!\n",
                member_id);
      retval = FENIX_ERROR_INVALID_MEMBERID;
   } else {
      
      //Free all of the pointers in the mentry
      __imr_member_free(mentry, group->base.depth);

      //Now shift all the subsequent mentries back one, unless I'm already the last one.
      int member_index = mentry - group->entries;
      if(member_index != (group->entries_count-1) ){
         memmove(mentry, mentry+1, group->entries_count - 1 - member_index);
      }

      group->entries_count--;
   }
   return retval;
}



int __imr_member_store(fenix_group_t* g, int member_id, 
        Fenix_Data_subset subset_specifier){
   int retval = -1;
   fenix_imr_group_t* group = (fenix_imr_group_t*)g;
   
   fenix_imr_mentry_t* mentry;
   int found_member = __imr_find_mentry(group, member_id, &mentry);

   fenix_member_entry_t* member_data;
   //Shouldn't need to check for failure to find the member, that should be done before
   //calling
   int member_data_index = __fenix_search_memberid(group->base.member, member_id);
   member_data = &(group->base.member->member_entry[member_data_index]);
   
   if(found_member != FENIX_SUCCESS){
      debug_print("ERROR Fenix_Data_member_store: member_id <%d> does not exist on rank <%d>!\n",
                member_id, g->current_rank);
      retval = FENIX_ERROR_INVALID_MEMBERID;
   } else {
      //Copy my own data, trade data with partner, update data region
      //Store my data at the beginning of the member's buffer, resiliency data after that.
      __fenix_data_subset_copy_data(&subset_specifier, mentry->data[mentry->current_head],
         member_data->user_data, member_data->datatype_size, member_data->current_count);
      
      if(group->raid_mode == 1){

         size_t serialized_size;
         void* serialized = __fenix_data_subset_serialize(&subset_specifier, 
               mentry->data[mentry->current_head], member_data->datatype_size, 
               member_data->current_count, &serialized_size);

         void* recv_buf = malloc(serialized_size * member_data->datatype_size);

         MPI_Sendrecv(serialized, serialized_size * member_data->datatype_size, MPI_BYTE,
                        group->partners[1], group->base.groupid ^ STORE_PAYLOAD_TAG, 
                      recv_buf, serialized_size * member_data->datatype_size, MPI_BYTE, 
                        group->partners[0], group->base.groupid ^ STORE_PAYLOAD_TAG, 
                      group->base.comm, NULL); 

         //Expand the serialized data out and store into the partner's portion of this data entry.
         __fenix_data_subset_deserialize(&subset_specifier, recv_buf, 
               ((uint8_t*)mentry->data[mentry->current_head]) + member_data->datatype_size*member_data->current_count,
               member_data->current_count, member_data->datatype_size);

         free(recv_buf);
         free(serialized);

      } else if(group->raid_mode == 5){
         //TODO: Try to optimize for partial commits - currently does parity on the whole region regardless of commit area.
         //TODO: I'm not sure if this is the best way to do this - could be a bottleneck if this is unoptimized since this 
         //      could be running on a lot of data.
         
         //Why does this do it this way?
         //In order to do recovery on a given block of data, we need to be missing only 1 of:
         //    all of the data in the corresponding blocks and the parity for those blocks
         //Standard RAID does this by having one disk store parity for a given block instead of data, but this assumes
         //    that there is no benefit to data locality - in our case we want each node to have a local copy of its own 
         //    data, preferably in a single (virtually) continuous memory range for data movement optimization. So we'll
         //    store the local data, then put 1/N of the parity data at the bottom of the commit.
         //The weirdness comes from the fact that a given node CANNOT contribute to the data being checked for parity which
         //    will be stored on itself. IE, a node cannot save both a portion of the data and the parity for that data portion - 
         //    doing so would mean if that node fails it is as if we lost two nodes for recovery semantics, making every failure
         //    non-recoverable.
         //    This means we need to do an XOR reduction across every node but myself, then store the result on myself - this is 
         //    a little awkward with MPI's reductions which require full comm participation and do not receive any information about
         //    the source of a given chunk of data (IE we can't exclude data from node X, as we want to).
         //This is easily doable using MPI send/recvs, but doing it that way neglects all of the data/comm size optimizations,
         //    as well as any block XOR optimizations from MPI's reduction operations.
         //We could do something like an alltoallv to send appropriate data to each node, then let them calculate parity info locally
         //    However, we have to either allocate space to hold an extra copy of the entire data size, or we overwrite our
         //    local buffer and have to re-distribute the data afterward.
         //I think the best way to handle it will be to manipulate the XOR function. We will do a reduction which uses local data
         //    that we do not actually want involved in calculating the parity. Then, we will XOR the local data with the result
         //    to get the accurate parity info.
         //    This involves computing the XOR on an extra 2/(set_size-1)*parity_size of data, but minimizes excess memory allocation
         //    and network use. Scales well with higher set sizes.
         int parity_size = (member_data->datatype_size * member_data->current_count)/(group->set_size - 1);
         int remainder = (member_data->datatype_size * member_data->current_count)%(group->set_size - 1);

         if(remainder != 0) remainder++;
         
         void* data_buf = mentry->data[mentry->current_head];
         //store parity info after my data in data region.
         //we always have a spare data buffer byte for rounding stuff, so store after that as well.
         void* parity_buf = (void*)((char*)data_buf + member_data->datatype_size*member_data->current_count + 2);
         
         int my_set_rank;
         MPI_Comm_rank(group->set_comm, &my_set_rank);
         int offset = 0, rounding_compensator;
         for(int i = 0; i < group->set_size; i++){
            //Last node is an edge case.
            if((my_set_rank == group->set_size-1) && i==my_set_rank){
              offset = 0;
            }

            MPI_Reduce((char*)data_buf + offset, parity_buf, parity_size + (i < remainder ? 1 : 0), MPI_BYTE,
                MPI_BXOR, i, group->set_comm);
            if(i != my_set_rank){
               offset += parity_size + (i < remainder ? 1 : 0);
            }
         }

         //Each node has buffer which contains parity^some_local_data, so now pull parity from that.
         offset = my_set_rank * parity_size + (my_set_rank < remainder ? my_set_rank : remainder);
         
         //As above, last node is an edge case.
         if(my_set_rank == group->set_size - 1){
            offset = 0;
         }

         //Utilize MPI's local XOR function, assuming it is more optimized than a naive implementation would be.
         MPI_Reduce_local((void*)((char*)data_buf + offset), parity_buf, parity_size + (my_set_rank < remainder ? 1 : 0),
             MPI_BYTE, MPI_BXOR);

         //Finally, each node has the right stuff.

      } else {
         debug_print("ERROR Fenix_Data_member_store: Raid mode <%d> is not supported yet!\n",
                   group->raid_mode);
         retval = FENIX_ERROR_UNINITIALIZED; 
      }

      //Make sure to update which data regions this entry contains.
      __fenix_data_subset_merge_inplace(mentry->data_regions + mentry->current_head, &subset_specifier);
      
   }


   return retval;
}





int __imr_member_storev(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier){return 0;}
int __imr_member_istore(fenix_group_t* group, int member_id, 
        Fenix_Data_subset subset_specifier, Fenix_Request *request){return 0;}
int __imr_member_istorev(fenix_group_t* group, int member_id, 
           Fenix_Data_subset subset_specifier, Fenix_Request *request){return 0;}



int __imr_commit(fenix_group_t* g){
   //No sources of error for this one yet.
   int to_return = FENIX_SUCCESS;
   
   fenix_imr_group_t *group = (fenix_imr_group_t*)g;

   if(group->num_snapshots == group->base.depth+1){
      //Full of timestamps, remove the oldest and proceed as normal.
      memcpy(group->timestamps, group->timestamps+1, group->base.depth);
      group->num_snapshots--;
   }
   group->timestamps[group->num_snapshots++] = group->base.timestamp;


   //For each entry id (eid)
   for(int eid = 0; eid < group->entries_count; eid++){ 
      fenix_imr_mentry_t *mentry = &group->entries[eid];

      if(mentry->current_head == group->base.depth + 1){
         //The entry is full, one snapshot should be shifted out.
         
         //Save this pointer to reuse the allocated memory
         void* first_data = mentry->data[0];
         
         for(int snapshot = 0; snapshot < group->base.depth + 1; snapshot++){
            //lightweight movement, just moving the pointers about.
            mentry->data[snapshot] = mentry->data[snapshot + 1];
            __fenix_data_subset_deep_copy(mentry->data_regions + snapshot + 1,
                     mentry->data_regions + snapshot);
            mentry->timestamp[snapshot] = mentry->timestamp[snapshot + 1];
         }

         mentry->data[group->base.depth + 1] = first_data;
         mentry->data_regions[group->base.depth + 1].specifier = __FENIX_SUBSET_EMPTY;
         mentry->current_head--;
      }

      mentry->timestamp[mentry->current_head++] = group->base.timestamp;
   }

   return to_return;
}


int __imr_snapshot_delete(fenix_group_t* g, int time_stamp){
   int retval = FENIX_SUCCESS;

   fenix_imr_group_t *group = (fenix_imr_group_t*)g;

   for(int entry_id = 0; entry_id < group->entries_count && retval == FENIX_SUCCESS; entry_id++){
      //Search for the timestamp in each group. Given how commits and deletes work, we know
      //the snapshots are sorted by timestamp in the arrays.
      fenix_imr_mentry_t mentry = group->entries[entry_id];

      //current_head is the staging area's entry, so start before that and work backwards.
      //We'll work backwards under the assumption that snapshots are likely to be deleted soon after creation.
      //  (Does this assumption seem valid?)
      for(int snapshot = mentry.current_head - 1; snapshot >= 0 && retval == FENIX_SUCCESS; snapshot--){
         if(mentry.timestamp[snapshot] < time_stamp){
            retval = FENIX_ERROR_INVALID_TIMESTAMP;

         } else if(mentry.timestamp[snapshot] == time_stamp){
            void* old_data = mentry.data[snapshot];

            for(int to_shift = snapshot; to_shift < mentry.current_head; to_shift++){
               mentry.timestamp[to_shift] = mentry.timestamp[to_shift + 1];
               __fenix_data_subset_deep_copy(mentry.data_regions + to_shift+1,
                     mentry.data_regions + to_shift);
               mentry.data[to_shift] = mentry.data[to_shift + 1];
            }
            mentry.data[mentry.current_head] = old_data;
            mentry.data_regions[mentry.current_head].specifier = __FENIX_SUBSET_EMPTY;

            mentry.current_head--;
            break;
         }
      }
   }

   if(retval == FENIX_SUCCESS){
      group->num_snapshots--;
   }

   return retval;
}



int __imr_barrier(fenix_group_t* group){return 0;}


int __imr_get_number_of_snapshots(fenix_group_t* group, 
        int* number_of_snapshots){
   return ((fenix_imr_group_t*)group)->num_snapshots;
}

int __imr_get_snapshot_at_position(fenix_group_t* g, int position,
        int* time_stamp){
   int retval = -1;

   fenix_imr_group_t *group = (fenix_imr_group_t*)g;

   if(!(position < group->num_snapshots)){
      retval = FENIX_ERROR_INVALID_POSITION;
   } else {
      //Each member ought to have the same snapshots, in the same order.
      //If this isn't true, some other bug has occurred. Thus, we will just
      //query the first member.
      *time_stamp = group->entries[0].timestamp[group->entries[0].current_head - 1 - position];
      retval = FENIX_SUCCESS;
   } 

   return retval;
}


int __imr_member_restore(fenix_group_t* g, int member_id,
        void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* data_found){ 
   int retval = -1;

   fenix_imr_group_t* group = (fenix_imr_group_t*)g;
   //One-time fix after a reinit.
   if(group->base.timestamp == -1 && group->num_snapshots > 0)
      group->base.timestamp = group->timestamps[group->num_snapshots-1];
   
   fenix_imr_mentry_t* mentry;
   //find_mentry returns the error status. We found the member (and corresponding data) if there are no errors.
   int found_member = !(__imr_find_mentry(group, member_id, &mentry));

   fenix_member_entry_t member_data;
   if(found_member){
      int member_data_index = __fenix_search_memberid(group->base.member, member_id);
      member_data = group->base.member->member_entry[member_data_index];
   }

   int recovery_locally_possible;

   fenix_imr_undo_log_t* undo_data; //Used for undoing partial restores interrupted by failures.

   if(group->raid_mode == 1){
      int my_data_found, partner_data_found;

      //We need to know if both partners found their data.
      //First send to partner 0 and recv from partner 1, then flip.
      MPI_Sendrecv(&found_member, 1, MPI_INT, group->partners[0], PARTNER_STATUS_TAG,
            &my_data_found, 1, MPI_INT, group->partners[1], PARTNER_STATUS_TAG, 
            group->base.comm, NULL);
      MPI_Sendrecv(&found_member, 1, MPI_INT, group->partners[1], PARTNER_STATUS_TAG,
            &partner_data_found, 1, MPI_INT, group->partners[0], PARTNER_STATUS_TAG, 
            group->base.comm, NULL);
     
      
      if(found_member && partner_data_found && my_data_found){
         //I have my data, and the person who's data I am backing up has theirs. We're good to go.
         retval = FENIX_SUCCESS;
      } else if (!found_member && (!my_data_found || !partner_data_found)){
         //I lost my data, and my partner doesn't have a copy for me to restore from.
         debug_print("ERROR Fenix_Data_member_restore: member_id <%d> does not exist at <%d> or partner(s) <%d> <%d>\n",
               member_id, group->base.current_rank, group->partners[0], group->partners[1]);
         
         retval = FENIX_ERROR_INVALID_MEMBERID;
      } else if(found_member){
         //My partner(s) need info on this member. This policy does nothing special w/ extra input params, so
         //I can just send the basic member metadata.
         if(!partner_data_found)
             __fenix_data_member_send_metadata(group->base.groupid, member_id, group->partners[0]);

         for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
            //send data region info next
            if(!partner_data_found)
                __fenix_data_subset_send(mentry->data_regions + snapshot, group->partners[0], 
                      __IMR_RECOVER_DATA_REGION_TAG ^ group->base.groupid, group->base.comm);
            
            size_t size;
            void* toSend;
            //send my data, to maintain resiliency on my data
            if(!my_data_found){
                toSend = __fenix_data_subset_serialize(mentry->data_regions+snapshot, 
                      mentry->data[snapshot], member_data.datatype_size, member_data.current_count, 
                      &size);
                MPI_Send(toSend, member_data.datatype_size*size, MPI_BYTE, group->partners[1], 
                      RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm);
                free(toSend);
            }
            
            //send their data
            if(!partner_data_found){
                toSend = __fenix_data_subset_serialize(mentry->data_regions+snapshot, 
                      ((char*)mentry->data[snapshot]) + member_data.datatype_size*member_data.current_count,
                      member_data.datatype_size, member_data.current_count, &size);
                MPI_Send(toSend, member_data.datatype_size*size, MPI_BYTE, group->partners[0], 
                      RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm);
                free(toSend);
            }
            
         }

      } else if(!found_member) {
         //I need info on this member.
         fenix_member_entry_packet_t packet;
         __fenix_data_member_recv_metadata(group->base.groupid, group->partners[1], &packet);
         
         //We remake the new member just like the user would.
         __fenix_member_create(group->base.groupid, packet.memberid, NULL, packet.current_count,
               packet.datatype_size);

         //Mark the member for deletion if another failure interrupts recovering fully.
         undo_data = (fenix_imr_undo_log_t*)malloc(sizeof(fenix_imr_undo_log_t));
         undo_data->groupid = group->base.groupid;
         undo_data->memberid = member_id;
         Fenix_Callback_register(__imr_undo_restore, (void*)undo_data);

         __imr_find_mentry(group, member_id, &mentry);
         int member_data_index = __fenix_search_memberid(group->base.member, member_id);
         member_data = group->base.member->member_entry[member_data_index];
         
         mentry->current_head = group->num_snapshots;

         //now recover data.
         for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
            mentry->timestamp[snapshot] = group->timestamps[snapshot];

            __fenix_data_subset_free(mentry->data_regions+snapshot);
            __fenix_data_subset_recv(mentry->data_regions+snapshot, group->partners[1],
                  __IMR_RECOVER_DATA_REGION_TAG ^ group->base.groupid, group->base.comm);

            int recv_size = __fenix_data_subset_data_size(mentry->data_regions + snapshot,
                  member_data.current_count);
            
            if(recv_size > 0){
               void* recv_buf = malloc(member_data.datatype_size * recv_size);
               //first receive their data, so store in the resiliency section.
               MPI_Recv(recv_buf, recv_size*member_data.datatype_size, MPI_BYTE, group->partners[0],
                     RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm, NULL);
               __fenix_data_subset_deserialize(mentry->data_regions + snapshot, recv_buf,
                        ((char*)mentry->data[snapshot]) + member_data.current_count*member_data.datatype_size,
                        member_data.current_count, member_data.datatype_size);

               //Now receive my data.
               MPI_Recv(recv_buf, recv_size*member_data.datatype_size, MPI_BYTE, group->partners[1],
                     RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm, NULL);
               __fenix_data_subset_deserialize(mentry->data_regions + snapshot, recv_buf,
                        mentry->data[snapshot], member_data.current_count, member_data.datatype_size);

               free(recv_buf);
            }
         }
         
         //Member restored fully, so we don't need to mark it for undoing on failure.
         Fenix_Callback_pop();
         free(undo_data);
      }


      recovery_locally_possible = found_member || (my_data_found && partner_data_found);
      if(recovery_locally_possible) retval = FENIX_SUCCESS;

   } else if (group->raid_mode == 5){
      int* set_results = (int *) malloc(sizeof(int) * group->set_size);
      MPI_Allgather((void*)&found_member, 1, MPI_INT, (void*)set_results, 1, MPI_INT, 
          group->set_comm);

      int recovering_node = -1, recovery_possible = 1;
      for(int i = 0; i < group->set_size; i++){
        if(!set_results[i]){
          
          if(recovering_node == -1){
            recovering_node = i;
          } else {
            recovery_possible = 0;
            break;
          }
        
        }
      }

      free(set_results);

      //If we have a recovering node, and recovery is possible, do it
      if((recovering_node != -1) && recovery_possible){
        int my_set_rank;
        MPI_Comm_rank(group->set_comm, &my_set_rank);

        //The recovering node needs metadata on this member, just needs it from one partner.
        if((recovering_node == 0 && my_set_rank == 1) || my_set_rank == 0){
           //I'm the node that's going to send metadata
           
           //This function pulls comm from the base group - so we need to give 
           //dest_rank in terms of that comm
           __fenix_data_member_send_metadata(group->base.groupid, member_id, group->partners[recovering_node]);

           //Now my partner will need all of the entries. First they'll need to know how many snapshots
           //to expect.
           MPI_Send((void*) &(group->num_snapshots), 1, MPI_INT, recovering_node, 
                 RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->set_comm);

           //They also need the timestamps for each snapshot, as well as the value for the next.
           MPI_Send((void*)mentry->timestamp, group->num_snapshots+1, MPI_INT, recovering_node,
                 RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->set_comm);
          
           for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
              __fenix_data_subset_send(mentry->data_regions + snapshot, recovering_node, 
                    __IMR_RECOVER_DATA_REGION_TAG ^ group->base.groupid, group->set_comm);
           }

         } else if(!found_member) {
           //I'm the one that needs the info.
           fenix_member_entry_packet_t packet;
           __fenix_data_member_recv_metadata(group->base.groupid, group->partners[my_set_rank==0 ? 1 : 0], &packet);
           
           //We remake the new member just like the user would.
           __fenix_member_create(group->base.groupid, packet.memberid, NULL, packet.current_count,
                 packet.datatype_size);

           //Mark the member for deletion if another failure interrupts recovering fully.
           undo_data = (fenix_imr_undo_log_t*)malloc(sizeof(fenix_imr_undo_log_t));
           undo_data->groupid = group->base.groupid;
           undo_data->memberid = member_id;
           Fenix_Callback_register(__imr_undo_restore, (void*)undo_data);


           __imr_find_mentry(group, member_id, &mentry);
           int member_data_index = __fenix_search_memberid(group->base.member, member_id);
           member_data = group->base.member->member_entry[member_data_index];
          

           MPI_Recv((void*)&(group->num_snapshots), 1, MPI_INT, (my_set_rank==0 ? 1 : 0),
                 RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->set_comm, NULL);

           mentry->current_head = group->num_snapshots;

           //We also need to explicitly ask for all timestamps, since user may have deleted some and caused mischief.
           MPI_Recv((void*)(mentry->timestamp), group->num_snapshots + 1, MPI_INT, (my_set_rank==0 ? 1 : 0),
                 RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->set_comm, NULL);

           for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
              __fenix_data_subset_free(mentry->data_regions+snapshot);
              __fenix_data_subset_recv(mentry->data_regions+snapshot, (my_set_rank==0 ? 1 : 0),
                    __IMR_RECOVER_DATA_REGION_TAG ^ group->base.groupid, group->set_comm);
           }
         }

         for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
            //Similar to the process of doing a store, we're going to end up XORing with noisy data from
            //the recovering node, then XORing with it again to get what we actually want.
            int parity_size = (member_data.datatype_size*member_data.current_count)/(group->set_size-1);
            int remainder = (member_data.datatype_size*member_data.current_count)%(group->set_size-1);

            if(remainder > 0) remainder++;

            void* data_buf = mentry->data[snapshot];
            void* parity_buf = (void*)((char*)data_buf + member_data.datatype_size*member_data.current_count + 2);
            
            int offset = 0;
            for(int i = 0; i < group->set_size; i++){
               //Make sure to send the (out of order) parity info on the correct grouping
               void* toSend;
               if(i == my_set_rank){
                  if(my_set_rank != recovering_node){
                    toSend = parity_buf;
                  } else {
                    toSend = data_buf;
                  }
               } else {
                  if(my_set_rank != recovering_node){
                     toSend = (void*)((char*)data_buf + offset);
                  } else {
                     toSend = parity_buf;
                  }
               }
                
               void* recv_buf = (i == my_set_rank ? parity_buf : (void*)((char*)data_buf + offset));

               MPI_Reduce(toSend, recv_buf, parity_size + (1<remainder? 1:0), MPI_BYTE, MPI_BXOR, 
                   recovering_node, group->set_comm);

               if(my_set_rank == recovering_node){
                  //Remove the random data I had to send from the result.
                  MPI_Reduce_local(toSend, recv_buf, parity_size + (i<remainder? 1:0),
                      MPI_BYTE, MPI_BXOR);
               }
               
               if(i != my_set_rank){
                  offset += parity_size + (i<remainder? 1:0); 
               }
            }

            if(!found_member){
               //Member restored fully, so we don't need to mark it for undoing on failure.
               Fenix_Callback_pop();
               free(undo_data);
            }

         }

         retval = FENIX_SUCCESS;
         recovery_locally_possible = 1;
      } else if(!found_member){
         debug_print("ERROR Fenix_Data_member_restore: member_id <%d> does not exist at <%d> and is not recoverable from RAID-5 set\n",
               member_id, group->base.current_rank);
         
         retval = FENIX_ERROR_INVALID_MEMBERID;
         recovery_locally_possible = 0;      
      } else {
         retval = FENIX_SUCCESS;
         recovery_locally_possible = 1;
      }



   } else {
      debug_print("ERROR Fenix_Data_member_store: Raid mode <%d> is not supported yet!\n",
                group->raid_mode);
      retval = FENIX_ERROR_UNINITIALIZED;
      recovery_locally_possible = 0;
   }
   


   //Now that we've ensured everyone has data, restore from it.
   
   int return_found_data;
   if(data_found == NULL){
      data_found = (Fenix_Data_subset*) malloc(sizeof(Fenix_Data_subset));
      return_found_data = 0;
   } else {
      return_found_data = 1;   
   }
   __fenix_data_subset_init(1, data_found);
   
   //Don't try to restore if we weren't able to get the relevant data.
   if(recovery_locally_possible && target_buffer != NULL){
      data_found->specifier = __FENIX_SUBSET_EMPTY;
      
      int oldest_snapshot;
      for(oldest_snapshot = (mentry->current_head - 1); oldest_snapshot >= 0; oldest_snapshot--){
         __fenix_data_subset_merge_inplace(data_found, mentry->data_regions + oldest_snapshot);
 
         if(__fenix_data_subset_is_full(data_found, member_data.current_count)){
            //The snapshots have formed a full set of data, not need to add older snapshots.
            break;
         }
      }
 
      //If there isn't a full set of data, don't try to pull from nonexistent snapshot.
      if(oldest_snapshot == -1){
         oldest_snapshot = 0;
      }
 
      for(int i = oldest_snapshot; i < mentry->current_head; i++){
         __fenix_data_subset_copy_data(&mentry->data_regions[i], target_buffer,
               mentry->data[i], member_data.datatype_size, member_data.current_count);
      }

      if(__fenix_data_subset_is_full(data_found, member_data.current_count)){
        retval = FENIX_SUCCESS;
      } else {
        retval = FENIX_WARNING_PARTIAL_RESTORE;
      }
   } else {
      data_found->specifier = __FENIX_SUBSET_EMPTY;   
   }

   if(!return_found_data){
      __fenix_data_subset_free(data_found);
      free(data_found);
   }

   //Dont forget to clear the commit buffer
   if(recovery_locally_possible) mentry->data_regions[mentry->current_head].specifier = __FENIX_SUBSET_EMPTY;


   return retval;
}

int __imr_member_lrestore(fenix_group_t* g, int member_id,
        void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* data_found){ 
   int retval = -1;

   fenix_imr_group_t* group = (fenix_imr_group_t*)g;
   
   fenix_imr_mentry_t* mentry;
   //find_mentry returns the error status. We found the member (and corresponding data) if there are no errors.
   int found_member = !(__imr_find_mentry(group, member_id, &mentry));
   
   if(!found_member){
      return FENIX_ERROR_INVALID_MEMBERID;
   }

   int member_data_index = __fenix_search_memberid(group->base.member, member_id);
   fenix_member_entry_t member_data = group->base.member->member_entry[member_data_index];
  


   int return_found_data;
   if(data_found == NULL){
      data_found = (Fenix_Data_subset*) malloc(sizeof(Fenix_Data_subset));
      return_found_data = 0;
   } else {
      return_found_data = 1;   
   }
   __fenix_data_subset_init(1, data_found);

   data_found->specifier = __FENIX_SUBSET_EMPTY;

      
   int oldest_snapshot;
   for(oldest_snapshot = (mentry->current_head - 1); oldest_snapshot >= 0; oldest_snapshot--){
      __fenix_data_subset_merge_inplace(data_found, mentry->data_regions + oldest_snapshot);
 
      if(__fenix_data_subset_is_full(data_found, member_data.current_count)){
         //The snapshots have formed a full set of data, not need to add older snapshots.
         break;
      }
   }
 
   //If there isn't a full set of data, don't try to pull from nonexistent snapshot.
   if(oldest_snapshot == -1){
      oldest_snapshot = 0;
   }
 
   for(int i = oldest_snapshot; i < mentry->current_head; i++){
      __fenix_data_subset_copy_data(&mentry->data_regions[i], target_buffer,
            mentry->data[i], member_data.datatype_size, member_data.current_count);
   }
 
   if(__fenix_data_subset_is_full(data_found, member_data.current_count)){
     retval = FENIX_SUCCESS;
   } else {
     retval = FENIX_WARNING_PARTIAL_RESTORE;
   }

   //Dont forget to clear the commit buffer
   mentry->data_regions[mentry->current_head].specifier = __FENIX_SUBSET_EMPTY;

   return retval;

}


int __imr_member_restore_from_rank(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp, 
        int source_rank){return 0;}


int __imr_member_get_attribute(fenix_group_t* group, fenix_member_entry_t* member, 
        int attributename, void* attributevalue, int* flag, int sourcerank){return 0;}

int __imr_member_set_attribute(fenix_group_t* g, fenix_member_entry_t* member, 
           int attributename, void* attributevalue, int* flag){ 
  //No mutable attributes (as of now) require any changes to this policy's info
  return FENIX_SUCCESS;
}

int __imr_reinit(fenix_group_t* g, int* flag){
  fenix_imr_group_t* group = (fenix_imr_group_t*)g;

  if(group->raid_mode == 5){
    //Rebuild the set comm to re-include the failed node(s).
    MPI_Group comm_group, set_group;
    MPI_Comm_group(g->comm, &comm_group);
    MPI_Group_incl(comm_group, group->set_size, group->partners, &set_group);
    MPI_Comm_create_group(g->comm, set_group, 0, &(group->set_comm));
  }

  __imr_sync_timestamps(group);

  *flag = FENIX_SUCCESS;

  return FENIX_SUCCESS;
}

void __imr_sync_timestamps(fenix_imr_group_t* group){
   int n_snapshots = group->num_snapshots;
   
   if(group->raid_mode == 1){
      int partner_snapshots;
      MPI_Sendrecv(&n_snapshots,       1, MPI_INT, group->partners[0], 34560,
                   &partner_snapshots, 1, MPI_INT, group->partners[1], 34560,
                   group->base.comm, MPI_STATUS_IGNORE);
      n_snapshots = n_snapshots > partner_snapshots ? n_snapshots : partner_snapshots;

      MPI_Sendrecv(&n_snapshots,       1, MPI_INT, group->partners[1], 34561,
                   &partner_snapshots, 1, MPI_INT, group->partners[0], 34561,
                   group->base.comm, MPI_STATUS_IGNORE);
      n_snapshots = n_snapshots > partner_snapshots ? n_snapshots : partner_snapshots;
   } else {
      MPI_Allreduce(MPI_IN_PLACE, &n_snapshots, 1, MPI_INT, MPI_MAX, group->set_comm);
   }

   bool need_reset = group->num_snapshots != n_snapshots;
   for(int i = group->num_snapshots; i < n_snapshots; i++) group->timestamps[i] = -1;

   if(group->raid_mode == 1){
      int* p0_stamps = (int*)malloc(sizeof(int)*n_snapshots);
      int* p1_stamps = (int*)malloc(sizeof(int)*n_snapshots);
      
      MPI_Sendrecv(group->timestamps, n_snapshots, MPI_INT, group->partners[1], 34562,
                   p0_stamps,         n_snapshots, MPI_INT, group->partners[0], 34562,
                   group->base.comm, MPI_STATUS_IGNORE);
      MPI_Sendrecv(group->timestamps, n_snapshots, MPI_INT, group->partners[0], 34563,
                   p1_stamps,         n_snapshots, MPI_INT, group->partners[1], 34563,
                   group->base.comm, MPI_STATUS_IGNORE);
      
      for(int i = 0; i < n_snapshots; i++){
         int old_stamp = group->timestamps[i];
         group->timestamps[i] = group->timestamps[i] > p0_stamps[i] ? group->timestamps[i] : p0_stamps[i];
         group->timestamps[i] = group->timestamps[i] > p1_stamps[i] ? group->timestamps[i] : p1_stamps[i];

         need_reset |= group->timestamps[i] != old_stamp;
      }
      
      free(p0_stamps);
      free(p1_stamps);
   } else {
      MPI_Allreduce(MPI_IN_PLACE, group->timestamps, n_snapshots, MPI_INT, MPI_MAX, group->set_comm);
   }

   group->num_snapshots = n_snapshots;
   if(n_snapshots > 0) group->base.timestamp = group->timestamps[n_snapshots-1];
   else group->base.timestamp = -1;

   //Now fix members
   if(need_reset && group->entries_count > 0) {
      if(fenix.options.verbose == 1){
         verbose_print("Outdated timestamps on rank %d. All members will require full recovery.\n", 
                       group->base.current_rank);
      }
      //For now, just delete all members and assume partner(s) can 
      //help me rebuild fully consistent state
      for(int i = group->entries_count-1; i >= 0; i--){
         int memberid = group->entries[i].memberid;
         Fenix_Data_member_delete(group->base.groupid, memberid);
      }
   }
}

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

   for(int entry = 0; entry < group->base.member->count; entry++){
     __imr_member_free(group->entries+entry, g->depth); 
   }
   free(group->entries);

   //We have the responsibility of destroying the member array in the base group struct.
   __fenix_data_member_destroy(group->base.member);
   
   free(group->partners);
   free(group);
   return FENIX_SUCCESS;
}
