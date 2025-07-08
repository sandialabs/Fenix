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

#include <fenix_data_subset.hpp>

#include "subset_common.hpp"

using namespace Fenix;

bool test_addition(const DataSubset& a, const DataSubset& b){
   printf("Testing subsets a=%s, b=%s\n", a.str().c_str(), b.str().c_str());

   const DataSubset c = a + b;
   const DataSubset d = b + a;

   printf("c=a+b=%s\n", c.str().c_str());
   printf("d=b+a=%s\n", d.str().c_str());

   if(c != d){
      printf("a+b != b+a\n");
      return false;
   }

   size_t start = std::min(a.start(), b.start());
   size_t end;
   if(a.end() == -1 || b.end() == -1){
      end = start+1000;
   } else {
      end = std::max(a.end(), b.end()) + 10;
   }
   
   for(int i = start; i <= end; i++){
      if(c.includes(i) != (a.includes(i) || b.includes(i))){
         if(c.includes(i)){
            printf("c=a+b incorrectly includes index %d not in a or b\n", i);
            return false;
         } else {
            printf(
               "c=a+b incorrectly excludes index %d in %s\n", i,
               a.includes(i) ? b.includes(i) ? "both" : "a" : "b"
            );
            return false;
         }
      }
      if(d.includes(i) != (a.includes(i) || b.includes(i))){
         if(d.includes(i)){
            printf("d=b+a incorrectly includes index %d not in a or b\n", i);
            return false;
         } else {
            printf(
               "d=b+a incorrectly excludes index %d in %s\n", i,
               a.includes(i) ? b.includes(i) ? "both" : "a" : "b"
            );
            return false;
         }
      }
   }

   return true;
}

int main(int argc, char **argv)
{
   bool success = true;

   auto subsets = get_subsets();
   for(const auto& a : subsets){
      for(const auto& b : subsets){
         success &= test_addition(a, b);
      }
   }

   return success ? 0 : 1;
}


