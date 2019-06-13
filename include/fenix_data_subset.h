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
#ifndef __FENIX_DATA_SUBSET_H__
#define __FENIX_DATA_SUBSET_H__
#include <mpi.h>

#define __FENIX_SUBSET_EMPTY   1
#define __FENIX_SUBSET_FULL    2
#define __FENIX_SUBSET_CREATE  3
#define __FENIX_SUBSET_CREATEV 4
#define __FENIX_SUBSET_UNDEFINED -1


//Specifier speeds up the process by letting us know if this is a simple
//subset in which each region is repeated w/ same stride, or if each
//region is never repeated (EG create vs createv). Also has specifiers for
//FULL/EMPTY.
typedef struct {
    int num_blocks;
    int* start_offsets;
    int* end_offsets;
    int* num_repeats;
    int stride;
    int specifier;
} Fenix_Data_subset;

int __fenix_data_subset_init(int num_blocks, Fenix_Data_subset* subset);
int __fenix_data_subset_create(int, int, int, int, Fenix_Data_subset *);
int __fenix_data_subset_createv(int, int *, int *, Fenix_Data_subset *);
void __fenix_data_subset_deep_copy(Fenix_Data_subset* from, Fenix_Data_subset* to);
void __fenix_data_subset_merge(Fenix_Data_subset* first_subset, 
      Fenix_Data_subset* second_subset, Fenix_Data_subset* output);
void __fenix_data_subset_merge_inplace(Fenix_Data_subset* first_subset, 
      Fenix_Data_subset* second_subset);
void __fenix_data_subset_copy_data(Fenix_Data_subset* ss, void* dest,
      void* src, size_t data_type_size, size_t max_size);
int __fenix_data_subset_data_size(Fenix_Data_subset* ss, size_t max_size);
void* __fenix_data_subset_serialize(Fenix_Data_subset* ss, void* src, 
      size_t type_size, size_t max_size, size_t* output_size);
void __fenix_data_subset_deserialize(Fenix_Data_subset* ss, void* src, 
      void* dest, size_t max_size, size_t type_size);
void __fenix_data_subset_send(Fenix_Data_subset* ss, int dest, int tag, MPI_Comm comm);
void __fenix_data_subset_recv(Fenix_Data_subset* ss, int src, int tag, MPI_Comm comm);
int __fenix_data_subset_is_full(Fenix_Data_subset* ss, size_t data_length);
int __fenix_data_subset_free(Fenix_Data_subset *);
int __fenix_data_subset_delete(Fenix_Data_subset *);

#endif // FENIX_DATA_SUBSET_H
