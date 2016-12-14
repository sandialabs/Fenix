
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

#ifndef __DATA_RECOVERY__
#define __DATA_RECOVERY__

#include "util.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#define __FENIX_COMMIT_MODE     1
#define __GROUP_ENTRY_ATTR_SIZE 5
#define __NUM_MEMBER_ATTR_SIZE  3
#define __GRP_MEMBER_LENTRY_ATTR_SIZE 11

#define __FENIX_DEFAULT_GROUP_SIZE        2
#define __FENIX_DEFAULT_MEMBER_SIZE       5
#define __FENIX_DEFAULT_VERSION_SIZE      3

#define __FENIX_SUBSET_EMPTY   1
#define __FENIX_SUBSET_FULL    2
#define __FENIX_SUBSET_CREATE  3
#define __FENIX_SUBSET_CREATEV 4

#define STORE_RANK_TAG  2000
#define STORE_COUNT_TAG 2001
#define STORE_SIZE_TAG  2002
#define STORE_DATA_TAG  2003
#define STORE_PAYLOAD_TAG  2004

#define PARTNER_STATUS_TAG       1900
#define RECOVER_GROUP_TAG        1901
#define RECOVER_GROUP_ENTRY_TAG  1902
#define RECOVER_MEMBER_TAG       1903
#define RECOVER_MEMBER_ENTRY_TAG 1904
#define RECOVERY_VERSION_TAG     1905
#define RECOVER_SIZE_TAG         1906
#define RECOVER_DATA_TAG         1907

typedef struct __fenix_remote_entry {
    int remoterank;
    int count;
    size_t size;
    MPI_Datatype datatype;
    void *pdata;
    void *data;
} fenix_remote_entry_t;

typedef struct __fenix_local_entry {
    int currentrank;
    int count;
    size_t size;
    MPI_Datatype datatype;
    void *pdata;
    void *data;
} fenix_local_entry_t;

typedef struct __fenix_version {
    size_t num_copies;
    /* Number of copies            */
    size_t count;
    /* Number of versions          */
    size_t size;
    /* Size of bucket              */
    size_t position;
    /* Position of current version */
    fenix_local_entry_t *local_entry;
    fenix_remote_entry_t *remote_entry;
} fenix_version_t;

typedef struct __fenix_member_entry {
    int memberid;
    enum states state;
    fenix_version_t version;
    void *current_buf;
    MPI_Datatype current_datatype;
    int size_datatype;
    int current_count;
    int current_size;
    int currentrank;
    int remoterank;
    int remoterank_front;
    int remoterank_back;
} fenix_member_entry_t;

typedef struct __fenix_member {
    size_t count;
    int temp_count;
    size_t size;
    fenix_member_entry_t *member_entry;
} fenix_member_t;

typedef struct __fenix_group_entry {
    int groupid;
    MPI_Comm comm;
    int comm_size;
    int current_rank;
    int in_rank;
    int out_rank;
    int timestart;
    int timestamp;
    int depth;
    int rank_separation;
    /* Subject to change */
    enum states state;
    int recovered;
    fenix_member_t member;
} fenix_group_entry_t;

typedef struct __fenix_group {
    size_t count;
    size_t size;
    fenix_group_entry_t *group_entry;
} fenix_group_t;

typedef struct __member_store_packet {
    int rank;
    MPI_Datatype datatype;
    int entry_count;
    size_t entry_size;
    int entry_real_count;
    int num_blocks;

} member_store_packet_t;

typedef struct __fenix_subset_offsets  {
    size_t start;
    size_t end;
} fenix_subset_offsets_t ;

typedef struct __two_container_packet {
    size_t count;
    size_t size;
} two_container_packet_t;

typedef struct __container_packet {
    size_t count;
    size_t size;
    size_t position;
    size_t num_copies;
} container_packet_t;

typedef struct __group_entry_packet {
    int groupid;
    int timestamp;
    int depth;
    int rank_separation;
    enum states state;
} group_entry_packet_t;

typedef struct __member_entry_packet {
    int memberid;
    enum states state;
    MPI_Datatype current_datatype;
    int size_datatype;
    int current_count;
    int current_size;
    int currentrank;
    int remoterank;
    int remoterank_front;
    int remoterank_back;
} member_entry_packet_t;

