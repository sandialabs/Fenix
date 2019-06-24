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
   int entries_size;
   int entries_count;
   fenix_imr_mentry_t* entries;
   int num_snapshots;
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

   int my_rank, comm_size;
   MPI_Comm_size(comm, &comm_size);
   MPI_Comm_rank(comm, &my_rank);

   if(new_group->raid_mode == 1){
      new_group->partners = (int*) malloc(sizeof(int) * 2);
      
      //Set up the person who's data I am storing
      //We need to add comm size to the value since otherwise we might be modding a negative number,
      //  which is implementation-dependent behavior.
      new_group->partners[0] = (comm_size + my_rank - new_group->rank_separation)%comm_size;

      //Set up the person who is storing my data
      new_group->partners[1] = (my_rank + new_group->rank_separation)%comm_size;
   
   } else if(new_group->raid_mode == 5 || new_group->raid_mode == 6){
      new_group->set_size = policy_vals[2];
      new_group->partners = (int*) malloc(sizeof(int));
   }

   new_group->entries_size = __FENIX_IMR_DEFAULT_MENTRY_NUM;
   new_group->entries_count = 0;
   new_group->entries = 
      (fenix_imr_mentry_t*) malloc(sizeof(fenix_imr_mentry_t) * __FENIX_IMR_DEFAULT_MENTRY_NUM);
   new_group->num_snapshots = 0;


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

void __imr_alloc_data_region(void** region, int raid_mode, int local_data_size){
   if(raid_mode == 1){
      *region = (void*) malloc(2*local_data_size);
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
         __imr_alloc_data_region(new_imr_mentry->data + i, group->raid_mode, local_data_size);

         //Initialize to smallest # blocks allowed.
         __fenix_data_subset_init(1, new_imr_mentry->data_regions + i);
         new_imr_mentry->data_regions[i].specifier = __FENIX_SUBSET_EMPTY;

        //-1 is not a valid timestamp, use as an indicator that the data isn't valid.
        new_imr_mentry->timestamp[i] = -1;
      }
      //The first commit's timestamp is the group's timestart.
      new_imr_mentry->timestamp[0] = group->base.timestart;

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
   int retval = -1;
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
      debug_print("ERROR Fenix_Data_member_store: member_id <%d> does not exist!\n",
                member_id);
      retval = FENIX_ERROR_INVALID_MEMBERID;
   } else {
      //Copy my own data, trade data with partner, update data region
      //Store my data at the beginning of the member's buffer, resiliency data after that.
      __fenix_data_subset_copy_data(&subset_specifier, mentry->data[mentry->current_head],
         member_data->user_data, member_data->datatype_size, member_data->current_count);
      
      size_t serialized_size;
      void* serialized = __fenix_data_subset_serialize(&subset_specifier, 
            mentry->data[mentry->current_head], member_data->datatype_size, 
            member_data->current_count, &serialized_size);

      if(group->raid_mode == 1){

         void* recv_buf = malloc(serialized_size * member_data->datatype_size);

         MPI_Sendrecv(serialized, serialized_size * member_data->datatype_size, MPI_BYTE,
               group->partners[1], group->base.groupid ^ STORE_PAYLOAD_TAG, recv_buf, 
               serialized_size * member_data->datatype_size, MPI_BYTE, group->partners[0], 
               group->base.groupid ^ STORE_PAYLOAD_TAG, group->base.comm, NULL); 

         //Expand the serialized data out and store into the partner's portion of this data entry.
         __fenix_data_subset_deserialize(&subset_specifier, recv_buf, 
               mentry->data[mentry->current_head] + member_data->datatype_size*member_data->current_count,
               member_data->current_count, member_data->datatype_size);

         free(recv_buf);

      } else {
         debug_print("ERROR Fenix_Data_member_store: Raid mode <%d> is not supported yet!\n",
                   group->raid_mode);
         retval = FENIX_ERROR_UNINITIALIZED; 
      }

      free(serialized);

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

   //For each entry id (eid)
   for(int eid = 0; eid < group->entries_count; eid++){ 
      fenix_imr_mentry_t *mentry = &group->entries[eid];

      //Two cases for each member entry: 
      //    (1) depth has been reached, shift out the oldest commit 
      //    (2) depth has not been reached, just commit and start filling a new location.
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
         mentry->timestamp[group->base.depth + 1] = mentry->timestamp[group->base.depth] + 1;
      
      } else {
         //The entry is not full, just shift the current head.
         mentry->current_head++;

         //Everything is initialized to correct values, we just need to provide
         //the correct timestamp for the next snapshot.
         mentry->timestamp[mentry->current_head] = mentry->timestamp[mentry->current_head-1] + 1;
          
         if(eid == 0){
            //Only do this once
            group->num_snapshots++;
         }
      }
   }

   group->base.timestamp = group->entries[0].timestamp[group->entries[0].current_head - 1];

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
      //If this isn't true, some other bug has occured. Thus, we will just
      //query the first member.
      *time_stamp = group->entries[0].timestamp[group->entries[0].current_head - 1 - position];
      retval = FENIX_SUCCESS;
   } 

   return retval;
}


