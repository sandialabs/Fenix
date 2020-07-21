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

#include <stdlib.h>
#include <stdio.h>
#include <fenix.h>
#include <fenix_process_recovery.h>
#include <mpi-ext.h>

fenix_comm_list_t my_list = {NULL, NULL};

int __fenix_comm_push(MPI_Comm *comm) {
  fenix_comm_list_elm_t *current = (fenix_comm_list_elm_t *) malloc(sizeof(fenix_comm_list_elm_t));
  if (!current) return 0;
  current->next = NULL;
  current->comm = comm;
  if (!my_list.tail) {
    /* if list was empty, initialize head and tail                             */
    current->prev = NULL;
    my_list.head = my_list.tail = current;
  }
  else {
    /* if list was not empty, add element to the head of the list              */
    current->prev = my_list.head;
    my_list.head->next = current;
    my_list.head = current;
  }
  return FENIX_SUCCESS;
}

int __fenix_comm_delete(MPI_Comm *comm) {

  fenix_comm_list_elm_t *current = my_list.tail;
  while (current) {
    if (*(current->comm) == *comm) {
      if (current != my_list.head && current != my_list.tail) {
	current->prev->next = current->next;
        current->next->prev = current->prev;
      }
      else if (current == my_list.tail) {
        if (current->next) {
          current->next->prev = NULL;
          my_list.tail = current->next;
	}
        else my_list.tail = my_list.head = NULL;
      }
      else {
        if (current->prev) {
          current->prev->next = NULL;
          my_list.head = current->prev;
	}
        else my_list.tail = my_list.head = NULL;
      }
      MPIX_Comm_revoke(*comm);
      PMPI_Comm_free(comm);
      free(current);
      return 1;
    }
    else current = current->next;
  }
  /* if we end up here, the requested communicator has not been found */
  return 0;
}
  

void __fenix_comm_list_destroy(void) {
  if (my_list.tail == NULL) {
    return;
  }
  else {
    fenix_comm_list_elm_t *current = my_list.tail;
    while (current->next) {
      fenix_comm_list_elm_t *new = current->next;
      MPIX_Comm_revoke(*current->comm);
      PMPI_Comm_free(current->comm);
      free(current);
      current = new;
    }
    MPIX_Comm_revoke(*current->comm);
    PMPI_Comm_free(current->comm);
    free(current);
  }
  my_list.tail = my_list.head = NULL;
}

