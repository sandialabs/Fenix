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
//        Rob Van der Wijngaart, Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <assert.h>

#include "fenix_ext.hpp"
#include "fenix_process_recovery.hpp"
#include "fenix_data_group.hpp"
#include "fenix_data_recovery.hpp"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"
#include "fenix_exception.hpp"
#include <mpi.h>

using namespace Fenix;

int __fenix_callback_register(fenix_callback_func& recover)
{
    if(!fenix.fenix_init_flag) return FENIX_ERROR_UNINITIALIZED;

    fenix.callbacks.push_back(recover);

    return FENIX_SUCCESS;
}

int __fenix_callback_pop(){
   if(!fenix.fenix_init_flag) return FENIX_ERROR_UNINITIALIZED;
   if(fenix.callbacks.empty()) return FENIX_ERROR_CALLBACK_NOT_REGISTERED;

   fenix.callbacks.pop_back();

   return FENIX_SUCCESS;
}

void __fenix_callback_invoke_all(){
    //If callbacks are invoked in a nested manner due to caught exceptions
    //within a callback, we want to only finish the most recent call. All prior
    //calls should exit as soon as control returns.
    static int callbacks_depth = 0;
    int m_callbacks_layer = callbacks_depth++;

    try {
        for(auto& cb : fenix.callbacks) {
            if(callbacks_depth != m_callbacks_layer+1) break;
            cb(*fenix.user_world, fenix.mpi_fail_code);
        }
    } catch (const CommException& e) {
        switch(fenix.callback_exception_mode){
            case(RETHROW):
                if(m_callbacks_layer == 0) callbacks_depth = 0;
                throw;
            case(SQUASH):
                break;
        }
    }

    //Reset the callback depth when leaving the outermost call
    if(m_callbacks_layer == 0) callbacks_depth = 0;
}
