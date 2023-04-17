```
 ************************************************************************


            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
            _|        _|        _|_|    _|    _|      _|  _|
            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
            _|        _|        _|    _|_|    _|      _|  _|
            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|


 ************************************************************************
```

# Installation

These instructions assume you are in your home directory.

1. Checkout Fenix sources
   * For example: ` git clone <address of this repo> && cd Fenix`
2. Create a build directory.
3. Specify the MPI C compiler to use. [Open MPI 5+](https://github.com/open-mpi/ompi/tree/v5.0.x) is the required version.
   * Check out the CMake documentation for the best information on how to do this, but in general:
      * Set the CC environment variable to the correct `mpicc`,
      * Invoke cmake with `-DCMAKE_C_COMPILER=mpicc`,
      * Add the mpi install directory to CMAKE_PREFIX_PATH (see CMakeLists.txt FENIX_SYSTEM_INC_FIX option).
   * If you experience segmentation faults during simple MPI function calls, it is likely you have mixed up 
4. Run ` cmake ../ -DCMAKE_INSTALL_PREFIX=... && make install`
5. Optionally, add the install prefix to your CMAKE\_PREFIX\_PATHS environment variable, to enable `find_package(fenix)` in your other projects.


<pre>
// ************************************************************************
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
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY AND SANDIA 
// CORPORATION "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
// RUTGERS UNIVERSITY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE 
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY 
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
// OF SUCH DAMAGE.
//
// Authors Marc Gamell, Matthew Whitlock, Eric Valenzuela, Keita Teranishi, Manish Parashar
//        and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
// ************************************************************************
</pre>
