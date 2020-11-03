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
//        Rob Van der Wijngaart, and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <assert.h>

#include "fenix_comm_list.h"
#include "fenix_ext.h"
#include "fenix_process_recovery.h"
#include "fenix_data_group.h"
#include "fenix_data_recovery.h"
#include "fenix_opt.h"
#include "fenix_util.h"
#include <mpi.h>


int __fenix_callback_register(void (*recover)(MPI_Comm, int, void *), void *callback_data)
{
    int error_code = FENIX_SUCCESS;
    if (fenix.fenix_init_flag) {
        fenix_callback_func *fp = s_malloc(sizeof(fenix_callback_func));
        fp->x = recover;
        fp->y = callback_data;
        __fenix_callback_push( &fenix.callback_list, fp);
    } else {
        error_code = FENIX_ERROR_UNINITIALIZED;
    }
    return error_code;
}

void __fenix_callback_invoke_all(int error)
{
    fenix_callback_list_t *current = fenix.callback_list;
    while (current != NULL) {
        (current->callback->x)((MPI_Comm) fenix.new_world, error,
                               (void *) current->callback->y);
        current = current->next;
    }
}

void __fenix_callback_push(fenix_callback_list_t **head, fenix_callback_func *fp)
{
    fenix_callback_list_t *callback = malloc(sizeof(fenix_callback_list_t));
    callback->callback = fp;
    callback->next = *head;
    *head = callback;
}

int __fenix_callback_destroy(fenix_callback_list_t *callback_list)
{
    int error_code = FENIX_SUCCESS;

    if ( fenix.fenix_init_flag ) {

        fenix_callback_list_t *current = callback_list;

        while (current != NULL) {
            fenix_callback_list_t *old;
            old = current;
            current = current->next;
            free( old->callback );
            free( old );
        }

    } else {
        error_code = FENIX_ERROR_UNINITIALIZED;
    }

    return error_code;
}

