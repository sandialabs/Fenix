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

#include "fenix_data_recovery.h"
#include "fenix_process_recovery.h"
#include "fenix_util.h"
#include "fenix_ext.h"

const Fenix_Data_subset  FENIX_DATA_SUBSET_FULL = {0, NULL, NULL, 0, 2};
const Fenix_Data_subset  FENIX_DATA_SUBSET_EMPTY = {0, NULL, NULL, 0, 1};

/*
int Fenix_Callback_register(void (*recover)(MPI_Comm, int, void *), void *callback_data) {
    return __fenix_callback_register(recover, callback_data);
}

int Fenix_Initialized(int *flag) {
    *flag = (fenix.fenix_init_flag) ? 1 : 0;
    return FENIX_SUCCESS;
}

int Fenix_Finalize() {
    __fenix_finalize();
    return FENIX_SUCCESS;
}
*/

int Fenix_Data_group_create( int group_id, MPI_Comm comm, int start_time_stamp, int depth ) {
    //return __fenix_group_create(group_id, comm, start_time_stamp, depth);
    return __Fenix_Data_group_create(group_id, comm, start_time_stamp, depth);
}

int Fenix_Data_member_create( int group_id, int member_id, void *buffer, int count, MPI_Datatype datatype ) {
    //return __fenix_member_create(group_id, member_id, buffer, count, datatype);
    return __Fenix_Data_member_create(group_id, member_id, buffer, count, datatype);
}

/*int Fenix_Data_group_get_redundancy_policy( int group_id, int policy_name, void *policy_value, int *flag ) {
    return __fenix_group_get_redundancy_policy( group_id, policy_name, policy_value, flag );
}

int Fenix_Data_group_set_redundancy_policy( int group_id, int policy_name, void *policy_value, int *flag ) {
    return  __fenix_group_set_redundancy_policy( group_id, policy_name, policy_value, flag);
}

int Fenix_Data_wait(Fenix_Request request) {
    return __fenix_data_wait(request);
}

int Fenix_Data_test(Fenix_Request request, int *flag) {
    return __fenix_data_test(request, flag);
}*/

//int Fenix_Data_member_store(int group_id, int member_id, Fenix_Data_subset subset_specifier) {
int Fenix_Data_member_store(int group_id, int member_id) {
    //return __fenix_member_store(group_id, member_id, subset_specifier);
    return __Fenix_Data_member_store(group_id, member_id);
}

/*int Fenix_Data_member_storev(int group_id, int member_id, Fenix_Data_subset subset_specifier) {
    return 0;
}

int Fenix_Data_member_istore(int group_id, int member_id, Fenix_Data_subset subset_specifier, Fenix_Request *request) {
    return 0;
}

int Fenix_Data_member_istorev(int group_id, int member_id, Fenix_Data_subset subset_specifier, Fenix_Request *request) {
    return 0;
}

int Fenix_Data_commit(int group_id, int *time_stamp) {
    return __fenix_data_commit(group_id, time_stamp);
}*/

int Fenix_Data_commit_barrier(int group_id, int *time_stamp) {
    return __Fenix_Data_commit_barrier(group_id, time_stamp);
}

/*
int Fenix_Data_barrier(int group_id) {
    return 0;
}*/

/*int Fenix_Data_member_restore(int group_id, int member_id, void *target_buffer, int max_count, int time_stamp) {
    return __Fenix_Data_member_restore(group_id, member_id, target_buffer, max_count, time_stamp);
}*/
int Fenix_Data_member_restore(int group_id, int member_id, void *target_buffer, int max_count, MPI_Datatype datatype, int time_stamp) {
    return __Fenix_Data_member_restore(group_id, member_id, target_buffer, max_count, datatype, time_stamp);
}

/*int Fenix_Data_member_resore_from_rank(int group_id, int member_id, void *target_buffer, int max_count, int time_stamp, int source_rank) {
    return 0;
}

int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset, int stride, Fenix_Data_subset *subset_specifier) {
    return __fenix_data_subset_create(num_blocks, start_offset, end_offset, stride, subset_specifier);
}

int Fenix_Data_subset_createv(int num_blocks, int *array_start_offsets, int *array_end_offsets, Fenix_Data_subset *subset_specifier) {
    return __fenix_data_subset_createv(num_blocks, array_start_offsets, array_end_offsets, subset_specifier);
}

int Fenix_Data_subset_delete(Fenix_Data_subset *subset_specifier) {
    return 0;
}

int Fenix_Data_group_get_number_of_members(int group_id, int *number_of_members) {
    return 0;
}

int Fenix_Data_group_get_member_at_position(int group_id, int *member_id, int position) {
    return 0;
}

int Fenix_Data_group_get_number_of_snapshots(int group_id, int *number_of_snapshots) {
    return __fenix_get_number_of_snapshots(group_id, number_of_snapshots);
}

int Fenix_Data_group_get_snapshot_at_position(int group_id, int position, int *time_stamp) {
    return __fenix_get_snapshot_at_position(group_id, position, time_stamp);
}

int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename, void *attributevalue, int *flag, int source_rank) {
    return __fenix_member_get_attribute(group_id, member_id, attributename, attributevalue, flag, source_rank);
}*/

int Fenix_Data_member_attr_set(int group_id, int member_id, int attribute_name, void *attribute_value, int *flag) {
    return __fenix_member_set_attribute(group_id, member_id, attribute_name, attribute_value, flag);
}

/*int Fenix_Data_snapshot_delete(int group_id, int time_stamp) {
    return __fenix_snapshot_delete(group_id, time_stamp);
}*/

int Fenix_Data_group_delete(int group_id) {
    //return __fenix_group_delete(group_id);
    return __Fenix_Data_group_delete(group_id);
}

int Fenix_Data_member_delete(int group_id, int member_id) {
    //return __fenix_member_delete(group_id, member_id);
    return __Fenix_Data_member_delete(group_id, member_id);
}
