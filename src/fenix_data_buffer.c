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
#include "fenix_data_buffer.h"
#include "fenix_ext.h"

/**
 * @brief
 */
fenix_local_entry_t *__fenix_init_local() {
  fenix_local_entry_t *local = (fenix_local_entry_t *) s_malloc(
          sizeof(fenix_local_entry_t));;
  local->currentrank = -1;
  local->pdata = NULL;
  local->data = NULL;
  local->count = 0;
  local->datatype_size = 0;
  local->datatype = NULL;

  if (__fenix_options.verbose == 44) {
    verbose_print(
            "c-rank: %d, role: %d, ld-currentrank: %d, ld-count: %d, ld-size: %d\n",
              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, local->currentrank,
            local->count, local->datatype_size);
  }

  return local;
}

/**
 * @brief
 */
fenix_remote_entry_t *__fenix_init_remote() {
  fenix_remote_entry_t *remote = (fenix_remote_entry_t *) s_malloc(    sizeof(fenix_remote_entry_t));
  remote->remoterank = -1;
  remote->pdata = NULL;
  remote->data = NULL;
  remote->count = 0;
  remote->datatype_size = 0;
  remote->datatype = NULL;

  if (__fenix_options.verbose == 45) {
    verbose_print(
            "c-rank: %d, role: %d, rd-remoterank: %d, rd-count: %d, rd-size: %d\n",
              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, remote->remoterank,
            remote->count, remote->datatype_size);
  }

  return remote;
}

/**
 * @brief
 */
fenix_buffer_entry_t *__fenix_data_buffer_create() {
  fenix_buffer_entry_t *buffer = (fenix_buffer_entry_t *) s_malloc( sizeof( fenix_buffer_entry_t ) );
  buffer->origin_rank = -1;
  buffer->data = NULL;
  buffer->count = 0;
  buffer->datatype_size = 0;
  buffer->datatype = NULL;

  if ( __fenix_options.verbose == 45 ) {
    verbose_print(
            "c-rank: %d, role: %d, rd-remoterank: %d, rd-count: %d, rd-size: %d\n",
              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, buffer->origin_rank,
           buffer->count, buffer->datatype_size);
  }

  return buffer;
}


int __fenix_data_buffer_reset( fenix_buffer_entry_t *buffer ) {
  buffer->origin_rank = -1;
  buffer->data = NULL;
  buffer->count = 0;
  buffer->datatype_size = 0;
  buffer->datatype = NULL;

  if (__fenix_options.verbose == 45) {
    verbose_print(
            "c-rank: %d, role: %d, rd-remoterank: %d, rd-count: %d, rd-size: %d\n",
              __fenix_get_current_rank(*__fenix_g_world), __fenix_g_role, buffer->origin_rank,
           buffer->count, buffer->datatype_size);
  }


  return FENIX_SUCCESS;
}

void __fenix_data_buffer_destroy(  fenix_buffer_entry_t *buffer  ) {

  if( buffer->data != NULL ) {
    free( buffer->data );
  }
  free( buffer );

}
