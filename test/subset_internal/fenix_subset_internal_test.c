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
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY and SANDIA CORPORATION
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RUTGERS 
// UNIVERISY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <fenix.h>
#include <fenix_data_recovery.h> // Never called explicitly by the  users
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

int _verify_subset( double *data, int num_blocks, int start_offset, int end_offset, int stride,
                    Fenix_Data_subset *subset_specifier );

int _verify_subset( double *data, int num_repeats, int start_offset, int end_offset, int stride,
                    Fenix_Data_subset *sp)
{
   int i, j;
   int idx;
   int block_size;
   int flag = 0;
   double accumulator =0.0;

   if( num_repeats != sp->num_repeats[0]+1 ) {
      flag = 1;
      printf("num_repeats set incorrectly.");
   } 
   if( start_offset != sp->start_offsets[0] ) {
      flag = 2;
      printf("start_offset set incorrectly\n");
   }
   if( end_offset != sp->end_offsets[0]){
      flag = 3;
      printf("end_offset set incorrectly\n");
   }
   if( sp->specifier != __FENIX_SUBSET_CREATE ) {
      flag = 4;
      printf("specifier set incorrectly\n");
   }
   if(stride != sp->stride){
      flag = 5;
      printf("stride set incorrectly\n");
   }

   /* Itertate over the loop to see if any memory error occurs*/
   idx = start_offset;
   block_size = end_offset - start_offset;
   for ( i = 0; i < num_repeats; i++ ) {
      for( j = 0; j < block_size; j++ ) {
         accumulator += data[idx+j];
      }
      idx += stride;
   }

  return flag;
}


int main(int argc, char **argv)
{
   Fenix_Data_subset subset_specifier;
   int num_blocks;
   int start_offset, end_offset, stride;
   int space_size;
   double *d_space;

  if (argc < 6) {
      printf("Usage: %s <array size> <# blocks> <start offset> <end offset> <stride> \n", *argv);
      exit(0);
  }

   space_size   = atoi(argv[1]);
   num_blocks   = atoi(argv[2]);
   start_offset = atoi(argv[3]);
   end_offset   = atoi(argv[4]);
   stride       = atoi(argv[5]); 

   if( space_size < (num_blocks * stride + start_offset) ) {
      printf("Error: Array size is smaller than (the number of blocks x stride) + start_offset\n");
      printf("Aborting\n");
      exit(0);
   }

   if( start_offset > end_offset ) {
      printf("Error: Start offset must be less than end_offset\n");
      printf("Aborting\n");
      exit(0);
   }
   
   d_space = (double *)malloc(sizeof(double)*space_size);
   Fenix_Data_subset_create(num_blocks, start_offset, end_offset, stride, &subset_specifier);
   //data_subset_create( num_blocks, start_offset, end_offset, stride, &subset_specifier );
   // Verification
   int err_code = _verify_subset( d_space, num_blocks, start_offset, end_offset, stride, &subset_specifier );
   // free_data_subset_fixed ( &subset_specifier);
   free(d_space);
   if( err_code == 0 ) {
      printf("Passed\n");
   }
   return err_code;
}


