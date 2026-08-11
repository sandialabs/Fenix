Installation Guide
==================

This guide covers installing Fenix from source. For a quick install, see the :doc:`quickstart`.

.. contents:: On This Page
   :local:
   :depth: 2

Requirements
------------

Before installing Fenix, ensure you have:

Required
~~~~~~~~

- **Open MPI 5.0 or later** with ULFM (User Level Failure Mitigation) support.
  ULFM is an MPI extension that provides low-level fault tolerance mechanisms
  (failure detection, communicator repair, etc.) that Fenix builds upon.
- **CMake 3.12 or later**
- **C++20 compatible compiler** (GCC 10+, Clang 10+, or equivalent)
- **MPI C and C++ compilers** (mpicc, mpicxx)

Optional
~~~~~~~~

- **Doxygen** (for building API documentation)
- **Sphinx** (for building user documentation)
- **Graphviz** (for rendering diagrams in documentation - optional, docs build without it)
- **Google Test** (for running tests, included if not found)

Checking Your MPI Installation
-------------------------------

Verify Open MPI Version
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   mpiexec --version

You should see "Open MPI" version 5.0 or later.

Check for ULFM Support
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   mpiexec --with-ft mpi --version

If this works without error, you have ULFM support. If you get an error like:

.. code-block:: text

   mpiexec: Error: unknown option "--with-ft"

Then your Open MPI was built without fault tolerance support.

Installing Open MPI with ULFM
------------------------------

If you don't have Open MPI 5+ with ULFM, you'll need to build it from source.

Quick Install
~~~~~~~~~~~~~

.. code-block:: bash

   # Download Open MPI 5.0
   wget https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.0.tar.gz
   tar xzf openmpi-5.0.0.tar.gz
   cd openmpi-5.0.0

   # Configure with fault tolerance enabled
   ./configure --prefix=$HOME/openmpi-5.0 \
     --with-ft=mpi \
     --enable-mpi-ft-mpi

   # Build and install
   make -j4
   make install

   # Add to PATH
   export PATH=$HOME/openmpi-5.0/bin:$PATH
   export LD_LIBRARY_PATH=$HOME/openmpi-5.0/lib:$LD_LIBRARY_PATH

Add the ``export`` lines to your ``~/.bashrc`` to make them permanent.

Detailed Instructions
~~~~~~~~~~~~~~~~~~~~~

For detailed Open MPI build instructions, see:
https://github.com/open-mpi/ompi/tree/v5.0.x

Building Fenix
--------------

Basic Installation
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/sandialabs/Fenix.git
   cd Fenix

   # Create build directory
   mkdir build && cd build

   # Configure
   cmake ../ \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_INSTALL_PREFIX=$HOME/fenix

   # Build
   make -j4

   # Install
   make install

This installs to ``$HOME/fenix`` by default. Choose any location you prefer.

Recommended Build Options
~~~~~~~~~~~~~~~~~~~~~~~~~~

For development and learning:

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_INSTALL_PREFIX=$HOME/fenix \
     -DCMAKE_BUILD_TYPE=Debug \
     -DBUILD_EXAMPLES=ON \
     -DBUILD_TESTING=ON \
     -DBUILD_DOCS=ON

For production use:

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_INSTALL_PREFIX=/opt/fenix \
     -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_TESTING=OFF

CMake Options Reference
-----------------------

Build Configuration
~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Option
     - Default
     - Description
   * - ``CMAKE_BUILD_TYPE``
     - Release
     - Build type: ``Debug``, ``Release``, ``RelWithDebInfo``
   * - ``CMAKE_INSTALL_PREFIX``
     - /usr/local
     - Installation directory
   * - ``CMAKE_C_COMPILER``
     - (auto)
     - MPI C compiler (use ``mpicc``)
   * - ``CMAKE_CXX_COMPILER``
     - (auto)
     - MPI C++ compiler (use ``mpicxx``)

Feature Options
~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 35 15 50

   * - Option
     - Default
     - Description
   * - ``BUILD_EXAMPLES``
     - OFF
     - Build example programs
   * - ``BUILD_TESTING``
     - ON
     - Build test suite
   * - ``BUILD_DOCS``
     - ON
     - Build documentation (requires Doxygen)
   * - ``DOCS_ONLY``
     - OFF
     - Only build documentation, skip library

Advanced Options
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 35 15 50

   * - Option
     - Default
     - Description
   * - ``FENIX_SYSTEM_INC_FIX``
     - ON
     - Fix for multiple MPI versions (recommended)
   * - ``FENIX_PROPAGATE_INC_FIX``
     - ON
     - Propagate MPI header fix to linking projects
   * - ``FENIX_C_CATCH_RUNTIME_EXCEPTIONS``
     - OFF
     - Catch runtime exceptions in C API
   * - ``FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS``
     - OFF
     - Catch runtime exceptions in C++ API

Verifying Installation
----------------------

Run Tests
~~~~~~~~~

.. code-block:: bash

   cd build
   ctest -V --timeout 20

All tests should pass. If tests fail, see :doc:`troubleshooting`.

Build an Example
~~~~~~~~~~~~~~~~

.. code-block:: bash

   cd build/examples/08_inline_recovery
   mpiexec --with-ft mpi -n 7 ./stencil_skeleton

Check Installed Files
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   ls $HOME/fenix/
   # Should see: include/ lib/ share/

   ls $HOME/fenix/include/
   # Should see: fenix.h fenix.hpp fenix_ext.hpp fenix/ ...

   ls $HOME/fenix/lib/
   # Should see: libfenix.a libmlog.a cmake/ ...

