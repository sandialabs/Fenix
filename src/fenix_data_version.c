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

#include "fenix-config.h"
#include "fenix_data_version.h"
#include "fenix_ext.h"
#include <stdlib.h>

#if 0
int __fenix_create_version ( fenix_version_t **v ) {

  *v = (fenix_version_t *) malloc(sizeof( fenix_version_t ) );
  /* Allcoate local entry */
  *v
  /* Allocate remote entry */
  *v->remote_entry;

  return 0;
}

int __fenix_free_version ( fenix_version_t *v )  {

  __fenix_free_remote_entry( v->remote_entry,  v->num_copies );

  __fenix_free_local_entry( v->local_entry,  v->num_copies );

  free( fenix_version_t );
  return 0;
}

int __fenix_reset_version( fenix_version_t *v ) {
  v->num_copies = 0;
  v->count = 0;
  v->num_versions = 0;
  v->total_size = __FENIX_DATA_VERSION_DEFAULT;
  v->current_position = 0;

  __fenix_reset_remote_entry( v->remote_entry, v->num_copies );
  __fenix_reset_local_entry( v->local_entry, v->num_copies );

  return 0;
}

#endif
/**
 * @brief
 * @param
 * @param
 */
void __fenix_reinit_version(fenix_version_t *v, fenix_container_packet_t packet) {

  int first_index = v->total_size;
  v->num_copies = packet.num_copies;
  v->count = packet.count;
  v->total_size = packet.total_size;
  v->position = packet.position;
  v->local_entry = (fenix_local_entry_t *) realloc(v->local_entry,
                                                   (v->total_size) *
                                                   sizeof(fenix_local_entry_t));
  v->remote_entry = (fenix_remote_entry_t *) realloc(v->remote_entry,
                                                     (v->total_size) *
                                                     sizeof(fenix_remote_entry_t));

  if (__fenix_options.verbose == 49) {
    verbose_print("c-rank: %d, role: %d, v-count: %d, v-size: %d, v-position: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, v->count,
                  v->total_size, v->position);
  }

/*
 * Allocate space for data entry
 */
  int version_index;
  for (version_index = first_index; version_index < v->total_size; version_index++) {
    v->local_entry[version_index] = *__fenix_init_local();
    v->remote_entry[version_index] = *__fenix_init_remote();
  }
}
