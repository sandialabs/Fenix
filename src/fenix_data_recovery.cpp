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



#include "fenix.hpp"
#include "fenix_data_recovery.hpp"
#include "fenix_data_policy.hpp"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"
#include "fenix_ext.hpp"
#include "fenix_data_subset.hpp"

#include <mpi-ext.h>

namespace Fenix::Data {

/**
 * @brief           create new group or recover group data for lost processes
 * @param groud_id  
 * @param comm
 * @param time_start
 * @param depth
 */
int group_create( int groupid, MPI_Comm comm, int timestart, int depth, int policy_name, 
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
      group->reinit(flag);
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
    retval = group->get_redundant_policy(policy_name, policy_value, flag);
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
int member_create(
  int groupid, int memberid, void *data, int count, MPI_Datatype datatype
) {
  auto [group_index, group] = find_group(groupid);
  if(!group) return FENIX_ERROR_INVALID_GROUPID;

  auto [member_index, mentry] = group->search_member(memberid);
  if(mentry){
    debug_print("ERROR Fenix_Data_member_create: member_id <%d> already exists\n",
                memberid);
    return FENIX_ERROR_INVALID_MEMBERID;
  }

  if (fenix.options.verbose == 13) {
    verbose_print("c-rank: %d, group_index: %d, member_index: %d\n",
                   __fenix_get_current_rank(fenix.new_world),
                  group_index, member_index);
  }

  //First, we'll make a fenix-core member entry, then pass that info to
  //the specific data policy.
  mentry = __fenix_data_member_add_entry(group, memberid, data, count, __fenix_get_size(datatype));

  //Pass the info along to the policy
  return group->member_create(mentry);
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

int member_store(int groupid, int memberid, const DataSubset& specifier) {
  auto [group_index, group] = find_group(groupid);
  if(!group){
    debug_print("ERROR Fenix_Data_member_store: group_id <%d> does not exist", groupid);
    return FENIX_ERROR_INVALID_GROUPID;
  }
  
  return group->member_store(memberid, specifier);
}

int member_storev(int groupid, int memberid, const DataSubset& specifier) {
  auto [group_index, group] = find_group(groupid);
  if(!group){
    debug_print("ERROR Fenix_Data_member_storev: group_id <%d> does not exist", groupid);
    return FENIX_ERROR_INVALID_GROUPID;
  }
  
  return group->member_storev(memberid, specifier);
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param subset_specifier
 * @param request
 */
int member_istore(int groupid, int memberid, const DataSubset& specifier,
                  Fenix_Request *request) {
  fatal_print("unimplemented");
  auto [group_index, group] = find_group(groupid);
  if(!group){
    debug_print("ERROR Fenix_Data_member_istore: group_id <%d> does not exist", groupid);
    return FENIX_ERROR_INVALID_GROUPID;
  }

  return group->member_istore(memberid, specifier, request);
}

int member_istorev(
  int groupid, int memberid, const DataSubset& specifier, Fenix_Request *request
) {
  fatal_print("unimplemented");
  auto [group_index, group] = find_group(groupid);
  if(!group){
    debug_print("ERROR Fenix_Data_member_istore: group_id <%d> does not exist", groupid);
    return FENIX_ERROR_INVALID_GROUPID;
  }
  
  return group->member_istore(memberid, specifier, request);
}


/**
 * @brief
 * @param group_id
 * @param time_stamp
 */
int commit(int groupid, int *timestamp) {
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
    
    if (group->timestamp != -1) group->timestamp++;
    else group->timestamp = group->timestart;
    
    group->commit();
    
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
int commit_barrier(int groupid, int *timestamp) {
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
   
    //We want to make sure there aren't any failed MPI operations (IE unfinished stores)
    //But we don't want to fail to commit if a failure has happened since a successful store.
    int old_failure_handling = fenix.ignore_errs;
    fenix.ignore_errs = 1;

    int can_commit = 0;

    //We'll use comm_agree as a resilient barrier
    //Our error handler also enters an agree, with a unique location bit set.
    //So if we aren't all here, we've hit an error already.

    int location = FENIX_DATA_COMMIT_BARRIER_LOC;
    int ret = MPIX_Comm_agree(*fenix.user_world, &location);
    if(location == FENIX_DATA_COMMIT_BARRIER_LOC) can_commit = 1;

    fenix.ignore_errs = old_failure_handling;

    if(can_commit == 1){
        if (group->timestamp != -1) group->timestamp++;
        else group->timestamp = group->timestart;
        retval = group->commit();
    }


    if(can_commit != 1 || ret != MPI_SUCCESS) {
        //A rank failure has happened, lets trigger error handling if enabled.
        int throwaway = 1;
        MPI_Allreduce(MPI_IN_PLACE, &throwaway, 1, MPI_INT, MPI_SUM, *fenix.user_world);
    }

    
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
int member_restore(int groupid, int memberid, void *data, int maxcount, int timestamp, DataSubset& data_found) {
  int retval =  FENIX_SUCCESS;
  data_found = {};

  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery);

  if (fenix.options.verbose == 25) {
    int member_index = -1;
    if(group_index != -1) member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index], memberid);
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
    retval = group->member_restore(memberid, data, maxcount, timestamp, data_found);
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
int member_lrestore(int groupid, int memberid, void *data, int maxcount, int timestamp, DataSubset& data_found) {
  int retval =  FENIX_SUCCESS;
  data_found = {};

  int group_index = __fenix_search_groupid(groupid, fenix.data_recovery);
  int member_index = -1;

  if(group_index != -1) member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index], memberid);


  if (fenix.options.verbose == 25) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }

  if (group_index == -1) {
    debug_print("ERROR Fenix_Data_member_lrestore: group_id <%d> does not exist\n",
                groupid);
    retval = FENIX_ERROR_INVALID_GROUPID;
  } else {
    fenix_group_t *group = (fenix.data_recovery->group[group_index]);
    retval = group->member_lrestore(memberid, data, maxcount, timestamp, data_found);
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
  
  if(group_index != -1) member_index = __fenix_search_memberid(fenix.data_recovery->group[group_index], memberid);

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
    retval = group->member_restore_from_rank(memberid, target_buffer, 
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
  auto group = find_group(group_id).second;
  if(!group) return FENIX_ERROR_INVALID_GROUPID;

  *num_members = group->members.size();
  return FENIX_SUCCESS;
}

/**
 * @brief
 * @param group_id
 * @param member_id
 * @param position
 */
int __fenix_get_member_at_position(int group_id, int *member_id, int position) {
  auto [group_index, group] = find_group(group_id);
  if(!group) return FENIX_ERROR_INVALID_GROUPID;

  if(position < 0 || position >= group->members.size()){
    debug_print(
            "ERROR Fenix_Data_group_get_member_at_position: position <%d> must be a value between 0 and number_of_members-1 \n",
            position);
    return FENIX_ERROR_INVALID_POSITION;
  }
  auto iter = group->members.begin();
  std::advance(iter, position);
  *member_id = iter->first;
  return FENIX_SUCCESS;
}

std::optional<std::vector<int>> group_members(int group_id){
  auto [group_index, group] = find_group(group_id);
  if(!group) return {};
  return group->get_member_ids();
}

std::optional<std::vector<int>> group_snapshots(int group_id){
  auto [group_index, group] = find_group(group_id);
  if(!group) return {};
  return group->get_snapshots();
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
    retval = group->get_number_of_snapshots(num_snapshots);
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
  auto [group_index, group] = find_group(groupid);
  if(!group) return FENIX_ERROR_INVALID_GROUPID;

  auto [member_index, mentry] = group->find_member(memberid);
  if(!mentry) return FENIX_ERROR_INVALID_MEMBERID;

  if (fenix.options.verbose == 34) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }

  return group->member_get_attribute(mentry, attributename,
          attributevalue, flag, sourcerank);
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
  auto [group_index, group] = find_group(groupid);
  if(!group) return FENIX_ERROR_INVALID_GROUPID;

  auto [member_index, mentry] = group->find_member(memberid);
  if(!mentry) return FENIX_ERROR_INVALID_MEMBERID;

  if (fenix.options.verbose == 35) {
    verbose_print("c-rank: %d, role: %d, group_index: %d, member_index: %d\n",
                    __fenix_get_current_rank(fenix.new_world), fenix.role, group_index,
                  member_index);
  }
  
  int my_datatype_size;
  int myerr;

  //Always pass attribute changes along to group - they might have unknown attributes
  //or side-effects to handle from changes. They get change info before
  //changes are made, in case they need prior state.
  int retval = group->member_set_attribute(mentry, attributename,
          attributevalue, flag);

  switch (attributename) {
    case FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER:
      mentry->user_data = (char*)attributevalue;
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

  return retval;
}

/**
 * @brief
 * @param group_id
 * @param time_stamp
 */
int snapshot_delete(int group_id, int time_stamp) {
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
    retval = group->snapshot_delete(time_stamp);
  }
  return retval;
}

} // namespace Fenix::Data
