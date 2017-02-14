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

#include "mpi.h"
#include "fenix-config.h"
#include "fenix_ext.h"
#include "fenix_data_member.h"
#include "fenix_data_version.h"
#include "fenix_data_buffer.h"
#include "fenix_data_packet.h"
#include "fenix_constants.h"

/**
 * @brief
 */
fenix_member_t *__fenix_data_member_init() {
  fenix_member_t *member = (fenix_member_t *)
          s_calloc(1, sizeof(fenix_member_t));
  member->count = 0;
  member->total_size = __FENIX_DEFAULT_MEMBER_SIZE;
  member->member_entry = (fenix_member_entry_t *) s_malloc(
          __FENIX_DEFAULT_MEMBER_SIZE * sizeof(fenix_member_entry_t));

  if (__fenix_options.verbose == 42) {
    verbose_print("c-rank: %d, role: %d, m-count: %d, m-size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, member->count,
                  member->total_size);
  }

  int member_index;
  for (member_index = 0; member_index <
                         __FENIX_DEFAULT_MEMBER_SIZE; member_index++) { // insert default values
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->memberid = -1;
    mentry->state = EMPTY;

    if (__fenix_options.verbose == 42) {
      verbose_print("c-rank: %d, role: %d, m-memberid: %d, m-state: %d\n",
                      __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role,
                    mentry->memberid, mentry->state);
    }

    mentry->version = __fenix_data_version_init();
  }
  return member;
}

void __fenix_data_member_destroy( fenix_member_t *member ) {

  int member_index;
  for ( member_index = 0; member_index < member->total_size; member_index++ ) {
      __fenix_data_version_destroy( member->member_entry[member_index].version );
  }
  free( member->member_entry );
  free( member );
}

/**
 * @brief
 * @param
 */
void __fenix_ensure_member_capacity(fenix_member_t *m) {
  fenix_member_t *member = m;
  if (member->count >= member->total_size) {
    int start_index = member->total_size;
    member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                              (member->total_size * 2) *
                                                              sizeof(fenix_member_entry_t));
    member->total_size = member->total_size * 2;

    if (__fenix_options.verbose == 52) {
      verbose_print("c-rank: %d, role: %d, m-count: %d, m-size: %d\n",
                    member->count, member->total_size);
    }

    int member_index;
    for (member_index = start_index; member_index < member->total_size; member_index++) {
      fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
      mentry->memberid = -1;
      mentry->state = EMPTY;

      if (__fenix_options.verbose == 52) {
        verbose_print(
                "c-rank: %d, role: %d, member[%d] m-memberid: %d, m-state: %d\n",
                  __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                member_index, mentry->memberid, mentry->state);
      }

      mentry->version = __fenix_data_version_init();
    }
  }
}

/**
 * @brief
 * @param
 */
void __fenix_ensure_version_capacity(fenix_member_t *m) {
  fenix_member_t *member = m;
  int member_index;
  for (member_index = 0; member_index < member->total_size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    fenix_version_t *version = mentry->version;
    if (version->total_size > __FENIX_DEFAULT_VERSION_SIZE) {
      version->local_entry = (fenix_local_entry_t *) realloc(version->local_entry,
                                                             (version->total_size * 2) *
                                                             sizeof(fenix_local_entry_t));
      version->remote_entry = (fenix_remote_entry_t *) realloc(
              version->remote_entry,
              (version->total_size * 2) *
              sizeof(fenix_remote_entry_t));
      version->total_size = version->total_size * 2;

      if (__fenix_options.verbose == 53) {
        verbose_print(
                "c-rank: %d, role: %d, member[%d] v-count: %d, v-size: %d\n",
                  __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                member_index, version->count, version->total_size);
      }

    }
  }
}

/**
 * @brief
 * @param
 * @param
 */
void __fenix_data_member_reinit(fenix_member_t *m, fenix_two_container_packet_t packet,
                   enum states mystatus) {
  fenix_member_t *member = m;
  int start_index = member->total_size;
  member->count = 0;
  member->temp_count = packet.count;
  member->total_size = packet.total_size;
  member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                            (member->total_size) *
                                                            sizeof(fenix_member_entry_t));
  if (__fenix_options.verbose == 50) {
    verbose_print("c-rank: %d, role: %d, m-count: %d, m-size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                  member->count, member->total_size);
  }

  int member_index;
  /* Why start_index is set to the number of member entries ? */
  // for (member_index = start_index; member_index < member->size; member_index++) {
  for (member_index = 0; member_index < member->total_size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->memberid = -1;
    mentry->state = mystatus;
    if (__fenix_options.verbose == 50) {
      verbose_print("c-rank: %d, role: %d, m-memberid: %d, m-state: %d\n",
                      __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                    mentry->memberid, mentry->state);
    }

    mentry->version = __fenix_data_version_init();
  }
}
