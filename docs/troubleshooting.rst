Troubleshooting Guide
=====================

Common problems and solutions when working with Fenix.

.. contents:: Quick Jump
   :local:
   :depth: 2

Installation Issues
-------------------

mpiexec: unknown option "--with-ft"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: text

   mpiexec: Error: unknown option "--with-ft"

**Cause:** Your Open MPI was not built with ULFM (fault tolerance) support.

**Solution:**

You need Open MPI 5.0+ with ULFM enabled. Build from source:

.. code-block:: bash

   ./configure --prefix=$HOME/openmpi-5.0 \
     --with-ft=mpi \
     --enable-mpi-ft-mpi
   make -j4 && make install

See :doc:`installation` for complete instructions.

Segfault in MPI Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Program crashes with segfault in basic MPI calls like ``MPI_Init`` or ``MPI_Comm_rank``.

**Cause:** Multiple MPI versions on system. Fenix was compiled against one MPI installation's headers, but at runtime the program links to a different MPI installation's libraries. This causes binary incompatibility.

**Solution 1 - Enable System Include Fix:**

.. code-block:: bash

   cd build
   cmake ../ -DFENIX_SYSTEM_INC_FIX=ON
   make clean && make

This forces Fenix to use the correct MPI headers.

**Solution 2 - Explicitly Specify MPI:**

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=/path/to/correct/mpicc \
     -DCMAKE_CXX_COMPILER=/path/to/correct/mpicxx \
     -DCMAKE_PREFIX_PATH=/path/to/correct/openmpi

**Verification:**

.. code-block:: bash

   ldd build/examples/08_inline_recovery/stencil_skeleton | grep mpi
   # Should show only ONE libmpi.so path

CMake Can't Find MPI
~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: text

   CMake Error: Could not find MPI

**Solution:** Set ``CMAKE_PREFIX_PATH`` to your MPI installation:

.. code-block:: bash

   cmake ../ -DCMAKE_PREFIX_PATH=/path/to/openmpi

Or specify compilers explicitly:

.. code-block:: bash

   cmake ../ \
     -DCMAKE_C_COMPILER=$(which mpicc) \
     -DCMAKE_CXX_COMPILER=$(which mpicxx)

C++20 Compilation Errors
~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Errors about designated initializers or C++20 features.

**Cause:** Compiler too old or C++20 not enabled.

**Solution:**

Ensure GCC 10+, Clang 10+, or equivalent:

.. code-block:: bash

   g++ --version  # Should be 10.0 or later

Force C++20:

.. code-block:: bash

   cmake ../ -DCMAKE_CXX_STANDARD=20

Runtime Issues
--------------

Program Hangs at MPI_Init
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Program hangs indefinitely at ``MPI_Init`` or shortly after.

**Common Causes:**

1. **Missing --with-ft flag**

   .. code-block:: bash

      # Wrong
      mpiexec -n 4 ./my_app

      # Correct
      mpiexec --with-ft mpi -n 4 ./my_app

2. **Firewall blocking MPI communication**

   Temporarily disable or configure firewall to allow MPI ports.

3. **SSH not configured** (for multi-node)

   Set up passwordless SSH between nodes.

**Debug Steps:**

.. code-block:: bash

   # Test basic MPI
   mpiexec --with-ft mpi -n 2 hostname

   # Test with oversubscription
   mpiexec --with-ft mpi --map-by :oversubscribe -n 4 hostname

Program Crashes on Failure
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Instead of recovering, program crashes when rank fails.

**Cause 1:** Not enough spare ranks.

**Solution:** Increase spare count:

.. code-block:: cpp

   fenix::init({.out_comm = &res_comm, .spares = 3});  // More spares

**Cause 2:** Error in recovery callback.

**Solution:** Add error checking in callback:

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     if (fenix::error() != FENIX_SUCCESS) {
       printf("Recovery failed: %d\\n", fenix::error());
       return;
     }
     // ... recovery logic ...
   });

**Cause 3:** Message logging not activated.

**Solution:** Ensure message logs are activated:

.. code-block:: cpp

   fenix::mlog::activate(log_id);

Recovered Data is Incorrect
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** After recovery, restored data doesn't match what was checkpointed.

**Common Causes:**

1. **Buffer pointer changed**

   For resizable data (``std::vector``), update buffer pointer:

   .. code-block:: cpp

      data.resize(new_size);
      Fenix_Data_member_attr_set(
        group, member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
        data.data(), &flag
      );

2. **Wrong snapshot restored**

   Specify which snapshot:

   .. code-block:: cpp

      member_restore(group, member, NULL, 0,
                     FENIX_DATA_SNAPSHOT_LATEST, subset);

3. **Subset mismatch**

   Ensure the same element ranges are used for store and restore. If you used ``SUBSET_FULL`` for store, use ``SUBSET_FULL`` for restore (or let it default). If you stored a custom subset, restore must match those same element ranges.

