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
//        Rob Van der Wijngaart, and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#ifndef __FENIX_PROCESS_RECOVERY__
#define __FENIX_PROCESS_RECOVERY__

#include <mpi.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <signal.h>

#define __FENIX_RESUME_AT_INIT 0 
#define __FENIX_RESUME_NO_JUMP 200

typedef void (*recover)( MPI_Comm, int, void *);

typedef struct fcouple {
    recover x;
    void *y;
} fenix_callback_func;

typedef struct __fenix_callback_list {
    fenix_callback_func *callback;
    struct __fenix_callback_list *next;
} fenix_callback_list_t;

typedef struct __fenix_comm_list_elm {
  struct __fenix_comm_list_elm *next;
  struct __fenix_comm_list_elm *prev;
  MPI_Comm *comm;
} fenix_comm_list_elm_t;

typedef struct {
  fenix_comm_list_elm_t *head;
  fenix_comm_list_elm_t *tail;
} fenix_comm_list_t;

int __fenix_preinit(int *, MPI_Comm, MPI_Comm *, int *, char ***, int, int, MPI_Info, int *, jmp_buf *);

int __fenix_create_new_world();

int __fenix_repair_ranks();

int __fenix_callback_register(void (*recover)(MPI_Comm, int, void *), void *);

void __fenix_callback_push(fenix_callback_list_t **, fenix_callback_func *);

void __fenix_callback_invoke_all(int error);

int __fenix_callback_destroy(fenix_callback_list_t *callback_list);

int* __fenix_get_fail_ranks(int *, int, int);

int __fenix_spare_rank();

int __fenix_get_rank_role();

void __fenix_set_rank_role(int FenixRankRole);

void __fenix_postinit(int *);

void __fenix_finalize();

void __fenix_finalize_spare();

void __fenix_test_MPI(MPI_Comm*, int*, ...);

#endif
