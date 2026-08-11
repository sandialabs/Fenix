```
 ╔═══════════════════════════════════════════════════════════════════════╗
 ║                                                                       ║
 ║       _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|            ║
 ║       _|        _|        _|_|    _|    _|      _|  _|               ║
 ║       _|_|_|    _|_|_|    _|  _|  _|    _|        _|                 ║
 ║       _|        _|        _|    _|_|    _|      _|  _|               ║
 ║       _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|            ║
 ║                                                                       ║
 ║              Fault Tolerance for MPI Applications                    ║
 ║                                                                       ║
 ╚═══════════════════════════════════════════════════════════════════════╝
```

[![Documentation](https://img.shields.io/badge/docs-latest-blue.svg)](https://sandialabs.github.io/Fenix/develop/index.html)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-green.svg)](LICENSE)

# About

Fenix is a software library compatible with the Message Passing Interface (MPI) that enables **fault recovery without application shutdown**. When MPI ranks fail, Fenix automatically repairs communicators and allows your application to continue running.

## Key Features

🛡️ **Process Recovery** - Automatically repair communicators when ranks fail using spare ranks

💾 **Data Recovery** - Optional high-performance in-memory checkpoint/restart with RAID-style redundancy

📨 **Message Recovery** - Optional message logging and replay for localized fault tolerance

🔄 **Flexible Recovery Patterns** - Choose between longjmp-based or inline recovery patterns

⚡ **High Performance** - Minimal overhead during normal execution, fast recovery

## Quick Links

- 📚 **[Full Documentation](https://sandialabs.github.io/Fenix/develop/index.html)** - Complete guides, tutorials, and API reference
- 🚀 **[Quick Start](https://sandialabs.github.io/Fenix/develop/quickstart.html)** - Get running in 10 minutes
- 📖 **[Examples](examples/)** - Sample programs demonstrating Fenix features
- 🐛 **[Troubleshooting](https://sandialabs.github.io/Fenix/develop/troubleshooting.html)** - Common issues and solutions

## What Makes Fenix Different?

Unlike traditional checkpoint/restart that stops and restarts your entire application:

- ✅ **No full restart** - Continue execution with minimal interruption
- ✅ **Automatic recovery** - Fenix handles communicator repair transparently  
- ✅ **Flexible data recovery** - Use built-in checkpointing or integrate with your own
- ✅ **MPI-native** - Works with standard MPI code patterns

# Installation

## Requirements

- **Open MPI 5.0+** with ULFM (User Level Failure Mitigation) support
- **CMake 3.12+**
- **C++20** compatible compiler
- **Graphviz** (optional, for building documentation with diagrams)

## Quick Install

```bash
# Clone the repository
git clone https://github.com/sandialabs/Fenix.git
cd Fenix

# Configure and build
mkdir build && cd build
cmake ../ \
  -DCMAKE_C_COMPILER=mpicc \
  -DCMAKE_CXX_COMPILER=mpicxx \
  -DCMAKE_INSTALL_PREFIX=$HOME/fenix \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTING=ON

# Build and install
make -j4
make install

# Run tests to verify
ctest -V --timeout 20
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `BUILD_TESTING` | ON | Build test suite |
| `BUILD_DOCS` | ON | Build documentation (requires Doxygen) |
| `FENIX_SYSTEM_INC_FIX` | ON | Fix for multiple MPI versions (use if seeing segfaults) |
| `FENIX_C_CATCH_RUNTIME_EXCEPTIONS` | OFF | Catch runtime exceptions in C API |
| `FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS` | OFF | Catch runtime exceptions in C++ API |

## Using Fenix in Your Project

### With CMake

Add to your `CMakeLists.txt`:

```cmake
find_package(fenix REQUIRED)
target_link_libraries(your_target fenix)
```

Make sure `CMAKE_PREFIX_PATH` includes your Fenix installation:

```bash
export CMAKE_PREFIX_PATH=$HOME/fenix:$CMAKE_PREFIX_PATH
```

### Manual Compilation

```bash
mpicc your_program.c -o your_program \
  -I$HOME/fenix/include \
  -L$HOME/fenix/lib -lfenix
```

## Running Fenix Programs

**Important:** You must use the `--with-ft mpi` flag when running Fenix programs:

```bash
mpiexec --with-ft mpi -n <num_ranks> ./your_program
```

For development/testing (allows running as root and oversubscription):

```bash
mpiexec --with-ft mpi --allow-run-as-root --map-by :oversubscribe \
  --mca async_mpi_finalize 1 -n <num_ranks> ./your_program
```

## Troubleshooting Installation

**Multiple MPI versions causing segfaults:**
```bash
cmake ../ -DFENIX_SYSTEM_INC_FIX=ON ...
```

**Open MPI doesn't support `--with-ft mpi`:**

You need Open MPI 5+ with ULFM support. See [Open MPI ULFM documentation](https://github.com/open-mpi/ompi/tree/v5.0.x) for build instructions.

**Tests failing:**

Ensure you're running tests with fault tolerance enabled:
```bash
ctest -V --timeout 20
```

For more help, see the [troubleshooting guide](https://sandialabs.github.io/Fenix/develop/troubleshooting.html).


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