**Debug Steps:**

.. code-block:: cpp

   // Check what was stored
   DataSubset stored_subset;
   member_restore(group, member, nullptr, 0,
                  FENIX_DATA_SNAPSHOT_LATEST, stored_subset);
   printf("Stored size: %d\\n", stored_subset.max_count());

Out of Spares
~~~~~~~~~~~~~

**Problem:**

.. code-block:: text

   Fenix: Error: No spare ranks available

**Cause:** More failures than spare ranks.

**Solution 1 - Increase Spares:**

.. code-block:: cpp

   fenix::init({.out_comm = &res_comm, .spares = 5});

Rule of thumb: 5-10% of total ranks for large jobs.

**Solution 2 - Reduce Failure Rate:**

Check why so many failures are occurring (hardware issues, bugs, etc.).

Compilation Issues
------------------

Undefined Reference to Fenix Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: text

   undefined reference to `Fenix_Init'

**Cause:** Not linking against Fenix library.

**Solution:**

Add ``-lfenix`` to link command:

.. code-block:: bash

   mpicxx my_app.cpp -o my_app \
     -I$HOME/fenix/include \
     -L$HOME/fenix/lib -lfenix

Or with CMake:

.. code-block:: cmake

   find_package(fenix REQUIRED)
   target_link_libraries(my_app fenix)

fenix.hpp Not Found
~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: text

   fatal error: fenix.hpp: No such file or directory

**Solution:**

Add include path:

.. code-block:: bash

   mpicxx -I$HOME/fenix/include ...

Or set environment:

.. code-block:: bash

   export CPATH=$HOME/fenix/include:$CPATH

Cannot Find libfenix.a
~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: text

   ld: cannot find -lfenix

**Solution:**

Add library path:

.. code-block:: bash

   mpicxx ... -L$HOME/fenix/lib -lfenix

Or set environment:

.. code-block:: bash

   export LIBRARY_PATH=$HOME/fenix/lib:$LIBRARY_PATH
   export LD_LIBRARY_PATH=$HOME/fenix/lib:$LD_LIBRARY_PATH

Test Failures
-------------

Tests Hang
~~~~~~~~~~

**Problem:** ``ctest`` hangs indefinitely.

**Cause:** MPI not configured for fault tolerance.

**Solution:**

Tests should automatically use ``--with-ft mpi``. Verify:

.. code-block:: bash

   cd build
   ctest -V -R hello_world --timeout 20

If still hangs, test MPI directly:

.. code-block:: bash

   mpiexec --with-ft mpi --allow-run-as-root -n 2 hostname

Specific Test Fails
~~~~~~~~~~~~~~~~~~~

**Problem:** One or more tests consistently fail.

**Debug Steps:**

.. code-block:: bash

   # Run specific test with verbose output
   cd build
   ctest -R test_name -V --timeout 20

   # Run test directly
   cd test/directory
   mpiexec --with-ft mpi --allow-run-as-root -n 4 ./test_binary

   # Check test logs
   cat Testing/Temporary/LastTest.log

All Tests Timeout
~~~~~~~~~~~~~~~~~

**Problem:** All tests timeout after 20 seconds.

**Cause:** MPI not starting properly.

**Solution:**

1. **Verify MPI works:**

   .. code-block:: bash

      mpiexec --with-ft mpi -n 2 hostname

2. **Allow root** (if running as root):

   .. code-block:: bash

      mpiexec --with-ft mpi --allow-run-as-root -n 2 hostname

3. **Enable oversubscription** (if not enough cores):

   .. code-block:: bash

      mpiexec --with-ft mpi --map-by :oversubscribe -n 8 hostname

Performance Issues
------------------

Checkpointing Takes Too Long
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** ``data::commit_barrier()`` takes several seconds.

**Solutions:**

1. **Checkpoint less frequently:**

   .. code-block:: cpp

      if (iteration % 50 == 0) {  // Was: % 10
        data::checkpoint(group, SUBSET_FULL);
      }

2. **Use partial checkpoints:**

   .. code-block:: cpp

      // Only checkpoint important data
      member_store(group, critical_member, {{0, 1000}});

3. **Choose faster policy:**

   .. code-block:: cpp

      // IMR is faster than RAID for small data
      group_create(group, comm, timestamp, depth,
                   FENIX_DATA_POLICY_IMR, NULL);

High Memory Usage
~~~~~~~~~~~~~~~~~

**Problem:** Fenix uses too much memory.

**Solutions:**

1. **Reduce checkpoint history depth:**

   .. code-block:: cpp

      // Keep fewer snapshots (default is deep history)
      group_create(group, comm, timestamp,
                   1,  // depth=1 keeps only latest
                   policy, params);

2. **Reduce message log window:**

   .. code-block:: cpp

      // Keep fewer message log regions
      mlog::create(log_id, comm, 5);  // Was: 10

3. **Use data subsets:**

   Checkpoint only critical data, not everything.

Slow Recovery
~~~~~~~~~~~~~

**Problem:** Recovery takes a long time after failure.

**Solutions:**

1. **Checkpoint more frequently** (reduces replay time):

   .. code-block:: cpp

      if (iteration % 5 == 0) {  // Was: % 20
        data::checkpoint(group, SUBSET_FULL);
      }

2. **Use message logging** (avoids recomputation):

   .. code-block:: cpp

      mlog::create(log_id, comm, window_size);
      mlog::activate(log_id);

3. **Optimize checkpoint size** (faster to read/write).

Debugging Strategies
--------------------

Enable Debug Build
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cd build
   cmake ../ -DCMAKE_BUILD_TYPE=Debug
   make clean && make

This adds symbols and assertions.

Use GDB with MPI
~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Launch with xterm for each rank
   mpiexec --with-ft mpi -n 4 xterm -e gdb ./my_app

   # Or attach to running process
   gdb -p $(pgrep my_app)

Check Return Codes
~~~~~~~~~~~~~~~~~~

Always check Fenix function return codes:

.. code-block:: cpp

   int ret = member_restore(group, member);
   if (ret != FENIX_SUCCESS) {
     printf("Restore failed: %d\\n", ret);
     // Handle error
   }

Add Logging
~~~~~~~~~~~

.. code-block:: cpp

   printf("[Rank %d] Before checkpoint\\n", rank);
   data::checkpoint(group, SUBSET_FULL);
   printf("[Rank %d] After checkpoint\\n", rank);

Use Fenix Error Code
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   if (fenix::error() != FENIX_SUCCESS) {
     printf("Fenix error code: %d\\n", fenix::error());
   }

Verify MPI Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Check MPI version
   mpiexec --version

   # Check fault tolerance support
   ompi_info | grep -i fault
   ompi_info | grep -i ulfm

   # List available MPI implementations
   ldd $(which mpiexec) | grep libmpi

Getting More Help
-----------------

Still stuck? Here's what to try:

1. **Check the FAQ:** :doc:`faq`

2. **Search existing issues:** https://github.com/sandialabs/Fenix/issues

3. **Ask a question:** Open a new issue with:

   - Your Fenix version (``git rev-parse HEAD``)
   - Open MPI version (``mpiexec --version``)
   - OS and compiler versions
   - Minimal reproducible example
   - Full error message and backtrace

4. **Review documentation:**

   - :doc:`installation` - Install issues
   - :doc:`quickstart` - Getting started
   - :doc:`api/index` - API reference
   - :doc:`guides/index` - Conceptual guides

Common Error Codes
------------------

.. list-table::
   :header-rows: 1
   :widths: 15 20 65

   * - Code
     - Constant
     - Meaning
   * - 0
     - ``FENIX_SUCCESS``
     - Operation succeeded
   * - -1
     - ``FENIX_ERROR_UNINITIALIZED``
     - Fenix not initialized (call ``fenix::init()``)
   * - -2
     - ``FENIX_ERROR_INVALID_GROUPID``
     - Invalid data group ID
   * - -3
     - ``FENIX_ERROR_INVALID_MEMBERID``
     - Invalid member ID
   * - -4
     - ``FENIX_ERROR_NO_SNAPSHOT``
     - No snapshot available for restore
   * - -5
     - ``FENIX_ERROR_NO_SPARE_RANKS``
     - Out of spare ranks
   * - -10
     - ``FENIX_ERROR_MPI``
     - Underlying MPI error

Quick Reference
---------------

Diagnostic Commands
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Check Fenix installation
   ls $HOME/fenix/lib/libfenix.a
   ls $HOME/fenix/include/fenix.hpp

   # Check MPI
   mpiexec --version
   ompi_info | grep -i ulfm

   # Run simple test
   cd Fenix/build
   ctest -R 01_hello_world -V

   # Check linking
   ldd ./my_app | grep -E "(mpi|fenix)"

Environment Variables
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Add to ~/.bashrc
   export PATH=$HOME/openmpi-5.0/bin:$PATH
   export LD_LIBRARY_PATH=$HOME/openmpi-5.0/lib:$LD_LIBRARY_PATH
   export LD_LIBRARY_PATH=$HOME/fenix/lib:$LD_LIBRARY_PATH
   export CPATH=$HOME/fenix/include:$CPATH
   export LIBRARY_PATH=$HOME/fenix/lib:$LIBRARY_PATH

Minimal Test Program
~~~~~~~~~~~~~~~~~~~~

Use this to verify Fenix works:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <stdio.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank;
     MPI_Comm_rank(res_comm, &rank);
     printf("Hello from rank %d\\n", rank);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Compile and run:

.. code-block:: bash

   mpicxx -std=c++17 test.cpp -o test -lfenix
   mpiexec --with-ft mpi -n 3 ./test

Should print "Hello" from 2 ranks (3 total - 1 spare).
