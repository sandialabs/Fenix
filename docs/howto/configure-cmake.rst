Configure CMake Build
=====================

This guide explains all Fenix CMake build options and shows you how to configure builds for different scenarios including development, production, and cross-compilation.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Basic build configuration:

.. code-block:: bash

   mkdir build && cd build
   cmake ../ \
     -DCMAKE_INSTALL_PREFIX=/path/to/install \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_BUILD_TYPE=Release
   make -j
   make install

All CMake Options
------------------

Build Control Options
~~~~~~~~~~~~~~~~~~~~~

**BUILD_EXAMPLES**

- **Type:** Boolean (ON/OFF)
- **Default:** OFF
- **Purpose:** Build example programs from the ``examples/`` directory

.. code-block:: bash

   cmake ../ -DBUILD_EXAMPLES=ON

Example programs demonstrate Fenix usage patterns and are useful for learning and testing.

**BUILD_TESTING**

- **Type:** Boolean (ON/OFF)
- **Default:** ON
- **Purpose:** Build test suite and enable ``ctest``

.. code-block:: bash

   cmake ../ -DBUILD_TESTING=ON
   make
   ctest -V

Tests verify Fenix functionality and are essential for development.

**BUILD_DOCS**

- **Type:** Boolean (ON/OFF)
- **Default:** ON
- **Purpose:** Build documentation if Sphinx is found

.. code-block:: bash

   cmake ../ -DBUILD_DOCS=ON
   make  # Also builds docs
   # Or explicitly:
   make sphinx-doc

Requires Sphinx and breathe for building documentation.

**DOCS_ONLY**

- **Type:** Boolean (ON/OFF)
- **Default:** OFF
- **Purpose:** Only build documentation, skip library compilation

.. code-block:: bash

   cmake ../ -DDOCS_ONLY=ON
   make sphinx-doc

Useful for documentation-only builds in CI or on machines without MPI.

Exception Handling Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**FENIX_C_CATCH_RUNTIME_EXCEPTIONS**

- **Type:** Boolean (ON/OFF)
- **Default:** OFF
- **Purpose:** Catch C++ exceptions in C API and return error codes

.. code-block:: bash

   cmake ../ -DFENIX_C_CATCH_RUNTIME_EXCEPTIONS=ON

**When to enable:**

- Interfacing with pure C code that can't handle C++ exceptions
- Need error codes instead of exceptions in C API
- Debugging exception-related issues

**Effect:**

.. code-block:: c

   // With OFF (default):
   // Exceptions propagate to caller

   // With ON:
   int error;
   Fenix_Data_member_restore(group_id, member_id, buffer, count,
                             FENIX_DATA_SNAPSHOT_LATEST, &error);
   if (error != FENIX_SUCCESS) {
     // Exception was caught and converted to error code
   }

**FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS**

- **Type:** Boolean (ON/OFF)
- **Default:** OFF
- **Purpose:** Catch C++ exceptions in C++ API and return error codes

.. code-block:: bash

   cmake ../ -DFENIX_CPP_CATCH_RUNTIME_EXCEPTIONS=ON

**When to enable:**

- Prefer error codes over exceptions in C++ API
- Integrating with exception-free C++ codebases
- Performance-critical code avoiding exception overhead

**Effect:**

.. code-block:: cpp

   // With OFF (default):
   try {
     fenix::data::member_restore(group_id, member_id);
   } catch (fenix::Exception& e) {
     // Handle exception
   }

   // With ON:
   int error = fenix::data::member_restore(group_id, member_id);
   if (error != FENIX_SUCCESS) {
     // Error code returned instead of exception
   }

MPI Header Override Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**FENIX_SYSTEM_INC_FIX**

- **Type:** Boolean (ON/OFF)
- **Default:** ON
- **Purpose:** Force Fenix to use correct MPI headers when multiple MPI installations exist

.. code-block:: bash

   cmake ../ -DFENIX_SYSTEM_INC_FIX=ON

**When to enable (keep ON):**

- Multiple MPI versions installed on system
- Experiencing segfaults with MPI calls
- System MPI headers conflict with target MPI

**When to disable:**

- Only one MPI installation on system
- Conflicts with other build system behavior
- You are certain correct MPI headers are used

**Problem it solves:**

.. code-block:: bash

   # System has MPI in /usr/include (version 3)
   # You want to use MPI in /opt/mpi (version 5)
   # CMake might include both, causing crashes

   # With FENIX_SYSTEM_INC_FIX=ON:
   # Fenix forces use of /opt/mpi headers only

**FENIX_PROPAGATE_INC_FIX**

- **Type:** Boolean (ON/OFF)
- **Default:** ON
- **Purpose:** Apply MPI header fix to projects that link against Fenix

.. code-block:: bash

   cmake ../ -DFENIX_PROPAGATE_INC_FIX=ON

**When to enable:**

- Your application might also see wrong MPI headers
- Want consistent MPI headers throughout stack

**When to disable:**

- Application handles MPI headers correctly
- Conflicts with application's build system

Build Type and Compiler Options
--------------------------------

