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

#ifndef __FENIX_EXT_H__
#define __FENIX_EXT_H__
#include <mpi.h>
#include "fenix_opt.h"
#include "fenix_data_group.h"

extern __fenix_debug_options __fenix_options;
extern int __fenix_g_fenix_init_flag;
extern int __fenix_g_role;
extern fenix_group_t *__fenix_g_data_recovery;

extern int __fenix_g_num_inital_ranks;
extern int __fenix_g_num_survivor_ranks;
extern int __fenix_g_num_recovered_ranks;
extern int __fenix_g_resume_mode;  // Defines how program resumes after process recovery
extern int __fenix_g_spawn_policy;               // Indicate dynamic process spawning
extern int __fenix_g_spare_ranks;                // Spare ranks entered by user to repair failed ranks
extern int __fenix_g_replace_comm_flag;
extern int __fenix_g_repair_result;


extern MPI_Comm *__fenix_g_world;                // Duplicate of the MPI communicator provided by user
extern MPI_Comm *__fenix_g_new_world;            // Global MPI communicator identical to g_world but without spare ranks
extern MPI_Comm *__fenix_g_user_world;           // MPI communicator with repaired ranks
extern MPI_Comm __fenix_g_original_comm;
extern MPI_Op __fenix_g_agree_op;


#endif // __FENIX_EXT_H__

