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
#ifndef __FENIX_DATA_MEMBER_H__
#define __FENIX_DATA_MEMBER_H__

#include <mpi.h>
#include "fenix_data_version.h"
#include "fenix_data_packet.h"
#include "fenix_util.h"


#define __FENIX_DEFAULT_MEMBER_SIZE 512

typedef struct __fenix_member_entry {
    int memberid;
    enum states state;
    fenix_version_t *version;
    void *user_data;
    MPI_Datatype current_datatype;
    int datatype_size;
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
    size_t total_size;
    fenix_member_entry_t *member_entry;
} fenix_member_t;

typedef struct __member_store_packet {
    int rank;
    MPI_Datatype datatype;
    int entry_count;
    size_t entry_size;
    int entry_real_count;
    int num_blocks;

} fenix_member_store_packet_t;

typedef struct __member_entry_packet {
    int memberid;
    enum states state;
    MPI_Datatype current_datatype;
    int datatype_size;
    int current_count;
    int current_size;
    int currentrank;
    int remoterank;
    int remoterank_front;
    int remoterank_back;
} fenix_member_entry_packet_t;

fenix_member_t *__fenix_data_member_init( );
void __fenix_data_member_destroy( fenix_member_t *member ) ;

void __fenix_ensure_member_capacity( fenix_member_t *m );
void __fenix_ensure_version_capacity( fenix_member_t *m ) ;

void __fenix_data_member_reinit(fenix_member_t *m, fenix_two_container_packet_t packet,
                   enum states mystatus);
#endif // FENIX_DATA_MEMBER_H
