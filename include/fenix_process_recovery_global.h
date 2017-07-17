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
//        Rob Van der Wijngaart, and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/
#ifndef __FENIX_PROCES_RECOVERY_GLOBAL_H__
#define __FENIX_PROCES_RECOVERY_GLOBAL_H__

#include <mpi.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <signal.h>

#include "fenix_opt.h"
#include "fenix_util.h"
#include "fenix_data_group.h"


/* This header file is intended to provide global variable defintiions for fenix_process_recovery.c only */

int __fenix_g_num_inital_ranks;     // Keeps the global MPI rank ID at Fenix_init
int __fenix_g_num_survivor_ranks;   // Keeps the global information on the number of survived MPI ranks after failure
int __fenix_g_num_recovered_ranks;  // Keeps the number of spare ranks brought into MPI communicator recovery
int __fenix_g_resume_mode;          // Defines how program resumes after process recovery
int __fenix_g_spawn_policy;         // Indicate dynamic process spawning
int __fenix_g_spare_ranks;          // Spare ranks entered by user to repair failed ranks
int __fenix_g_replace_comm_flag;    // Internal global variable to describe the status of MPI communicator
int __fenix_g_repair_result;        // Internal global variable to store the result of MPI communicator repair
jmp_buf *__fenix_g_recover_environment; // Calling environment to fill the jmp_buf structure


//enum FenixRankRole __fenix_g_role;    // Role of rank: initial, survivor or repair
int __fenix_g_role;    // Role of rank: initial, survivor or repair
int __fenix_g_fenix_init_flag = 0;

#include "fenix_request_store.h"
__fenix_request_store_t __fenix_g_request_store;

fenix_callback_list_t* __fenix_g_callback_list;  // singly linked list for user-defined Fenix callback functions
//fenix_communicator_list_t* __fenix_g_communicator_list;  // singly linked list for Fenix resilient communicators
__fenix_debug_options __fenix_options;    // This is reserved to store the user options

MPI_Comm *__fenix_g_world;                // Duplicate of the MPI communicator provided by user
MPI_Comm *__fenix_g_new_world;            // Global MPI communicator identical to g_world but without spare ranks
MPI_Comm *__fenix_g_user_world;           // MPI communicator with repaired ranks
MPI_Comm __fenix_g_original_comm;         // Keep the information of the original global MPI Communicator (this will be umodified until Fenix_finalize)
MPI_Op   __fenix_g_agree_op;              // This is reserved for the global agreement call for Fenix data recovery API

fenix_group_t *__fenix_g_data_recovery;   // Global pointer for Fenix Data Recovery Data Structure
#endif // __FENIX_PROCES_RECOVERY_GLOBAL_H__
