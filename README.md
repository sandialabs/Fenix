```
 ************************************************************************


            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
            _|        _|        _|_|    _|    _|      _|  _|
            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
            _|        _|        _|    _|_|    _|      _|  _|
            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|


 ************************************************************************
```

![R&D 100 2026 Winner Logo](/doc/images/RD100_2026_Winner_Logo_100px.png)

# About

Fenix is a software library compatible with the Message Passing
Interface (MPI) to support fault recovery without application
shutdown. Fenix has three components: process, data, and message
recovery. Following is a quick overview of the components; see our full
[documentation](https://sandialabs.github.io/Fenix/develop/index.html) for more
detail.

Fenix is compatible with both C and C++ applications (see `fenix.h` and
`fenix.hpp`), though it is most convenient for C++ applications that can
leverage RAII concepts, automatic destructors, and exception-based error
handling.

## Process Recovery

Process recovery is used to repair communicators whose ranks suffered failure
detected by the MPI runtime. Fenix does this by building a resilient
communicator that is automatically rebuilt for the application when a failure
is reported by MPI. This is accomplished using a custom MPI error handler on the
resilient communicator, not by using the PMPI layer, so Fenix's process recovery
is entirely compatible with MPI profiling/tuning tools.

Applications can choose how to be notified of a failure (the
`FENIX_RESUME_MODE`) after Fenix has rebuilt the resilient communicator with
any of the following modes:
 * `FENIX_RESUME_THROW`: The failed operation throws a `fenix::CommException`
 * `FENIX_RESUME_RETURN`: The failed operation returns the relevant MPI errcode
 * `FENIX_RESUME_JUMP`: Fenix longjmps back to `Fenix_Init`

Apps can also register recovery callbacks that will be invoked either
immediately before Fenix recovery or immediately before resuming the app.

The process recovery component is the core function of Fenix that the other
components build upon. When used alone, it saves apps from the expensive costs
associated with re-initializing MPI in large-scale jobs (potentially several
minutes per failed process). However, Fenix's strongest benefits come from
pairing this component with the data and/or message recovery components.

## Data Recovery

Fenix's data recovery component supports in-memory checkpoint/restart optimized
for online recovery. It minimizes collective synchronization requirements to
support our pseudo-local recovery model and maintain ~O(1) weak scaling.

This component supports a variety of mechanisms to store and restore data:
 * Registering raw memory regions
 * Registering pointers alongside a serialization function (which serializes
 using either a `FILE*` or `std::iostream`)
 * Requesting a `FILE*` or `std::iostream` to manually checkpoint/restart with

Checkpoints consist of three operations:
 * Staging data locally into Fenix's checkpoint stage
 * Storing data into a resilient memory space created alongside your cohort
 * Committing the stored data locally to confirm its validity as a checkpoint

Different functions in the data component may perform more than one operation,
as convenience functions. For instance, the typical user can simply call
`fenix::data::checkpoint` to handle all three steps at once.

## Message Recovery

Fenix's message recovery component can be used to save and replay message logs.
Applications can construct multiple logs on multiple comms, dynamically activate
or deactivate them, and can choose how and when to synchronize the logs and
replay any failed messages. Using this component, applications can perform non-
global restarts of (implicitly) coordinated checkpoints. It is not designed to
support uncoordinated checkpointing strategies. See a skeletonized stencil
example under `examples/08_inline_recovery` for a good idea of how message
recovery can simplify your recovery flow.

This component currently relies on PMPI layer overrides, so it is not compatible
with any other profiling/tuning libraries that do the same. To avoid polluting
the PMPI layer when linking against Fenix's other components, the PMPI overloads
are separated into a separate CMake module. To link against it, update your
CMakeLists.txt to use `find_package(fenix REQUIRE COMPONENTS mlog)` and link
against both the `fenix` and `fenix::mlog` libraries.

# Installation

These instructions assume you are in your home directory.

1. Checkout Fenix sources
   * For example: ` git clone <address of this repo> && cd Fenix`
2. Create a build directory.
3. Specify the MPI C compiler to use. [Open MPI 5+](https://github.com/open-mpi/ompi/tree/v5.0.x) is the only supported version at this time.
   * Check out the CMake documentation for the best information on how to do this, but in general:
      * Set the CC environment variable to the correct `mpicc`,
      * Invoke cmake with `-DCMAKE_C_COMPILER=mpicc`,
      * Add the mpi install directory to CMAKE\_PREFIX\_PATH.
   * If you experience segmentation faults during simple MPI function calls, this is often caused by accidentally building against multiple versions of MPI. See the FENIX\_SYSTEM\_INC\_FIX CMake option for a potential fix.
4. Run ` cmake ../ -DCMAKE_INSTALL_PREFIX=... && make install`
5. Optionally, add the install prefix to your CMAKE\_PREFIX\_PATH environment variable, to enable `find_package(fenix)` in your other projects.

# Papers and Articles

**Fenix rising.** (2026) M. E. Langely. *Sandia Lab News.*
https://www.sandia.gov/labnews/2026/09/24/fenix-rising/.

**Designing and Automating Asynchronous, Localized, Multi-Level Fault-Tolerance
at the Application Level [Dissertation].** (2024) M. Whitlock. *Georgia
Institute of Technology.* https://doi.org/1853/77831

**Asynchrony and Failure Masking via Pseudo-Local Process Recovery in MPI
Applications.** (2024) M. Whitlock, H. Kolla, A. Bouteiller, J. R. Mayo,
N. M. Morales, K. Teranishi, G. Bosilca. *2024 IEEE International Parallel and
Distributed Processing Symposium Workshop.*
https://doi.org/10.1109/IPDPSW63119.2024.00193

**Integrating process, control-flow, and data resiliency layers using a hybrid
Fenix/Kokkos approach.** (2022) M. Whitlock, N. M. Morales, G. Bosilca,
A. Bouteiller, B. Nicolae, K. Teranishi. *2022 IEEE International Conference on
Cluster Computing (CLUSTER).* https://doi.org/10.1109/CLUSTER51413.2022.00052

**Improving Scalability of Silent-Error Resilience for Message-Passing Solvers
via Local Recovery and Asynchrony.** (2020) H. Kolla, J. R. Mayo, K. Teranishi,
R. C. Armstrong. *2020 IEEE/ACM 10th Workshop on Fault Tolerance for HPC at 
eXtreme Scale (FTXS).* https://doi.org/10.1109/FTXS51974.2020.00006

**Integrating Inter-Node Communication with a Resilient Asynchronous Many-Task
Runtime System.** (2020) S. R. Paul, A. Hayashi, M. Whitlock, S. Bak,
K. Teranishi, J. Mayo, M. Grossman, V. Sarkar. *2020 Workshop on Exascale MPI
(ExaMPI).* https://doi.org/10.1109/ExaMPI52011.2020.00010

**Modeling and Simulating Multiple Failure Masking Enabled by Local Recovery for
Stencil-Based Applications at Extreme Scales.** (2017) M. Gamell, K. Teranishi,
J. Mayo, H. Kolla, M. A. Heroux, J. Chen, M. Parashar. *IEEE Transactions on
Parallel and Distributed Systems.* https://doi.org/10.1109/TPDS.2017.2696538

**Evaluating Online Global Recovery with Fenix Using Application-Aware In-Memory
Checkpointing Techniques.** (2016) M. Gamell, D. S. Katz, K. Teranishi,
M. A. Heroux, R. F. Van der Wijngaart, T. G. Mattson, M. Parashar. *2016 45th
International Conference on Parallel Processing Workshops (ICPPW).*
https://doi.org/10.1109/ICPPW.2016.56

**Exploring Automatic, Online Failure Recovery for Scientific Applications at
Extreme Scales.** (2014) M. Gamell, D. S. Katz, H. Kolla, J. Chen, S. Klasky,
M. Parashar. *SC '14: Proceedings of the International Conference for High
Performance Computing, Networking, Storage and Analysis.* 
https://doi.org/10.1109/SC.2014.78


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
// Questions? Contact Matthew Whitlock (mwhitlo@sandia.gov)
// ************************************************************************
</pre>
