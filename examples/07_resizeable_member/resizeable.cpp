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
// LIMITED TO, THE  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

constexpr int kKillID = 2;
constexpr int my_group = 0;
constexpr int my_member = 0;      
constexpr int start_timestamp = 0;
constexpr int group_depth = 1;
int errflag;

using Fenix::DataSubset;
using namespace Fenix::Data;

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  
  MPI_Comm res_comm;
  Fenix::init({.out_comm = &res_comm, .spares = 1}); 

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);
  
  std::vector<int> data;

  bool should_throw = Fenix_get_role() == FENIX_ROLE_RECOVERED_RANK;
  while(true) try {
    if(should_throw){
      should_throw = false;
      Fenix::throw_exception();
    }
    
    //Initial work and commits
    if(Fenix_get_role() == FENIX_ROLE_INITIAL_RANK){
      Fenix_Data_group_create(
        my_group, res_comm, start_timestamp, group_depth, FENIX_DATA_POLICY_IMR,
        NULL, &errflag
      );
      Fenix_Data_member_create(
        my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
      );

      data.resize(100);
      for(int& i : data) i = -1;
    
      
      //Store the whole array first. We need to keep our buffer pointer updated
      //since resizing an array can change it
      Fenix_Data_member_attr_set(
        my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
        &errflag
      );
      member_store(my_group, my_member, {{0, data.size()-1}});
      Fenix_Data_commit_barrier(my_group, NULL);

      
      //Now commit a smaller portion with different data.
      data.resize(50);
      int val = 1;
      for(int& i : data) i = val++;

      Fenix_Data_member_attr_set(
        my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
        &errflag
      );
      member_store(my_group, my_member, {{0, data.size()-1}});
      Fenix_Data_commit_barrier(my_group, NULL);
   
      
      if(rank == kKillID){
        fprintf(stderr, "Doing kill on node %d\n", rank);
        raise(SIGTERM);
      }
    }
    
    Fenix_Finalize();

    
    break;
  } catch (const Fenix::CommException& e) {
    const Fenix::CommException* err = &e;
    while(true) try {
      //We've had a failure! Time to recover data.
      fprintf(stderr, "Starting data recovery on rank %d\n", rank);
      if(err->fenix_err != FENIX_SUCCESS){
        fprintf(stderr, "FAILURE on Fenix Init (%d). Exiting.\n", err->fenix_err);
        exit(1);
      }
      
      Fenix_Data_group_create(
        my_group, res_comm, start_timestamp, group_depth, FENIX_DATA_POLICY_IMR,
        NULL, &errflag
      );
      
      //Do a null restore to get information about the stored subset
      DataSubset stored_subset;
      int ret = member_restore(
        my_group, my_member, nullptr, 0, FENIX_TIME_STAMP_MAX, stored_subset
      );
      if(ret != FENIX_SUCCESS) {
        fprintf(stderr, "Rank %d restore failure w/ code %d\n", rank, ret);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }

      //Resize data to fit all stored data
      data.resize(stored_subset.end()+1);

      //Set all data to a value that was never stored, just for testing
      for(int& i : data) i = -2;
      
      //Now do an lrestore to get the recovered data.
      ret = member_lrestore(
        my_group, my_member, data.data(), data.size(), FENIX_TIME_STAMP_MAX,
        stored_subset
      );
      
      break;
    } catch (const Fenix::CommException& nested){
      err = &nested;
    }
  }

  //Ensure data is correct after execution and recovery
  bool successful = data.size() == 50;
  if(!successful) printf("Rank %d expected data size 50, but got %d\n", rank, data.size());

  for(int i = 0; i < data.size() && successful; i++){
    successful &= data[i] == i+1;
    if(!successful) printf("Rank %d data[%d]=%d, but should be %d!\n", rank, i, data[i], i+1);
  }

  if(successful){
    printf("Rank %d successfully recovered\n", rank);
  } else {
    printf("FAILURE on rank %d\n", rank);
  }

  MPI_Finalize();
  return !successful; //return error status
}
