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
//        Rob Van der Wijngaart, Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/


#ifndef __FENIX_HPP__
#define __FENIX_HPP__

#include <mpi.h>
#include <functional>
#include <vector>
#include <optional>
#include "fenix.h"
#include "fenix_exception.hpp"
#include "fenix_data_subset.hpp"

/**
 * @brief As the C-style callback, but accepts an std::function and does not use the void* pointer.
 *
 * @param[in] callback The function to register.
 *
 * @returnstatus
 */
int Fenix_Callback_register(std::function<void(MPI_Comm, int)> callback);

namespace Fenix {

using Role = Fenix_Rank_role;
constexpr Role INITIAL_RANK   = FENIX_ROLE_INITIAL_RANK;
constexpr Role RECOVERED_RANK = FENIX_ROLE_RECOVERED_RANK;
constexpr Role SURVIVOR_RANK  = FENIX_ROLE_SURVIVOR_RANK;

using ResumeMode = Fenix_Resume_mode;
constexpr ResumeMode JUMP   = FENIX_RESUME_JUMP;
constexpr ResumeMode RETURN = FENIX_RESUME_RETURN;
constexpr ResumeMode THROW  = FENIX_RESUME_THROW;

using UnhandledMode = Fenix_Unhandled_mode;
constexpr UnhandledMode SILENT = FENIX_UNHANDLED_SILENT;
constexpr UnhandledMode PRINT  = FENIX_UNHANDLED_PRINT;
constexpr UnhandledMode ABORT  = FENIX_UNHANDLED_ABORT;

namespace Args {
struct FenixInitArgs {
    int* role                    = nullptr;
    MPI_Comm in_comm             = MPI_COMM_WORLD;
    MPI_Comm* out_comm           = nullptr;
    int* argc                    = nullptr;
    char*** argv                 = nullptr;
    int spares                   = 0;
    int spawn                    = 0;
    ResumeMode resume_mode       = THROW;
    UnhandledMode unhandled_mode = ABORT;
    int* err                     = nullptr;
};
}

void init(const Args::FenixInitArgs args);

//!@brief Throw an exception for the most recent fault. Helpful for spares.
void throw_exception();

//!@brief Overload of #Fenix_get_role
Fenix_Rank_role role();

//!@brief Overload of #Fenix_get_error
int error();

//!@brief Overload of #Fenix_get_nspare
int nspare();

//!@brief Overload of #Fenix_Callback_register
int callback_register(std::function<void(MPI_Comm, int)> callback);

//@!brief Overload of #Fenix_Callback_pop
int callback_pop();

/**
 * @brief Get the failed ranks from the most recent recovery
 * @return vector of failed ranks
 */
std::vector<int> fail_list();

//!@brief Overload of #Fenix_Process_detect_failures
int detect_failures(bool recover = true);

//!@brief Overload of #Fenix_Initialized that directly returns true if initialized
bool initialized();

} // namespace Fenix

namespace Fenix::Data {

extern const DataSubset SUBSET_FULL;
extern const DataSubset SUBSET_EMPTY;
extern DataSubset SUBSET_IGNORE;

//@!brief Overload of Fenix_Data_group_create
int group_create(
    int group_id, MPI_Comm comm, int start_time_stamp, int depth,
    int policy_name, void* policy_value, int* flag
);

//@!brief Overload of Fenix_Data_member_create
int member_create(
    int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype
);

//!@brief Overload of #Fenix_Data_member_store
int member_store(int group_id, int member_id, const DataSubset& subset);

//!@brief Overload of #Fenix_Data_member_storev
int member_storev(int group_id, int member_id, const DataSubset& subset);

//!@brief Overload of #Fenix_Data_member_istore
int member_istore(
    int group_id, int member_id, const DataSubset& subset,
    Fenix_Request *request
);

//!@brief Overload of #Fenix_Data_member_istorev
int member_istorev(
    int group_id, int member_id, const DataSubset& subset,
    Fenix_Request *request
);

//!@brief Overload of #Fenix_Data_member_restore
int member_restore(
    int group_id, int member_id, void *target_buffer, int max_length,
    int time_stamp, DataSubset& data_found
);

//!@brief Overload of #Fenix_Data_member_lrestore
int member_lrestore(
    int group_id, int member_id, void *target_buffer, int max_length,
    int time_stamp, DataSubset& data_found
);

//@!brief overload of #Fenix_Data_commit
int commit(int group_id, int* time_stamp = nullptr);

//@!brief overload of #Fenix_Data_commit
int commit_barrier(int group_id, int* time_stamp = nullptr);

/**
 * @brief get the members of a group
 * @return vector of member IDs of each member in group_id if group exists
 */
std::optional<std::vector<int>> group_members(int group_id);

/**
 * @brief get the snapshots of a group
 * @return vector of timestamps of each snapshot in group_id if group exists
 */
std::optional<std::vector<int>> group_snapshots(int group_id);

//@!brief Overload of #Fenix_Data_snapshot_delete
int snapshot_delete(int group_id, int timestamp);

//@!brief overload of Fenix_Data_group_delete
int group_delete(int group_id);

//@!brief overload of Fenix_Data_member_delete
int member_delete(int group_id, int member_id);

} // namespace Fenix::Data

#endif
