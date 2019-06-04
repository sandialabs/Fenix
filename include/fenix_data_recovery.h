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

#ifndef __FENIX_DATA_RECOVERY__
#define __FENIX_DATA_RECOVERY__


#include "fenix_data_group.h"
#include "fenix_data_member.h"
#include "fenix_data_subset.h"
#include "fenix_util.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#define __FENIX_COMMIT_MODE     1
#define __GROUP_ENTRY_ATTR_SIZE 4
#define __NUM_MEMBER_ATTR_SIZE  3
#define __GRP_MEMBER_LENTRY_ATTR_SIZE 11






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







typedef struct __data_entry_packet {
    MPI_Datatype datatype;
    int count;
    int datatype_size;
} fenix_data_entry_packet_t;


int store_counter;

int __fenix_group_create(int, MPI_Comm, int, int, int, void*, int*);
int __fenix_group_get_redundancy_policy(int, int*, int*, int*);
int __fenix_member_create(int, int, void *, int, MPI_Datatype);
int __fenix_data_wait(Fenix_Request);
int __fenix_data_test(Fenix_Request, int *);
int __fenix_member_store(int, int, Fenix_Data_subset);
int __fenix_member_storev(int, int, Fenix_Data_subset);
int __fenix_member_istore(int, int, Fenix_Data_subset, Fenix_Request *);
int __fenix_member_istorev(int, int, Fenix_Data_subset, Fenix_Request *);
int __fenix_data_commit(int, int *);
int __fenix_data_commit_barrier(int, int *);
int __fenix_data_barrier(int);
int __fenix_member_restore(int, int, void *, int, int);
int __fenix_member_restore_from_rank(int, int, void *, int, int, int);
int __fenix_get_number_of_members(int, int *);
int __fenix_get_member_at_position(int, int *, int);
int __fenix_get_number_of_snapshots(int, int *);
int __fenix_get_snapshot_at_position(int, int, int *);
int __fenix_member_get_attribute(int, int, int, void *, int *, int);
int __fenix_member_set_attribute(int, int, int, void *, int *);
int __fenix_snapshot_delete(int groupid, int timestamp);

int __fenix_group_delete(int);
int __fenix_member_delete(int, int);

void __fenix_init_data_recovery();
void __fenix_init_partner_copy_recovery();


void __fenix_dr_print_store();
void __fenix_dr_print_restore();
void __fenix_dr_print_datastructure();
void __fenix_store_single();
void __fenix_store_all();

#endif
