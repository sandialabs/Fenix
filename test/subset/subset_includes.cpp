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
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <fenix_data_subset.hpp>
#include <fenix_data_buffer.hpp>

using namespace Fenix;

bool test_unstrided(int start, int end){
   printf("Testing subset [%d, %d] \n", start, end);
   std::cout.flush();

   DataSubset s({start, end});
   for(int i = 0; i < start; i++){
      if(s.includes(i)){
         printf("Subset [%d, %d] incorrectly includes %d\n", start, end, i);
         return false;
      }
   }
   
   int test_end = end == -1 ? start+20 : end;
   for(int i = start; i <= test_end; i++){
      if(!s.includes(i)){
         printf("Subset [%d, %d] incorrectly excludes %d\n", start, end, i);
         return false;
      }
   }

   if(end != -1){
      for(int i = end+1; i <= test_end+20; i++){
         if(s.includes(i)){
            printf("Subset [%d, %d] incorrectly includes %d\n", start, end, i);
            return false;
         }
      }
   }

   return true;
}

bool test_strided(int start, int end, int count, int stride){
   printf("Testing subset [%d, %d]x%d stride %d \n", start, end, count, stride);
   std::cout.flush();

   DataSubset s({start, end}, count, stride);

   for(int i = 0; i < start; i++){
      if(s.includes(i)){
         printf("Subset [%d, %d]x%d stride %d incorrectly includes %d\n", start, end, count, stride, i);
         return false;
      }
   }
   for(int b = 0; b < count && b < 5; b++){
      int b_start = start+b*stride;
      int b_end = end+b*stride;
      for(int i = b_start; i <= b_end; i++){
         if(!s.includes(i)){
            printf("Subset [%d, %d]x%d stride %d incorrectly excludes %d\n", start, end, count, stride, i);
            return false;
         }
      }
   }
   for(int b = 0; b < count-1 && b < 5; b++) {
      int b_end = end+b*stride;
      int next_b_start = start + (b+1)*stride;
      for(int i = b_end+1; i < next_b_start; i++){
         if(s.includes(i)){
            printf("Subset [%d, %d]x%d stride %d incorrectly includes %d\n", start, end, count, stride, i);
            return false;
         }
      }
   }

   int range_end = end+(count-1)*stride;
   for(int i = range_end+1; i < range_end+1+stride*2; i++){
      if(s.includes(i)){
         printf("Subset [%d, %d]x%d stride %d incorrectly includes %d\n", start, end, count, stride, i);
         return false;
      }
   }

   return true;
}

int main(int argc, char **argv)
{
   bool success = true;

   success &= test_unstrided(0, 10);
   success &= test_unstrided(5, 10);
   success &= test_unstrided(10, 10);
   success &= test_unstrided(0, -1);
   success &= test_unstrided(10, -1);
   
   success &= test_strided(0, 4, 2, 5);
   success &= test_strided(0, 4, 2, 6);
   success &= test_strided(0, 4, 10, 6);
   success &= test_strided(0, 4, 10, 10);
   
   return success ? 0 : 1;
}


