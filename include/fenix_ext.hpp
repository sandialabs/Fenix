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
#include <vector>
#include <unordered_map>
#include <map>
#include "fenix.h"
#include "fenix.hpp"
#include "fenix_opt.hpp"
#include "fenix_data_group.hpp"
#include "fenix/logging/comm_log.h"

namespace fenix {

// Fenix's global settings, configurable before initialization
struct Settings {
  RecoveryMode recovery              = REPAIR;
  // Defaults to JUMP if a jump_buf provided, else THROW
  ResumeMode resume                  = FENIX_RESUME_MODE_MAXCODE;
  CallbackExceptionMode cb_exception = SQUASH;
  UnhandledMode unhandled            = ABORT;
  MlogRecoveryMode mlog_recovery     = MANUAL;
  SpareWaitMode spare_wait           = YIELD;
};

// Configurations before init change this
inline Settings fenix_default_settings;

struct fenix_t {
  // Global Fenix settings
  Settings settings;

  int num_initial_ranks;
  int num_survivor_ranks  = 0; // As of last failure
  int num_recovered_ranks = 0; // As of last failure
  int spare_ranks;             // Spare ranks entered by user

  jmp_buf* recover_environment; // for FENIX_RESUME_JUMP

  int mpi_fail_code = MPI_SUCCESS;
  int repair_result = FENIX_SUCCESS; // Result of MPI comm repair
  int role          = FENIX_ROLE_INITIAL_RANK;

  int fenix_init_flag = false;
  int finalized       = false;

  int fail_world_size = 0;
  int* fail_world     = nullptr;

  //Save the pointer to role and error of Fenix_Init
  int* ret_role  = nullptr;
  int* ret_error = nullptr;

  std::unordered_map<CallbackLocation, std::vector<FenixCallbackFunc>>
    callbacks;
  fenix_debug_opt_t options; // This is reserved to store the user options

  MPI_Comm* world;      // Duplicate of comm provided by user
  MPI_Comm* user_world; // User-facing comm with repaired ranks and no spares
  MPI_Comm new_world;   // Internal duplicate of user_world
  int new_world_exists = false, user_world_exists = false;

  //Values used for Fenix_Process_detect_failures
  int dummy_recv_buffer;
  MPI_Request check_failures_req;

  MPI_Op agree_op;               // Global agreement call for data recovery API
  MPI_Errhandler mpi_errhandler; // Our custom error handler

  fenix::data::fenix_data_recovery_t* data_recovery = nullptr;

  // -------------------------
  // Message logging variables
  // -------------------------

  // All loggers indexed by ID
  std::unordered_map<int, std::shared_ptr<logging::CommLog>> mlogs;
  // Order of creation of all existing mlogs
  std::vector<int> mlog_order;
  // Active log (if any)
  std::shared_ptr<logging::CommLog> active_mlog;
  int active_mlog_id = FENIX_MLOG_NONE;

  // Maps of UID to MPI internal types for logging
  std::map<int, MPI_Datatype> mpi_types;
  std::map<int, MPI_Op> mpi_ops;

  static inline bool mpi_overloads_linked = false;
};

inline fenix::fenix_t fenix_rt;
} // namespace fenix
#endif // __FENIX_EXT_H__
