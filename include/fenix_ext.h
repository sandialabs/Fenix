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

#ifndef __FENIX_EXT_H__
#define __FENIX_EXT_H__

#include <mpi.h>
#include "fenix.h"
#include "fenix_opt.h"
#include "fenix_data_group.h"
#include "fenix_process_recovery.h"

typedef struct {
    int num_inital_ranks;     // Keeps the global MPI rank ID at Fenix_init
    int num_survivor_ranks;   // Keeps the global information on the number of survived MPI ranks after failure
    int num_recovered_ranks;  // Keeps the number of spare ranks brought into MPI communicator recovery
    int resume_mode;          // Defines how program resumes after process recovery
    int spawn_policy;         // Indicate dynamic process spawning
    int spare_ranks;          // Spare ranks entered by user to repair failed ranks
    int repair_result;        // Internal global variable to store the result of MPI communicator repair
    int finalized;
    jmp_buf *recover_environment; // Calling environment to fill the jmp_buf structure


    //enum FenixRankRole role;    // Role of rank: initial, survivor or repair
    int role;    // Role of rank: initial, survivor or repair
    int fenix_init_flag;

    int fail_world_size;
    int* fail_world;

    //Save the pointer to role and error of Fenix_Init
    int *ret_role;
    int *ret_error;

    fenix_callback_list_t* callback_list;  // singly linked list for user-defined Fenix callback functions
    //fenix_communicator_list_t* communicator_list;  // singly linked list for Fenix resilient communicators
    fenix_debug_opt_t options;    // This is reserved to store the user options

    MPI_Comm world;                 // Duplicate of the MPI communicator provided by user
    MPI_Comm new_world;            // Global MPI communicator identical to g_world but without spare ranks
    MPI_Comm *user_world;           // MPI communicator with repaired ranks
    MPI_Op   agree_op;              // This is reserved for the global agreement call for Fenix data recovery API
    
    
    MPI_Errhandler mpi_errhandler;  // This stores callback info for our custom error handler
    int ignore_errs;                // Set this to return errors instead of using the error handler normally. (Don't forget to unset!)
    int print_unhandled;            // Set this to print the error string for MPI errors of an unhandled return type.



    fenix_data_recovery_t *data_recovery;   // Global pointer for Fenix Data Recovery Data Structure
} fenix_t;

extern fenix_t fenix;
#endif // __FENIX_EXT_H__