CMAKE_BUILD_TYPE
~~~~~~~~~~~~~~~~

**Release (Recommended for production)**

.. code-block:: bash

   cmake ../ -DCMAKE_BUILD_TYPE=Release

- Optimizations enabled (-O3)
- Debug symbols stripped
- Assertions disabled
- Fastest runtime performance

**Debug (Recommended for development)**

.. code-block:: bash

   cmake ../ -DCMAKE_BUILD_TYPE=Debug

- No optimizations (-O0)
- Full debug symbols (-g)
- Assertions enabled
- Easy debugging with gdb/lldb

**RelWithDebInfo (Production debugging)**

.. code-block:: bash

   cmake ../ -DCMAKE_BUILD_TYPE=RelWithDebInfo

- Optimizations enabled (-O2)
- Debug symbols included (-g)
- Good performance with debuggability
- Useful for production debugging

**MinSizeRel (Embedded/constrained systems)**

.. code-block:: bash

   cmake ../ -DCMAKE_BUILD_TYPE=MinSizeRel

- Optimize for size (-Os)
- Smallest binary size
- Slightly slower than Release

Compiler Selection
~~~~~~~~~~~~~~~~~~

**MPI Compilers (Recommended)**

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx

Automatically includes MPI flags and libraries.

**Specific MPI Installation**

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=/opt/openmpi-5/bin/mpicc \
     -DCMAKE_CXX_COMPILER=/opt/openmpi-5/bin/mpicxx

**Manual MPI Specification**

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=gcc \
     -DCMAKE_CXX_COMPILER=g++ \
     -DMPI_C_COMPILER=/path/to/mpicc \
     -DMPI_CXX_COMPILER=/path/to/mpicxx

Install Location
~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake ../ -DCMAKE_INSTALL_PREFIX=/path/to/install
   make install

**Installs:**

- Headers: ``$PREFIX/include/``
- Libraries: ``$PREFIX/lib/``
- CMake config: ``$PREFIX/cmake/``

Common Configuration Examples
------------------------------

Development Build
~~~~~~~~~~~~~~~~~

Optimized for debugging and testing:

.. code-block:: bash

   mkdir build-dev && cd build-dev
   cmake ../ \
     -DCMAKE_BUILD_TYPE=Debug \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DBUILD_EXAMPLES=ON \
     -DBUILD_TESTING=ON \
     -DBUILD_DOCS=OFF \
     -DFENIX_SYSTEM_INC_FIX=ON
   make -j$(nproc)

   # Run tests
   ctest -V --timeout 20

Production Build
~~~~~~~~~~~~~~~~

Optimized for performance:

.. code-block:: bash

   mkdir build-release && cd build-release
   cmake ../ \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_INSTALL_PREFIX=/usr/local \
     -DBUILD_EXAMPLES=OFF \
     -DBUILD_TESTING=OFF \
     -DBUILD_DOCS=OFF \
     -DFENIX_SYSTEM_INC_FIX=ON
   make -j$(nproc)
   sudo make install

Testing Build
~~~~~~~~~~~~~

For continuous integration:

.. code-block:: bash

   mkdir build-ci && cd build-ci
   cmake ../ \
     -DCMAKE_BUILD_TYPE=RelWithDebInfo \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DBUILD_EXAMPLES=ON \
     -DBUILD_TESTING=ON \
     -DBUILD_DOCS=ON \
     -DFENIX_C_CATCH_RUNTIME_EXCEPTIONS=ON \
     -DFENIX_CPP_CATCH_RUNTIME_EXCEPTIONS=ON
   make -j$(nproc)

   # Run tests with repetition to catch flaky tests
   ctest -V --timeout 20 --repeat until-fail:5

Documentation-Only Build
~~~~~~~~~~~~~~~~~~~~~~~~

Build docs without compiling library:

.. code-block:: bash

   mkdir build-docs && cd build-docs
   cmake ../ -DDOCS_ONLY=ON
   make sphinx-doc

   # Documentation in: _build/html/

Custom MPI Build
~~~~~~~~~~~~~~~~

With specific MPI installation:

.. code-block:: bash

   MPI_ROOT=/opt/openmpi-5.0
   mkdir build && cd build
   cmake ../ \
     -DCMAKE_C_COMPILER=${MPI_ROOT}/bin/mpicc \
     -DCMAKE_CXX_COMPILER=${MPI_ROOT}/bin/mpicxx \
     -DCMAKE_PREFIX_PATH=${MPI_ROOT} \
     -DCMAKE_INSTALL_PREFIX=$HOME/fenix \
     -DCMAKE_BUILD_TYPE=Release \
     -DFENIX_SYSTEM_INC_FIX=ON
   make -j
   make install

Cross-Compilation Setup
-----------------------

