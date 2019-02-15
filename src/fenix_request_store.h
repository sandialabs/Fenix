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



#ifndef __FENIX_REQUEST_STORE_H__
#define __FENIX_REQUEST_STORE_H__

#include <mpi.h>
#include <string.h>
#include <stdio.h>

#include "fenix_stack.h"

/* 

   MPI_REQUEST_NULL = 0;

   user MPI_Irecv(&req);
   fenix PMPI_Irecv(&req);   
   MPI returns req=222;
   fenix store_add(222);
   store returns "-123";
   fenix returns req=-123;

   user MPI_Wait(-123);
   fenix store_get(-123);
   store_get {req_id = 0; req=222;}
   store_get returns req=222;
   fenix PMPI_Wait(222);

 */


typedef struct {
    char valid;
    MPI_Request r;
} __fenix_request_t;

#define __fenix_dynamic_array_type      __fenix_request_t
#define __fenix_dynamic_array_typename  req
#include "fenix_dynamic_array.h"
#undef __fenix_dynamic_array_type
#undef __fenix_dynamic_array_typename

typedef struct {
    __fenix_req_dynamic_array_t reqs; // list of requests
    int first_unused_position;        // first position in 'reqs' that has never been used
    __fenix_stack_t freed_list;       // list of positions in 'reqs' that are not used anymore
} fenix_request_store_t;

static inline
void __fenix_request_store_init(fenix_request_store_t *s)
{
    s->first_unused_position = 0;
    __fenix_int_stack_init(&(s->freed_list), 100);
    __fenix_req_dynamic_array_init(&(s->reqs), 500);
}

static inline
void __fenix_request_store_destroy(fenix_request_store_t *s)
{
    int valid_count = 0, i;
    for(i=0 ; i<s->first_unused_position ; i++)
        if(s->reqs.elements[i].valid) valid_count++;
    if(valid_count > 0)
        printf("[Fenix warning] __fenix_request_store_destroy. store contains valid elements (valid elems %d, first_unused_pos %d)\n", valid_count, s->first_unused_position);
    __fenix_req_dynamic_array_destroy(&(s->reqs));
    __fenix_int_stack_destroy(&(s->freed_list));
    s->first_unused_position = 0;
}

// returns request_id (i.e. position in the s->reqs.elements array)
static inline
int __fenix_request_store_add(fenix_request_store_t *s, 
                              MPI_Request *r)
{
    assert(*r != MPI_REQUEST_NULL);
    int position = -1;
    if(s->freed_list.size > 0) {
        position = __fenix_int_stack_pop(&(s->freed_list));
    } else {
        position = s->first_unused_position++;
    }
    assert(position >= 0);
    __fenix_req_dynamic_array_inc(&(s->reqs));
    __fenix_request_t *f = &(s->reqs.elements[position]);
    assert(!f->valid);
    memcpy(&(f->r), r, sizeof(MPI_Request));
    f->valid = 1;

    // Cannot return a position that is equivalent to MPI_REQUEST_NULL
    MPI_Request r_test;
    *((int *)&r_test) = position;
    if(r_test == MPI_REQUEST_NULL) {
        position = -123;
	{
	    *((int *)&r_test) = position;
	    assert(r_test != MPI_REQUEST_NULL);
	}
    }
    return position;
}

static inline
void __fenix_request_store_get(fenix_request_store_t *s, 
                               int request_id,
                               MPI_Request *r)
{
    {
        MPI_Request r_test;
        *((int *)&r_test) = request_id;
        assert(r_test != MPI_REQUEST_NULL);
    }
    if(request_id == -123) {
        MPI_Request r_test = MPI_REQUEST_NULL;
        request_id = *((int*) &r_test);
    }
    __fenix_request_t *f = &(s->reqs.elements[request_id]);
    assert(f->valid);
    memcpy(r, &(f->r), sizeof(MPI_Request));
    assert(*r != MPI_REQUEST_NULL);
}

static inline
void __fenix_request_store_remove(fenix_request_store_t *s, 
                                  int request_id)
{
    {
        MPI_Request r_test;
        *((int *)&r_test) = request_id;
        assert(r_test != MPI_REQUEST_NULL);
    }
    if(request_id == -123) {
        MPI_Request r_test = MPI_REQUEST_NULL;
        request_id = *((int*) &r_test);
    }
    __fenix_request_t *f = &(s->reqs.elements[request_id]);
    assert(f->valid);
    f->valid = 0;

    __fenix_int_stack_push(&(s->freed_list), request_id);
}


static inline
void __fenix_request_store_getremove(fenix_request_store_t *s, 
				     int request_id,
				     MPI_Request *r)
{
    {
        MPI_Request r_test;
        *((int *)&r_test) = request_id;
        assert(r_test != MPI_REQUEST_NULL);
    }
    if(request_id == -123) {
        MPI_Request r_test = MPI_REQUEST_NULL;
        request_id = *((int*) &r_test);
    }
    __fenix_request_t *f = &(s->reqs.elements[request_id]);
    assert(f->valid);
    memcpy(r, &(f->r), sizeof(MPI_Request));
    assert(*r != MPI_REQUEST_NULL);
    f->valid = 0;
    __fenix_int_stack_push(&(s->freed_list), request_id);
}

void __fenix_request_store_waitall_removeall(fenix_request_store_t *s);

#endif // __FENIX_REQUEST_STORE_H__