Using Fenix in Your Project
----------------------------

With CMake
~~~~~~~~~~

Add to your ``CMakeLists.txt``:

.. code-block:: cmake

   # Find Fenix
   find_package(fenix REQUIRED)

   # Link your executable
   add_executable(my_app my_app.cpp)
   target_link_libraries(my_app fenix)

Set ``CMAKE_PREFIX_PATH`` when configuring:

.. code-block:: bash

   cmake -DCMAKE_PREFIX_PATH=$HOME/fenix ..

Without CMake
~~~~~~~~~~~~~

Compile manually:

.. code-block:: bash

   # C++ application
   mpicxx -std=c++17 my_app.cpp -o my_app \
     -I$HOME/fenix/include \
     -L$HOME/fenix/lib -lfenix

   # C application
   mpicc my_app.c -o my_app \
     -I$HOME/fenix/include \
     -L$HOME/fenix/lib -lfenix

Add to environment:

.. code-block:: bash

   export CPATH=$HOME/fenix/include:$CPATH
   export LIBRARY_PATH=$HOME/fenix/lib:$LIBRARY_PATH
   export LD_LIBRARY_PATH=$HOME/fenix/lib:$LD_LIBRARY_PATH

Common Installation Issues
--------------------------

Multiple MPI Versions
~~~~~~~~~~~~~~~~~~~~~

**Problem:** Segfaults in basic MPI calls, even without failures.

**Cause:** System has multiple MPI installations, and Fenix is picking up wrong headers.

**Solution:** Enable the system include fix:

.. code-block:: bash

   cmake ../ -DFENIX_SYSTEM_INC_FIX=ON ...

This forces Fenix to use the correct MPI headers.

Missing --with-ft Flag
~~~~~~~~~~~~~~~~~~~~~~

**Problem:** ``mpiexec: Error: unknown option "--with-ft"``

**Cause:** Open MPI not built with ULFM support.

**Solution:** Rebuild Open MPI with ``--with-ft=mpi`` option (see above).

CMake Can't Find MPI
~~~~~~~~~~~~~~~~~~~~

**Problem:** ``CMake Error: Could not find MPI``

**Solution:** Specify MPI compilers explicitly:

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=/path/to/mpicc \
     -DCMAKE_CXX_COMPILER=/path/to/mpicxx

Or add MPI to ``CMAKE_PREFIX_PATH``:

.. code-block:: bash

   cmake ../ -DCMAKE_PREFIX_PATH=/path/to/openmpi

C++20 Errors
~~~~~~~~~~~~

**Problem:** Compilation errors about C++20 features.

**Cause:** Older compiler or not specifying C++20.

**Solution:** Ensure GCC 10+ or Clang 10+, and set C++ standard:

.. code-block:: bash

   cmake ../ -DCMAKE_CXX_STANDARD=20

Tests Hang
~~~~~~~~~~

**Problem:** ``ctest`` hangs indefinitely.

**Cause:** Not using fault-tolerant MPI launch.

**Solution:** Tests automatically use ``--with-ft mpi``. If hanging, check:

.. code-block:: bash

   # Verify MPI works
   mpiexec --with-ft mpi -n 2 hostname

If this hangs, your MPI installation has issues.

Platform-Specific Notes
-----------------------

Linux (Ubuntu/Debian)
~~~~~~~~~~~~~~~~~~~~~

Install build dependencies:

.. code-block:: bash

   sudo apt-get install build-essential cmake git
   sudo apt-get install libopenmpi-dev  # May not have ULFM

For documentation building:

.. code-block:: bash

   sudo apt-get install doxygen graphviz

.. note::
   Graphviz (provides the ``dot`` command) is optional. Without it, diagrams
   won't render in the documentation, but the build will still succeed.

You'll likely need to build Open MPI from source for ULFM support.

Linux (RHEL/CentOS/Rocky)
~~~~~~~~~~~~~~~~~~~~~~~~~~

Install build dependencies:

.. code-block:: bash

   sudo yum groupinstall "Development Tools"
   sudo yum install cmake git
   sudo yum install openmpi-devel  # May not have ULFM

For documentation building:

.. code-block:: bash

   sudo yum install doxygen graphviz

HPC Systems (SLURM/PBS)
~~~~~~~~~~~~~~~~~~~~~~~~

On HPC systems, use the system's MPI module:

.. code-block:: bash

   module load openmpi/5.0
   # Or: module load mpi/openmpi-5.0

   # Then build Fenix as normal
   cmake ../ -DCMAKE_C_COMPILER=mpicc ...

Check with your system administrator if Open MPI 5 with ULFM is available.

macOS
~~~~~

Install dependencies with Homebrew:

.. code-block:: bash

   brew install cmake
   brew install open-mpi  # May not have ULFM

For documentation building:

.. code-block:: bash

   brew install doxygen graphviz

You'll likely need to build Open MPI from source for ULFM support.

Next Steps
----------

After successful installation:

🚀 **Try the Quick Start:** :doc:`quickstart`

📚 **Explore Examples:** :doc:`examples/index`

🔨 **Start a Tutorial:** :doc:`tutorials/index`

📖 **Read the Guide:** :doc:`guides/index`

Need Help?
----------

- 🐛 **Installation problems:** :doc:`troubleshooting`
- 💬 **Questions:** See :doc:`faq`
- 🔧 **Build issues:** Check the `GitHub Issues <https://github.com/sandialabs/Fenix/issues>`_
