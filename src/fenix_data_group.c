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
#include "fenix_data_group.h"
#include "fenix_data_member.h"
#include "fenix_data_version.h"
#include "fenix_data_buffer.h"
#include "fenix_data_packet.h"
#include "fenix_constants.h"


/**
 * @brief
 */
fenix_group_t * __fenix_data_group_init() {
  fenix_group_t *group = (fenix_group_t *)
          s_calloc(1, sizeof(fenix_group_t));

  group->count = 0;
  group->total_size = __FENIX_DEFAULT_GROUP_SIZE;
  group->group_entry = (fenix_group_entry_t *) s_malloc(
          __FENIX_DEFAULT_GROUP_SIZE * sizeof(fenix_group_entry_t));

  if (__fenix_options.verbose == 41) {
    verbose_print("c-rank: %d, role: %d, g-count: %d, g-size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, group->count,
                  group->total_size);
  }

  int group_index;
  for (group_index = 0;
       group_index < __FENIX_DEFAULT_GROUP_SIZE; group_index++) { // insert default values

    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->depth = 1;
    gentry->groupid = -1;
    gentry->timestamp = 0;
    gentry->state = EMPTY;

    if (__fenix_options.verbose == 41) {
      verbose_print(
              "c-rank: %d, role: %d, g-depth: %d, g-groupid: %d, g-timestamp: %d g-state: %d\n",
                __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, gentry->depth,
              gentry->timestamp, gentry->state);
    }

    gentry->member = __fenix_data_member_init();
  }
  return group;
}


void __fenix_data_group_destroy( fenix_group_t *fx_group )  {
  //free(fx_group);

}



/**
 * @brief
 * @param
 * @param
 */
void __fenix_data_group_reinit(fenix_group_t *g, fenix_two_container_packet_t packet) {
  fenix_group_t *group = g;
  int start_index = group->total_size;
  group->count = packet.count;
  group->total_size = packet.total_size;
  group->group_entry = (fenix_group_entry_t *) s_realloc(group->group_entry,
                                                         (group->total_size) *
                                                         sizeof(fenix_group_entry_t));

  if (__fenix_options.verbose == 48) {
    verbose_print("c-rank: %d, role: %d, g-size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group->total_size);
  }

  int group_index;
  for (group_index = start_index; group_index < group->total_size; group_index++) {
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    gentry->depth = 1;
    gentry->groupid = -1;
    gentry->timestamp = 0;
    gentry->state = EMPTY;

    if (__fenix_options.verbose == 48) {
      verbose_print(
              "c-rank: %d, role: %d, g-depth: %d, g-groupid: %d, g-timestamp: %d, g-state: %d\n",
                __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, gentry->depth,
              gentry->groupid, gentry->timestamp, gentry->state);
    }

    gentry->member = __fenix_data_member_init();
  }
}