typedef struct __data_entry_packet {
    MPI_Datatype datatype;
    int count;
    int size;
} data_entry_packet_t;

extern int *rank_roles;
fenix_group_t *g_data_recovery;
int store_counter;

int group_create(int, MPI_Comm, int, int);
int member_create(int, int, void *, int, MPI_Datatype);
int group_get_redundancy_policy(int, int, void *, int *);
int group_set_redundancy_policy(int, int, void *, int *);
int data_wait(Fenix_Request);
int data_test(Fenix_Request, int *);
int member_store(int, int, Fenix_Data_subset);
int member_storev(int, int, Fenix_Data_subset);
int member_istore(int, int, Fenix_Data_subset, Fenix_Request *);
int member_istorev(int, int, Fenix_Data_subset, Fenix_Request *);
int data_commit(int, int *);
int data_commit_barrier(int, int *);
int data_barrier(int);
int member_restore(int, int, void *, int, int);
int member_restore_from_rank(int, int, void *, int, int, int);
int data_subset_create(int, int, int, int, Fenix_Data_subset *);
int data_subset_createv(int, int *, int *, Fenix_Data_subset *);
int data_subset_delete(Fenix_Data_subset *);
int get_number_of_members(int, int *);
int get_member_at_position(int, int *, int);
int get_number_of_snapshots(int, int *);
int get_snapshot_at_position(int, int, int *);
int member_get_attribute(int, int, int, void *, int *, int);
int member_set_attribute(int, int, int, void *, int *);
int snapshot_delete(int groupid, int timestamp);
int group_delete(int);
int member_delete(int, int);

void init_data_recovery();
void init_partner_copy_recovery();
fenix_group_t *init_group();
fenix_member_t *init_member();
fenix_version_t *init_version();
fenix_local_entry_t *init_local();
fenix_remote_entry_t *init_remote();
void free_local(fenix_local_entry_t *);
void free_remote(fenix_remote_entry_t *);
void reinit_group(fenix_group_t *, two_container_packet_t);
void reinit_version(fenix_version_t *, container_packet_t);
void reinit_member(fenix_member_t *, two_container_packet_t, enum states);
void ensure_group_capacity(fenix_group_t *);
void ensure_member_capacity(fenix_member_t *);
void ensure_version_capacity(fenix_member_t *);
int search_groupid(int);
int search_memberid(int, int);
int find_next_group_position(fenix_group_t *);
int find_next_member_position(fenix_member_t *);
int join_group(fenix_group_t *, fenix_group_entry_t *, MPI_Comm);
int join_member(fenix_member_t *, fenix_member_entry_t *, MPI_Comm);
int join_restore(fenix_group_entry_t *, fenix_version_t *, MPI_Comm);
int join_commit(fenix_group_entry_t *, fenix_version_t *, MPI_Comm);
fenix_local_entry_t *subset_full(fenix_member_entry_t *);
void subset(fenix_group_entry_t *, fenix_member_entry_t *, Fenix_Data_subset *);
fenix_local_entry_t *subset_variable(fenix_member_entry_t *, Fenix_Data_subset *);
int _send_metadata(int, int, MPI_Comm);
int _recover_metadata(int, int, MPI_Comm);
int _send_group_data(int, int, fenix_group_entry_t *, MPI_Comm);
int _recover_group_data(int, int, fenix_group_entry_t *, MPI_Comm);
int _pc_send_member_metadata(int, int, fenix_member_entry_t *, MPI_Comm);
int _pc_recover_member_metadata(int, int, fenix_member_entry_t *, MPI_Comm);
int _pc_send_members(int, int, int, fenix_member_t *, MPI_Comm);
int _pc_recover_members(int, int, int, fenix_member_t *, MPI_Comm);
int _pc_send_member_entries(int, int, int, fenix_version_t *, MPI_Comm);
int _pc_recover_member_entries(int, int, int, fenix_version_t *, MPI_Comm);
void __fenix_dr_print_store();
void __fenix_dr_print_restore();
void __fenix_dr_print_datastructure();
void store_single();
void store_all();

#endif