ARM64 Cross-Compilation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Create toolchain file: toolchain-arm64.cmake
   cat > toolchain-arm64.cmake << 'EOF'
   set(CMAKE_SYSTEM_NAME Linux)
   set(CMAKE_SYSTEM_PROCESSOR aarch64)

   set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
   set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

   set(MPI_C_COMPILER /opt/arm-mpi/bin/mpicc)
   set(MPI_CXX_COMPILER /opt/arm-mpi/bin/mpicxx)

   set(CMAKE_FIND_ROOT_PATH /opt/arm-sysroot)
   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
   set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
   EOF

   # Build
   mkdir build-arm64 && cd build-arm64
   cmake ../ \
     -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64.cmake \
     -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_TESTING=OFF
   make -j

For HPC Systems
~~~~~~~~~~~~~~~

Many HPC systems provide environment modules:

.. code-block:: bash

   # Load required modules
   module load openmpi/5.0.0
   module load cmake/3.25

   # Build with system compilers
   mkdir build && cd build
   cmake ../ \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=$HOME/software/fenix
   make -j
   make install

Troubleshooting CMake Issues
-----------------------------

Problem: CMake can't find MPI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:**

.. code-block:: text

   CMake Error: Could not find MPI

**Solutions:**

1. Use MPI compiler wrappers:

   .. code-block:: bash

      cmake ../ -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx

2. Specify MPI location:

   .. code-block:: bash

      cmake ../ -DMPI_HOME=/path/to/mpi

3. Load MPI module (HPC systems):

   .. code-block:: bash

      module load openmpi
      cmake ../

Problem: Wrong MPI headers used
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:**

- Segfaults in MPI calls
- Undefined MPI symbols
- Compilation errors about MPI types

**Solutions:**

1. Enable FENIX_SYSTEM_INC_FIX:

   .. code-block:: bash

      cmake ../ -DFENIX_SYSTEM_INC_FIX=ON

2. Verify MPI compiler:

   .. code-block:: bash

      which mpicc
      mpicc --version

3. Clean and rebuild:

   .. code-block:: bash

      rm -rf build/*
      cmake ../

Problem: Tests fail to run
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:**

.. code-block:: text

   mpiexec: command not found
   Test timeout exceeded

**Solutions:**

1. Ensure MPI runtime is in PATH:

   .. code-block:: bash

      which mpiexec
      export PATH=/opt/mpi/bin:$PATH

2. Check MPI is built with fault tolerance:

   .. code-block:: bash

      ompi_info | grep ft

3. Run tests with correct flags:

   .. code-block:: bash

      ctest -V --timeout 20

Problem: Build fails with C++20 errors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:**

.. code-block:: text

   error: 'concept' does not name a type
   error: C++20 required

**Solutions:**

1. Use recent compiler:

   .. code-block:: bash

      cmake ../ -DCMAKE_CXX_COMPILER=g++-11

2. Verify compiler version:

   .. code-block:: bash

      g++ --version  # Need >= 10

Problem: Installation permission denied
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:**

.. code-block:: text

   make install
   Permission denied: /usr/local/include

**Solutions:**

1. Use user-writable prefix:

   .. code-block:: bash

      cmake ../ -DCMAKE_INSTALL_PREFIX=$HOME/fenix
      make install

2. Or use sudo:

   .. code-block:: bash

      sudo make install

Verification
------------

After building, verify installation:

**Check installed files:**

.. code-block:: bash

   ls $PREFIX/include/fenix*.h*
   ls $PREFIX/lib/libfenix.*
   ls $PREFIX/cmake/fenix*.cmake

**Test find_package:**

.. code-block:: bash

   cat > test_find.cmake << 'EOF'
   cmake_minimum_required(VERSION 3.23)
   project(test)
   find_package(fenix REQUIRED)
   message(STATUS "Found Fenix: ${fenix_FOUND}")
   EOF

   cmake -P test_find.cmake -DCMAKE_PREFIX_PATH=$PREFIX

**Run example:**

.. code-block:: bash

   # If BUILD_EXAMPLES=ON
   cd build/examples/01_hello_world
   mpiexec --with-ft mpi -n 4 ./fenix_hello_world

Build Performance Tips
----------------------

Parallel Build
~~~~~~~~~~~~~~

.. code-block:: bash

   make -j$(nproc)  # Use all cores

   # Or specify:
   make -j8  # Use 8 cores

Ccache for Faster Rebuilds
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Install ccache
   sudo apt install ccache  # Debian/Ubuntu

   # Configure CMake to use it
   cmake ../ \
     -DCMAKE_C_COMPILER_LAUNCHER=ccache \
     -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

Ninja Build System
~~~~~~~~~~~~~~~~~~

Faster than Make:

.. code-block:: bash

   cmake ../ -GNinja
   ninja -j$(nproc)
   ninja install

Out-of-Source Builds
~~~~~~~~~~~~~~~~~~~~

Keep source tree clean:

.. code-block:: bash

   # Multiple build configurations
   mkdir -p build/{debug,release,testing}

   cd build/debug
   cmake ../../ -DCMAKE_BUILD_TYPE=Debug
   make -j

   cd ../release
   cmake ../../ -DCMAKE_BUILD_TYPE=Release
   make -j

See Also
--------

- :doc:`integrate-cmake-project` - Using Fenix in your CMake project
- :doc:`/troubleshooting` - Common build problems
- :doc:`test-locally` - Testing your Fenix build
