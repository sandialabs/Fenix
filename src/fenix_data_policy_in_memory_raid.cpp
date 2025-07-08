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

#include <algorithm>
#include <vector>
#include <deque>
#include <map>
#include <sstream>
#include <set>
#include <cstring>
#include <utility>

#include <mpi.h>
#include "fenix.h"
#include "fenix_ext.hpp"
#include "fenix_opt.hpp"
#include "fenix_data_subset.h"
#include "fenix_data_recovery.hpp"
#include "fenix_data_policy.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_member.hpp"
#include "fenix_data_policy_in_memory_raid.hpp"

#define __FENIX_IMR_DEFAULT_MENTRY_NUM 10
#define __FENIX_IMR_NO_MEMBERS 16000
#define __IMR_RECOVER_DATA_REGION_TAG 97854

#define STORE_PAYLOAD_TAG 2004

namespace Fenix::Data::IMR {

void __fenix_policy_in_memory_raid_get_group(fenix_group_t** group, MPI_Comm comm,
      int timestart, int depth, void* policy_value, int* flag){
   *group = new Group(comm, timestart, depth, (int*)policy_value, flag);
};

Entry::Entry(int size, int max_count)
   : elm_size(size), elm_max_count(max_count) {
   buf.reserve(size * max_count);
}

Entry::Entry(Entry&& other){
   *this = std::move(other);
}

Entry& Entry::operator=(Entry&& other){
    timestamp = std::exchange(other.timestamp, -2);

    region = std::move(other.region);
    partner_region = std::move(partner_region);
    buf = std::move(other.buf);
    partner_buf = std::move(other.partner_buf);
    elm_size = other.elm_size;
    elm_max_count = other.elm_max_count;
    return *this;
}

char* Entry::data(){ return buf.data(); }
void Entry::resize(int size){ buf.resize(size); }
int Entry::size(){ return buf.size(); }

char* Entry::partner_data(){ return partner_buf.data(); }
void Entry::partner_resize(int size){ partner_buf.resize(size); }
int Entry::partner_size(){ return partner_buf.size(); }

void Entry::add_and_fit(const DataSubset& subset){
   region += subset;

   int new_count = elm_max_count; 
   if(!new_count) region.max_count();
   if(!new_count) new_count = subset.max_count();

   int new_size = new_count*elm_size;
   if(new_size > buf.size()) buf.resize(new_size);
}

void Entry::partner_add_and_fit(const DataSubset& subset){
   partner_region += subset;

   int new_count = elm_max_count;
   if(!new_count) partner_region.max_count();
   if(!new_count) new_count = subset.max_count();

   int new_size = new_count*elm_size;
   if(new_size > partner_buf.size()) partner_buf.resize(new_size);
}

void Entry::reset(){
   timestamp = -2;
   
   buf.clear();
   partner_buf.clear();
   
   region = {};
   partner_region = {};
}

Member::Member(fenix_member_entry_t& my_mentry, Group& my_group)
   : mentry(my_mentry), group(my_group), send_buf(group.send_buf),
     recv_buf(group.recv_buf)
{
   for(int i = 0; i < group.depth+2; i++){
      entries.emplace_back(mentry.datatype_size, mentry.current_count);
   }
}

BuddyMember::BuddyMember(fenix_member_entry_t& my_mentry, Group& my_group)
   : Member(my_mentry, my_group) {
   for(auto& entry : entries)
      entry.partner_resize(mentry.datatype_size*mentry.current_count);
}

ParityMember::ParityMember(fenix_member_entry_t& my_mentry, Group& my_group)
   : Member(my_mentry, my_group) {
   int data_len = (mentry.datatype_size*mentry.current_count);
   int parity_len = data_len / (group.set_size-1);

   int remainder = data_len % (group.set_size-1);
   if(remainder) remainder++;
   if(remainder < group.set_rank) parity_len++;

   for(auto& entry : entries)
      entry.partner_resize(parity_len);
}


bool Member::snapshot_delete(int timestamp){
   bool found = false;
   for(int i = entries.size(); i >= 0; i--){
      if(entries[i].timestamp == timestamp){
         assert(!found);
         found = true;
         entries[i].reset();
      }
      //Move deleted snapshot to front
      if(found && i > 0){
         std::swap(entries[i], entries[i-1]);
      }
   }
   return found;
}

int Member::storev(const DataSubset& subset){
   Entry& e = entries.back();
   e.add_and_fit(subset);
   subset.copy_data(e.elm_size, e.elm_max_count, mentry.user_data, e.buf);
   return this->storev_impl(subset);
}

int BuddyMember::exch(
   const DataSubset& subset, const DataSubset& partner_subset
){
   const int rank = group.set_rank;
   const int left = rank == 0 ? group.set_size-1 : rank-1;
   const int right = rank == group.set_size-1 ? 0 : rank+1;

   Entry& e = entries.back();
   e.partner_add_and_fit(partner_subset);

   int recv_count = partner_subset.count(e.elm_max_count-1);
   recv_buf.reset(e.elm_size*recv_count);

   subset.serialize_data(e.elm_size, e.buf, send_buf);

   MPI_Sendrecv(
      send_buf.data(), send_buf.size(), MPI_BYTE, right, 0,
      recv_buf.data(), recv_buf.size(), MPI_BYTE,  left, 0,
      group.set_comm, MPI_STATUS_IGNORE
   );
 
   partner_subset.deserialize_data(
      e.elm_size, recv_buf, e.partner_buf
   );
   return FENIX_SUCCESS;
}

int BuddyMember::storev_impl(const DataSubset& subset){
   //My partner ranks (within set_comm)
   const int rank = group.set_rank;
   const int left = rank == 0 ? group.set_size-1 : rank-1;
   const int right = rank == group.set_size-1 ? 0 : rank+1;

   DataBuffer send_buf, recv_buf;
   subset.serialize(send_buf);

   for(int i = 0; i < group.set_size; i++){
      if(i == rank) send_buf.send(right, 0, group.set_comm);
      if(i == left) recv_buf.recv_unknown(left, 0, group.set_comm);
   }

   return exch(subset, DataSubset(recv_buf));
}

int Member::store(const DataSubset& subset){
   Entry& e = entries.back();
   e.add_and_fit(subset);
   subset.copy_data(e.elm_size, e.elm_max_count, mentry.user_data, e.buf);
   return this->store_impl(subset);
}

int BuddyMember::store_impl(const DataSubset& subset){
   return exch(subset, subset);
}

int ParityMember::store_impl(const DataSubset& subset){
   Entry& entry = entries.back();

   int parity_size = entry.size()/(group.set_size - 1);
   int remainder = entry.size()%(group.set_size - 1);

   //If we have any remainder, treat as if we have one more, since a rank
   //storing a larger parity block wasn't able to store a larger data block, so
   //all such ranks need one extra larger data block.
   if(remainder) remainder++;

   int m_parity_size = parity_size;
   if(group.set_rank<remainder) m_parity_size++;
   entry.partner_resize(m_parity_size);

   //Zero out the parity data before computing, so old data doesn't contribute
   std::memset(entry.partner_data(), 0, entry.partner_size());
   
   int offset = 0;
   for(int i = 0; i < group.set_size; i++){
      int len = i < remainder ? parity_size + 1 : parity_size;

      char* input;
      if(group.set_rank == i){
         //The rank storing this parity region contributes the all zero input
         //as a way of not contributing
         input = entry.partner_data();
      } else {
         if(offset+len > entry.size()){
            //Since we pretend to have an extra remainder if there is any
            assert(remainder);
            assert(group.set_rank >= remainder);
            assert(offset+len == entry.size()+1);
            offset--;
         }
         input = entry.data()+offset;
         offset += len;
      }

      MPI_Reduce(
         MPI_IN_PLACE, input, len, MPI_BYTE, MPI_BXOR, i, group.set_comm
      );
   }

   assert(offset == entry.size());
   return FENIX_SUCCESS;
}

void Member::commit(int timestamp){
   entries.back().timestamp = timestamp;

   Entry oldest = std::move(entries.front());
   entries.pop_front();
   oldest.reset();

   entries.push_back(std::move(oldest));
}

int Member::restore(){
   //First clear out any snapshots that we have but the group doesn't.
   auto begin = group.timestamps.begin();
   const auto end = group.timestamps.end();
   for(int entry = 0; entry < entries.size()-1; entry++){
      if(entries[entry].timestamp == -2) continue;
      begin = std::lower_bound(begin, end, entries[entry].timestamp);
      if(begin == end || *begin != entries[entry].timestamp)
         entries[entry].reset();
   }

   //Now make sure snapshots align with group's timestamps
   for(int snapshot = 1; snapshot <= group.timestamps.size(); snapshot++){
      int timestamp = group.timestamps[group.timestamps.size()-snapshot];
      int target = entries.size()-snapshot-1;
      int actual;
      for(actual = target; actual >= 0; actual--){
         if(entries[actual].timestamp == timestamp) break;
      }
      if(actual == target) continue;
      if(actual != -1) {
         std::swap(entries[actual], entries[target]);
      } else {
         int free = -1;
         for(int i = 0; i <= target && free == -1; i++){
            if(entries[i].timestamp == -2) free = i;
         }
         assert(free != -1);
         std::swap(entries[free], entries[target]);
      }
   }

   //Reset the current store buffer entry
   entries.back().reset();
   return this->restore_impl();
}

int BuddyMember::restore_impl(){
   //My partner ranks (within set_comm)
   const int rank = group.set_rank;
   const int left = rank == 0 ? group.set_size-1 : rank-1;
   const int right = rank == group.set_size-1 ? 0 : rank+1;

   //Data on which partners have found each snapshot
   int found[3];
   int& found_here = found[rank];
   int& found_left = found[left];
   int& found_right = found[right];

   auto e = entries.rbegin()+1;
   auto ts = group.timestamps.rbegin();
   for(; ts != group.timestamps.rend(); ts++, e++){
      fenix_assert(e->timestamp == -2 || e->timestamp == *ts);

      found_here = e->timestamp != -2;
      MPI_Allgather(
         MPI_IN_PLACE, 1, MPI_INT, found, 1, MPI_INT, group.set_comm
      );

      int n_missing = 0;
      for(int i = 0; i < group.set_size; i++) if(!found[i]) n_missing++;
      if(n_missing == 0)
         continue;
      else if(n_missing > 1){
         if(group.set_rank == 0)
            debug_print(
               "WARNING Fenix_Data_member_restore: %s member %d timestamp %d unrecoverable",
               group.str().c_str(), id, *ts
            );
         continue;
      }
     
      if(!found_here){
         //Fetch my data region from right partner
         recv_buf.recv_unknown(right, 0, group.set_comm);
         e->add_and_fit({recv_buf});
         //Fetch my data
         int m_count = e->region.count(e->elm_max_count-1);
         recv_buf.recv(m_count*e->elm_size, right, 0, group.set_comm);
         e->region.deserialize_data(e->elm_size, recv_buf, e->buf);

         //Fetch left partner's region
         recv_buf.recv_unknown(left, 0, group.set_comm);
         e->partner_add_and_fit({recv_buf});
         //Fetch data
         int p_count = e->partner_region.count(e->elm_max_count-1);
         recv_buf.recv(p_count*e->elm_size, left, 0, group.set_comm);
         e->region.deserialize_data(e->elm_size, recv_buf, e->partner_buf);

         //Only update timestamp after all other data updated, to indicate
         //recovery of this snapshot completed
         e->timestamp = *ts;
      }
      if(!found_left){
         //Send partner's data region
         e->partner_region.serialize(send_buf);
         send_buf.send(left, 0, group.set_comm);
         //Send their data
         e->partner_region.serialize_data(
            e->elm_size, e->partner_buf, send_buf
         );
         send_buf.send(left, 0, group.set_comm);
      }
      if(!found_right){
         //Send my data region
         e->region.serialize(send_buf);
         send_buf.send(right, 0, group.set_comm);
         //Send my data
         e->region.serialize_data(e->elm_size, e->buf, send_buf);
         send_buf.send(right, 0, group.set_comm);
      }
   }
   return FENIX_SUCCESS;
}

int ParityMember::restore_impl(){
   //Data on which partners have found each snapshot
   std::vector<int> found;
   found.resize(group.set_size);
   int found_here;

   auto e = entries.rbegin()+1;
   auto ts = group.timestamps.rbegin();
   for(; ts != group.timestamps.rend(); ts++, e++){
      fenix_assert(e->timestamp == -2 || e->timestamp == *ts);
     
      found_here = e->timestamp != -2;
      MPI_Allgather(
         &found_here, 1, MPI_INT, found.data(), 1, MPI_INT, group.set_comm
      );

      int recovering = -1;
      for(int i = 0; i < group.set_size; i++){
         if(found[i]) continue;
         if(recovering != -1){
            if(group.set_rank == 0)
               debug_print(
                  "WARNING Fenix_Data_member_restore: %s member %d timestamp %d unrecoverable",
                  group.str(), id, *ts
               );
            recovering = -1;
            break;
         } else {
            recovering = i;
         }
      }
      if(recovering == -1) continue;

      int sender = recovering == 0 ? 1 : 0;
      if(group.set_rank == sender){
         e->region.serialize(send_buf);
         send_buf.send(recovering, 0, group.set_comm);
      } else if(!found_here){
         recv_buf.recv_unknown(sender, 0, group.set_comm);
         e->add_and_fit({recv_buf});
      }

      //Use the same logic as store, but recovering rank is always root and
      //zeroes out the local data region before participating.
      int parity_size = e->size()/(group.set_size - 1);
      int remainder = e->size()%(group.set_size - 1);
      if(remainder) remainder++;
      int m_parity_size = parity_size;
      if(group.set_rank<remainder) m_parity_size++;
      e->partner_resize(m_parity_size);

      if(!found_here){
         std::memset(e->data(), 0, e->size());
         std::memset(e->partner_data(), 0, e->partner_size());
      }

      int offset = 0;
      for(int i = 0; i < group.set_size; i++){
         int len = i < remainder ? parity_size + 1 : parity_size;
         char* input;
         if(group.set_rank == i){
            input = e->partner_data();
         } else {
            if(offset+len > e->size()) offset--;
            input = e->data()+offset;
            offset += len;
         }
         MPI_Reduce(
            MPI_IN_PLACE, input, len, MPI_BYTE, MPI_BXOR, recovering,
            group.set_comm
         );
      }
      assert(offset == e->size());

      e->timestamp = *ts;
   }

   return FENIX_SUCCESS;
}

int Member::lrestore(
   char* target, int max_restore, int timestamp, DataSubset& recovered
){
   //Restoring always clears the commit buffer
   entries.back().reset();

   int end = 0;
   if(timestamp == FENIX_TIME_STAMP_MAX){
      if(entries[entries.size()-2].timestamp >= 0){
         end = entries.size()-1;
      }
   } else {
      for(int i = entries.size()-2; i >= 0; i--){
         if(entries[i].timestamp == timestamp){
            end = i+1;
            break;
         }
      }
   }

   int begin = end > 0 ? end-1 : end;
   if(max_restore != 0){
      for(int i = end-1; i >= 0 && !recovered.includes_all(max_restore) ; i--){
         if(entries[i].timestamp < 0) break;
         begin = i;
         recovered += entries[i].region;
      }
   } else if(begin < end) {
      recovered = entries[begin].region;
   }

   for(int i = begin; i < end && target != NULL; i++){
      Entry& e = entries[i];
      e.region.copy_data(e.elm_size, e.buf, max_restore, target);
   }

   if(end <= 0) return FENIX_ERROR_NODATA_FOUND;
   if(max_restore != 0 && !recovered.includes_all(max_restore))
      return FENIX_WARNING_PARTIAL_RESTORE;
   return FENIX_SUCCESS;
}

Group::Group(
   MPI_Comm m_comm, int timestart, int depth, int* policy, int* flag
){
   int* policy_vals = (int*)policy;
   mode = policy_vals ? policy_vals[0] : 1;
   rank_separation = policy_vals ? policy_vals[1] : __fenix_get_world_size(m_comm)/2;

   comm = m_comm;

   int my_rank, comm_size;
   MPI_Comm_size(comm, &comm_size);
   MPI_Comm_rank(comm, &my_rank);
   current_rank = my_rank;

   std::set<int> partner_set;
   partner_set.insert(my_rank);

   if(mode == 1){
      //odd-sized groups take some extra handling.
      bool isOdd = ((comm_size%2) != 0);
      
      int remaining_size = comm_size;
      if(isOdd) remaining_size -= 3;
      
      //We want to form groups of rank_separation*2 to pair within
      int n_full_groups = remaining_size / (rank_separation*2);
     
      //We don't always get what we want though, one group may need to be
      //smaller.
      int mini_group_size = 
         (remaining_size - n_full_groups*rank_separation*2)/2;
      
      int start_rank = mini_group_size + (isOdd?1:0);
      int mid_rank = comm_size/2; //Only used when isOdd
      
      int end_mini_group_start = comm_size-mini_group_size-(isOdd?1:0);
      int start_mini_group_start = (isOdd?1:0);
      bool in_start_mini = 
         my_rank >= start_mini_group_start
            && my_rank < start_mini_group_start+mini_group_size;
      bool in_end_mini = 
         my_rank >= end_mini_group_start && my_rank < comm_size-(isOdd?1:0);

      //Allocate the "normal" ranks
      if(my_rank >= start_rank && my_rank < end_mini_group_start
            && (!isOdd || my_rank != mid_rank)){
         //"effective" rank for determining which group I'm in and if I look
         //forward or backward for a partner.
         int e_rank = my_rank - start_rank;
         if(isOdd && my_rank > mid_rank)
            --e_rank; //We skip the middle rank when isOdd

         int my_partner;
         if(((e_rank/rank_separation)%2) == 0){
            //Look forward for partner.
            my_partner = my_rank + rank_separation;
            if(isOdd && my_rank < mid_rank && my_partner >= mid_rank)
               ++my_partner;
         } else {
            my_partner = my_rank - rank_separation;
            if(isOdd && my_rank > mid_rank && my_partner <= mid_rank)
               --my_partner;
         }

         partner_set.insert(my_partner);
      } else if(in_start_mini) {
         int e_rank = my_rank - start_mini_group_start;
         int partner = end_mini_group_start + e_rank;
         partner_set.insert(partner);
      } else if(in_end_mini) {
         int e_rank = my_rank - end_mini_group_start;
         int partner = start_mini_group_start + e_rank;
         partner_set.insert(partner);
      } else { 
         //Only things left are the three ranks that must be paired to handle
         //odd-sized comms
         partner_set.insert({0, mid_rank, comm_size-1});

         //my_rank should be one of the inserted ranks, or something in the
         //logic here is broken.
         assert(partner_set.size() == 3 || (comm_size==1 && partner_set.size()==1));
      }
   } else if(mode == 5){
      set_size = policy_vals[2];

      //User is responsible for giving values that "make sense" for set size and rank separation given a comm size.
      int my_set_pos = (my_rank/rank_separation)%set_size;
      for(int index = 0; index < set_size; index++){
        int partner = (comm_size + my_rank - 
                   rank_separation * (my_set_pos-index)
                  )%comm_size;
        partner_set.insert(partner);
      }
   }
   
   partners = {partner_set.begin(), partner_set.end()};
   
   //Make same MPI calls as reinit
   reinit(flag);
}

void Group::build_set_comm(){
   if(set_comm != MPI_COMM_NULL){
      MPI_Comm_free(&set_comm);
      set_comm = MPI_COMM_NULL;
   }

   MPI_Group comm_group, set_group;
   MPI_Comm_group(comm, &comm_group);
   MPI_Group_incl(comm_group, partners.size(), partners.data(), &set_group);
   MPI_Comm_create_group(comm, set_group, 0, &(set_comm));
   
   MPI_Group_free(&comm_group);
   MPI_Group_free(&set_group);

   MPI_Comm_size(set_comm, &set_size);
   MPI_Comm_rank(set_comm, &set_rank);
}

Member* Group::find_member(int memberid){
   auto iter = member_data.find(memberid);
   if(iter != member_data.end()) return iter->second.get();
   return nullptr;
}

std::string Group::str(){
   std::stringstream ss;
   ss << "Group " << groupid << " set ";
   ss << "[" << partners[0];
   for(int i=1; i<partners.size(); i++) ss << ", " << partners[i];
   ss << "]";

   return ss.str();
}

int Group::member_create(fenix_member_entry_t* mentry){
   auto iter = member_data.try_emplace(mentry->memberid, nullptr);
   if(iter.second){
      if(mode == 1)
         iter.first->second = std::make_shared<BuddyMember>(*mentry, *this);
      else if(mode == 5)
         iter.first->second = std::make_shared<ParityMember>(*mentry, *this);
      else assert(false);

      return FENIX_SUCCESS;
   } else return FENIX_ERROR_MEMBER_CREATE;
}

int Group::member_delete(int member_id){
   auto iter = member_data.find(member_id);

   if(iter == member_data.end()){
      debug_print("ERROR Fenix_Data_member_delete: member_id <%d> does not exist!\n",
                member_id);
      return FENIX_ERROR_INVALID_MEMBERID; 
   }

   member_data.erase(iter);
   return FENIX_SUCCESS;
}

int Group::member_store(int member_id, const DataSubset& subset){
   auto iter = member_data.find(member_id);
   if(iter == member_data.end()){
      debug_print(
         "ERROR Fenix_Data_member_store: %s unknown member_id %d on rank %d\n",
         this->str().c_str(), member_id, current_rank
      );
      return FENIX_ERROR_INVALID_MEMBERID;
   }
   return iter->second->store(subset);
}

int Group::member_storev(int member_id, const DataSubset& subset){
   auto iter = member_data.find(member_id);
   if(iter == member_data.end()){
      debug_print(
         "ERROR Fenix_Data_member_storev: %s unknown member_id %d on rank %d\n",
         this->str().c_str(), member_id, current_rank
      );
      return FENIX_ERROR_INVALID_MEMBERID;
   }
   return iter->second->storev(subset);
}

int Group::member_istore(
   int member_id, const DataSubset& subset, Fenix_Request *request
) {return 0;}

int Group::member_istorev(
   int member_id, const DataSubset& subset_specifier, Fenix_Request *request
) {return 0;}


int Group::commit(){
   if(timestamps.size() == depth+1){
      //Full of timestamps, remove the oldest and proceed as normal.
      timestamps.pop_front();
   }
   timestamps.push_back(timestamp);

   for(auto& iter : member_data){
      iter.second->commit(timestamp);
   }

   send_buf.clear();
   send_buf.shrink_to_fit();
   recv_buf.clear();
   recv_buf.shrink_to_fit();

   return FENIX_SUCCESS;
}


int Group::snapshot_delete(int to_delete){
   int retval = FENIX_SUCCESS;

   bool found = false;
   for(auto it = timestamps.begin(); it != timestamps.end(); it++){
      if(*it == to_delete){
         timestamps.erase(it);
         found = true;
         break;
      }
   }
   for(auto& iter : member_data){
      found |= iter.second->snapshot_delete(to_delete);
   }
   return found ? FENIX_SUCCESS : FENIX_ERROR_INVALID_TIMESTAMP;
}

int Group::barrier(){return 0;}

int Group::get_number_of_snapshots(int* num){
   *num = timestamps.size();
   return FENIX_SUCCESS;
}

int Group::get_snapshot_at_position(int idx, int* snapshot){
   if(idx >= timestamps.size() || idx < 0)
      return FENIX_ERROR_INVALID_POSITION;

   *snapshot = timestamps[idx];
   return FENIX_SUCCESS;
}

std::vector<int> Group::get_snapshots(){
  return {timestamps.begin(), timestamps.end()};
}

int Group::member_restore(
   int member_id, void* target_buffer, int max_count, int ts,
   DataSubset& data_found
){
   //TODO: Is this fix needed anymore?
   //One-time fix after a reinit.
   if(timestamp == -1 && !timestamps.empty())
      timestamp = timestamps.back();
   
   IMR::Member* member = find_member(member_id);

   std::vector<int> found_members(set_size);
   found_members[set_rank] = member ? 1 : 0;

   int allgather_ret = MPI_Allgather(
     MPI_IN_PLACE, 1, MPI_INT, found_members.data(), 1, MPI_INT, set_comm
   );

   int n_missing = 0;
   int first_found = -1, missing_rank = -1;
   for(int i = 0; i < found_members.size(); i++){
      if(!found_members[i]){
         n_missing++;
         missing_rank = i;
      }
      if(found_members[i] && first_found == -1)
         first_found = i;
   }

   if(n_missing > 1){
      if(set_rank != 0) return FENIX_ERROR_INVALID_MEMBERID;

      if(n_missing == set_size){
         debug_print(
            "ERROR Fenix_Data_member_restore: %s member_id %d not found\n",
            this->str().c_str(), member_id
         );
      } else {
         debug_print(
            "ERROR Fenix_Data_member_restore: %s member_id %d unrecoverable\n",
            this->str().c_str(), member_id
         );
      }
      return FENIX_ERROR_INVALID_MEMBERID;
   } else if(n_missing == 1){
      fenix_member_entry_packet_t packet;
      if(set_rank == first_found) packet = member->mentry.to_packet();

      MPI_Bcast(
         &packet, sizeof(packet), MPI_BYTE, first_found, set_comm
      );

      if(!found_members[set_rank]){
         this->member_create(__fenix_data_member_add_entry(
            this, packet.memberid, target_buffer, packet.current_count,
            packet.datatype_size
         ));
         member = find_member(member_id);
         assert(member);
      }
   }

   member->restore();

   send_buf.clear();
   send_buf.shrink_to_fit();
   recv_buf.clear();
   recv_buf.shrink_to_fit();

   return member->lrestore((char*)target_buffer, max_count, ts, data_found);
}

int Group::member_lrestore(
   int member_id, void* target_buffer, int max_count, int ts,
   DataSubset& data_found
){
   auto iter = member_data.find(member_id);
   if(iter == member_data.end()) return FENIX_ERROR_INVALID_MEMBERID;
   return iter->second->lrestore((char*)target_buffer, max_count, ts, data_found);
}


int Group::member_restore_from_rank(int member_id,
        void* target_buffer, int max_count, int timestamp, 
        int source_rank){return 0;}


int Group::member_get_attribute(fenix_member_entry_t* member, 
        int attributename, void* attributevalue, int* flag, int sourcerank){return 0;}

int Group::member_set_attribute(fenix_member_entry_t* member, 
           int attributename, void* attributevalue, int* flag){ 
  //No mutable attributes (as of now) require any changes to this policy's info
  return FENIX_SUCCESS;
}

int Group::reinit(int* flag){
  build_set_comm();
  sync_timestamps();

  *flag = FENIX_SUCCESS;
  return *flag;
}

void Group::sync_timestamps(){
   int n_snapshots = timestamps.size();
   
   MPI_Allreduce(MPI_IN_PLACE, &n_snapshots, 1, MPI_INT, MPI_MAX, set_comm);

   bool need_reset = timestamps.size() != n_snapshots;
   for(int i = timestamps.size(); i < n_snapshots; i++) timestamps.push_front(-1);

   std::vector<int> ts = {timestamps.begin(), timestamps.end()};
   MPI_Allreduce(
      MPI_IN_PLACE, ts.data(), n_snapshots, MPI_INT, MPI_MAX, set_comm
   );
   timestamps = {ts.begin(), ts.end()};
   
   if(!timestamps.empty()) timestamp = timestamps.back();
   else timestamp = -1;
}

int Group::get_redundant_policy(int* policy_name, void* policy_value, int* flag){
   *policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID;
   
   int* policy_vals = (int*) policy_value;
   policy_vals[0] = mode;
   policy_vals[1] = rank_separation;
   if(mode == 5) policy_vals[2] = set_size;

   *flag = FENIX_SUCCESS;
   return *flag;
}

int Group::group_delete(){
   delete this;
   return FENIX_SUCCESS;
}
} // namespace Fenix::IMR
