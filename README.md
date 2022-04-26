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
   * For example: ` git clone <address of this repo> `
2. Create a build directory.
   * For example: ` mkdir -p ~/build/fenix/ && cd ~/build/fenix/ `
3. Specify the MPI C compiler to use. [ULFM2 Open MPI](https://bitbucket.org/icldistcomp/ulfm2) is the required version.
   * To manually indicate which compiler `cmake` should use, set the `MPICC` variable to point to it.
      * For example: ` export MPICC=~/install/mpi-ulfm/bin/mpicc `
   * If the `MPICC` environment variable is not there, `cmake` will try to guess where the MPI implementation is. To help, make sure you include the installation directory of MPI in your `PATH`.
      * For example: ` export PATH=~/install/mpi-ulfm/bin:$PATH `
4. Run ` cmake <Fenix source directory> ` and ` make `
   * For example: ` cmake ~/Fenix && make `
5. For best compatibility with other cmake projects, run ` make install ` and add the install directory to your CMAKE\_PREFIX\_PATH


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