int __imr_member_restore(fenix_group_t* g, int member_id,
        void* target_buffer, int max_count, int time_stamp){ 
   int retval = -1;

   fenix_imr_group_t* group = (fenix_imr_group_t*)g;
   
   fenix_imr_mentry_t* mentry;
   //find_mentry returns the error status. We found the member (and corresponding data) if there are no errors.
   int found_member = !(__imr_find_mentry(group, member_id, &mentry));

   int member_data_index = __fenix_search_memberid(group->base.member, member_id);
   fenix_member_entry_t member_data = group->base.member->member_entry[member_data_index];

   if(group->raid_mode == 1){
      int my_data_found, partner_data_found;

      //We need to know if both partners found their data.
      //First send to partner 1 and recv from partner 0, then flip.
      MPI_Sendrecv(&found_member, 1, MPI_INT, group->partners[0], PARTNER_STATUS_TAG,
            &my_data_found, 1, MPI_INT, group->partners[1], PARTNER_STATUS_TAG, 
            group->base.comm, NULL);
      MPI_Sendrecv(&found_member, 1, MPI_INT, group->partners[1], PARTNER_STATUS_TAG,
            &partner_data_found, 1, MPI_INT, group->partners[0], PARTNER_STATUS_TAG, 
            group->base.comm, NULL);
      
      if(found_member && partner_data_found){
         //I have my data, and the person who's data I am backing up has theirs. We're good to go.
         retval = FENIX_SUCCESS;
      } else if (!found_member && !my_data_found) {
         //I lost my data, and my partner 1 doesn't have a copy for me to restore from.
         debug_print("ERROR Fenix_Data_member_restore: member_id <%d> does not exist at <%d> or partner <%d>\n",
               member_id, group->base.current_rank, group->partners[0]);
         
         retval = FENIX_ERROR_INVALID_MEMBERID;
      } else if(found_member && !partner_data_found){
         //My partner needs info on this member. This policy does nothing special w/ extra input params, so
         //I can just send the basic member metadata.
         __fenix_data_member_send_metadata(group->base.groupid, member_id, group->partners[0]);

         //Now my partner will need all of the entries. First they'll need to know how many snapshots
         //to expect.
         MPI_Send((void*) &(group->num_snapshots), 1, MPI_INT, group->partners[0], 
               RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm);

         //They also need the timestamps for each snapshot, as well as the value for the next.
         MPI_Send((void*)mentry->timestamp, group->num_snapshots+1, MPI_INT, group->partners[0],
               RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm);

         for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
            //send data region info next
            __fenix_data_subset_send(mentry->data_regions + snapshot, group->partners[0], 
                  __IMR_RECOVER_DATA_REGION_TAG ^ group->base.groupid, group->base.comm);
            
            size_t size;
            void* toSend = __fenix_data_subset_serialize(mentry->data_regions+snapshot, 
                  mentry->data[snapshot], member_data.datatype_size, member_data.current_count, 
                  &size);
            MPI_Send(toSend, member_data.datatype_size*size, MPI_BYTE, group->partners[0], 
                  RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm);
            
            free(toSend);
         }

      } else if(!found_member && partner_data_found) {
         //I need info on this member.
         fenix_member_entry_packet_t packet;
         __fenix_data_member_recv_metadata(group->base.groupid, group->partners[1], &packet);
         
         //We remake the new member just like the user would.
         __fenix_member_create(group->base.groupid, packet.memberid, NULL, packet.current_count,
               packet.current_datatype);

         __imr_find_mentry(group, member_id, &mentry);
         int member_data_index = __fenix_search_memberid(group->base.member, member_id);
         member_data = group->base.member->member_entry[member_data_index];
        

         MPI_Recv((void*)&(group->num_snapshots), 1, MPI_INT, group->partners[1],
               RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm, NULL);

         mentry->current_head = group->num_snapshots;

         //We also need to explicitly ask for all timestamps, since user may have deleted some and caused mischief.
         MPI_Recv((void*)(mentry->timestamp), group->num_snapshots + 1, MPI_INT, group->partners[1],
               RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm, NULL);

         //now recover data.
         for(int snapshot = 0; snapshot < group->num_snapshots; snapshot++){
            __fenix_data_subset_free(mentry->data_regions+snapshot);
            __fenix_data_subset_recv(mentry->data_regions+snapshot, group->partners[1],
                  __IMR_RECOVER_DATA_REGION_TAG ^ group->base.groupid, group->base.comm);

            int recv_size = __fenix_data_subset_data_size(mentry->data_regions + snapshot,
                  member_data.current_count);
            
            if(recv_size > 0){
               void* recv_buf = malloc(member_data.datatype_size * recv_size);
               MPI_Recv(recv_buf, recv_size*member_data.datatype_size, MPI_BYTE, group->partners[1],
                     RECOVER_MEMBER_ENTRY_TAG^group->base.groupid, group->base.comm, NULL);

               __fenix_data_subset_deserialize(mentry->data_regions + snapshot, recv_buf,
                        mentry->data[snapshot], member_data.current_count, member_data.datatype_size);

               free(recv_buf);
            }
         }

      }


      //Now that we've ensured everyone has data, restore from it.
      //Don't try to restore if we weren't able to get the relevant data.
      if(found_member || my_data_found){
         Fenix_Data_subset* data_region = (Fenix_Data_subset*)malloc(sizeof(Fenix_Data_subset));
         __fenix_data_subset_init(1, data_region);
         data_region->specifier = __FENIX_SUBSET_EMPTY;
         
         int oldest_snapshot;
         for(oldest_snapshot = (mentry->current_head - 1); oldest_snapshot >= 0; oldest_snapshot--){
            __fenix_data_subset_merge_inplace(data_region, mentry->data_regions + oldest_snapshot);

            if(__fenix_data_subset_is_full(data_region, member_data.current_count)){
               //The snapshots have formed a full set of data, not need to add older snapshots.
               break;
            }
         }

         //If there isn't a full set of data, don't try to pull from nonexistant snapshot.
         if(oldest_snapshot == -1){ 
            oldest_snapshot = 0;
         }

         for(int i = oldest_snapshot; i < mentry->current_head; i++){
            __fenix_data_subset_copy_data(&mentry->data_regions[i], target_buffer,
                  mentry->data[i], member_data.datatype_size, member_data.current_count);
         }

         __fenix_data_subset_free(data_region);
      }
      
   } else {
      debug_print("ERROR Fenix_Data_member_store: Raid mode <%d> is not supported yet!\n",
                group->raid_mode);
      retval = FENIX_ERROR_UNINITIALIZED;  
   }
   
   return retval;
}


int __imr_member_restore_from_rank(fenix_group_t* group, int member_id,
        void* target_buffer, int max_count, int time_stamp, 
        int source_rank){return 0;}


int __imr_member_get_attribute(fenix_group_t* group, fenix_member_t* member, 
        int attributename, void* attributevalue, int* flag, int sourcerank){return 0;}
int __imr_member_set_attribute(fenix_group_t* group, fenix_member_t* member, 
           int attributename, void* attributevalue, int* flag){return 0;}

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
