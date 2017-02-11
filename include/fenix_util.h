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

#ifndef __FENIX_UTIL__
#define __FENIX_UTIL__

#include "fenix.h"
#include "fenix_process_recovery.h"
#include <mpi.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/times.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <signal.h>
#include <libgen.h>

char *logname;

#define LDEBUG(f...)  {LLIND("debug",f);}
#define LLIND(t,f...) {fprintf(stderr,"%s - %s (%i): %s: \n",logname,__PRETTY_FUNCTION__,getpid(),t); fprintf(stderr,f);}
#define ERRHANDLE(f...){LFATAL(f);}
#define LFATAL(f...)  {LLINF("fatal", f);}
#define LLINF(t,f...) {fprintf(stderr,"(%i): %s: ", getpid(), t); fprintf(stderr, f);}

enum states { EMPTY = 0, OCCUPIED = 1, DELETED = 2, NEEDFIX = 3 };

void __fenix_ranks_agree(int *, int *, int *, MPI_Datatype *);

int __fenix_binary_search(int *, int, int);

int __fenix_comparator(const void *, const void *);

int __fenix_get_size(MPI_Datatype);

int __fenix_get_fenix_default_rank_separation();

int __fenix_get_current_rank(MPI_Comm);

int __fenix_get_partner_rank(int, MPI_Comm);

int __fenix_get_world_size(MPI_Comm);

int __fenix_mpi_wait(MPI_Request *);

int __fenix_mpi_test(MPI_Request *);

void __fenix_callback_push(struct callback_list **, fenix_callback_func *);

void *s_calloc(int count, size_t size);

void *s_malloc(size_t size);

void *s_realloc(void *mem, size_t size);

#endif
