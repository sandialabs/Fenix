Debugging Fenix Applications
============================

Learn how to diagnose and fix problems in Fenix applications using debugging tools, logging, and error handling patterns.

.. contents:: Quick Jump
   :local:
   :depth: 2

When to Use This Guide
----------------------

Use this guide when:

- Your Fenix application crashes or hangs
- Recovery fails or produces incorrect results
- You need to understand why a failure occurred
- You want to add debugging information to your app

Prerequisites
-------------

- Working Fenix installation with Open MPI 5+ ULFM
- Basic understanding of GDB (helpful but not required)
- Fenix application that compiles successfully

Building for Debug
------------------

Debug vs Release Builds
~~~~~~~~~~~~~~~~~~~~~~~

Always start debugging with a debug build. Debug builds include:

- Symbol information for debuggers
- Assertions that catch logic errors
- Less aggressive compiler optimizations
- Better stack traces

Create a debug build:

.. code-block:: bash

   cd /path/to/fenix/build
   cmake ../ -DCMAKE_BUILD_TYPE=Debug
   make clean && make -j4

For your application:

.. code-block:: bash

   # With CMake
   cmake ../ -DCMAKE_BUILD_TYPE=Debug

   # With manual compilation
   mpicxx -g -O0 my_app.cpp -o my_app -lfenix

The ``-g`` flag adds debug symbols, ``-O0`` disables optimizations.

.. tip::
   Keep separate build directories for debug and release:

   .. code-block:: bash

      mkdir build-debug build-release
      cd build-debug && cmake ../ -DCMAKE_BUILD_TYPE=Debug

Checking Return Codes
----------------------

Always Check Fenix Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most important debugging practice is checking return codes. Every Fenix function returns a status code.

Bad (no error checking):

.. code-block:: cpp

   #include <fenix.hpp>

   // BAD: Ignores errors
   fenix::data::member_store(group, member);
   fenix::data::commit_barrier(group);

Good (with error checking):

.. code-block:: cpp

   #include <fenix.hpp>
   #include <stdio.h>

   int ret = fenix::data::member_store(group, member);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Rank %d: Store failed with code %d\n", rank, ret);
     // Handle error appropriately
   }

   ret = fenix::data::commit_barrier(group, &timestamp);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Rank %d: Commit failed with code %d\n", rank, ret);
     // Handle error appropriately
   }

C API Error Checking
~~~~~~~~~~~~~~~~~~~~

For C applications, check the error parameter passed to ``Fenix_Init``:

.. code-block:: c

   #include <fenix.h>

   int fenix_role, error;
   MPI_Comm new_comm;

   Fenix_Init(&fenix_role, MPI_COMM_WORLD, &new_comm,
              &argc, &argv, spare_ranks, &error);

   if (error != FENIX_SUCCESS) {
     fprintf(stderr, "Fenix_Init failed: %d\n", error);
     MPI_Abort(MPI_COMM_WORLD, 1);
   }

   // Later, check other operations
   int ret = Fenix_Data_member_store(group, member, FENIX_DATA_SUBSET_FULL);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Store failed: %d\n", ret);
   }

Common Error Codes
~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Error Code
     - Likely Cause
   * - ``FENIX_SUCCESS`` (0)
     - No error
   * - ``FENIX_ERROR_UNINITIALIZED``
     - Called Fenix function before ``Fenix_Init``
   * - ``FENIX_ERROR_INVALID_GROUPID``
     - Group doesn't exist or wrong ID
   * - ``FENIX_ERROR_INVALID_MEMBERID``
     - Member doesn't exist in group
   * - ``FENIX_ERROR_NODATA_FOUND``
     - No checkpoint data available
   * - ``FENIX_WARNING_SPARE_RANKS_DEPLETED``
     - Out of spare ranks, comm was shrunk
   * - ``FENIX_WARNING_PARTIAL_RESTORE``
     - Some data couldn't be restored

See :doc:`/api/return-codes` for the complete list.

Using GDB with MPI
------------------

Running Under GDB
~~~~~~~~~~~~~~~~~

Option 1: Debug All Ranks with xterm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Launch each MPI rank in its own terminal with GDB:

.. code-block:: bash

   mpiexec --with-ft mpi -n 4 xterm -e gdb --args ./my_app

This opens 4 xterm windows, each running GDB on one rank. In each window:

.. code-block:: text

   (gdb) run
   # When it crashes:
   (gdb) backtrace
   (gdb) info locals
   (gdb) print my_variable

.. note::
   This requires X11 forwarding if running remotely. Use ``ssh -X`` or ``ssh -Y``.

Option 2: Debug a Specific Rank
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Debug only rank 0:

.. code-block:: bash

   mpiexec --with-ft mpi -n 4 bash -c '
     if [ $OMPI_COMM_WORLD_RANK -eq 0 ]; then
       xterm -e gdb --args ./my_app
     else
       ./my_app
     fi
   '

Option 3: Attach to Running Process
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Start your app normally:

.. code-block:: bash

   mpiexec --with-ft mpi -n 4 ./my_app &

Find the process to debug:

.. code-block:: bash

   ps aux | grep my_app
   # Shows PIDs for each rank

Attach GDB to one:

.. code-block:: bash

   gdb -p <PID>

Common GDB Commands
~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   # Run until crash
   (gdb) run

   # Show stack trace
   (gdb) backtrace
   (gdb) bt

   # Show variables in current scope
   (gdb) info locals

   # Print variable value
   (gdb) print my_variable
   (gdb) p rank

   # Set breakpoint
   (gdb) break Fenix_Data_member_store
   (gdb) break my_app.cpp:42

   # Continue execution
   (gdb) continue
   (gdb) c

   # Step through code
   (gdb) next      # Next line (skip functions)
   (gdb) step      # Step into function
   (gdb) finish    # Run until function returns

Debugging Specific Issues
~~~~~~~~~~~~~~~~~~~~~~~~~

Segmentation Fault
^^^^^^^^^^^^^^^^^^

When your app crashes with a segfault:

.. code-block:: bash

   mpiexec --with-ft mpi -n 4 xterm -e gdb --args ./my_app

.. code-block:: text

   (gdb) run
   # Crashes
   (gdb) backtrace
   # Shows where it crashed

Common causes:

1. Null pointer dereference
2. Invalid buffer pointer after ``vector::resize()``
3. Using freed memory
4. Stack overflow

Hang/Deadlock
^^^^^^^^^^^^^

If your app hangs, attach GDB and see where it's stuck:

.. code-block:: bash

   # In one terminal
   mpiexec --with-ft mpi -n 4 ./my_app

   # In another terminal, find PIDs
   ps aux | grep my_app

   # Attach to each rank
   gdb -p <PID_rank0>
   gdb -p <PID_rank1>
   # etc.

.. code-block:: text

   # In each GDB session
   (gdb) where
   # Shows current location

Common causes:

1. Mismatched collective calls
2. One rank died without others knowing
3. Barrier before all ranks reach it
4. Missing ``--with-ft mpi`` flag

Logging Strategies
------------------

Basic Printf Debugging
~~~~~~~~~~~~~~~~~~~~~~

Add diagnostic output at key points:

.. code-block:: cpp

   #include <stdio.h>

   int rank;
   MPI_Comm_rank(resilient_comm, &rank);

   printf("[Rank %d] Before checkpoint\n", rank);
   fflush(stdout);  // Important: flush immediately

   fenix::data::checkpoint(group, fenix::data::SUBSET_FULL);

   printf("[Rank %d] After checkpoint\n", rank);
   fflush(stdout);

.. warning::
   Always call ``fflush(stdout)`` after ``printf`` in MPI apps. Otherwise, buffered output may be lost if a rank fails.

Log Recovery Events
~~~~~~~~~~~~~~~~~~~

Log when ranks recover to understand failure patterns:

.. code-block:: cpp

   if (fenix::role() == fenix::INITIAL_RANK) {
     printf("[Rank %d] Starting fresh\n", rank);
   } else if (fenix::role() == fenix::RECOVERED_RANK) {
     printf("[Rank %d] Recovered from failure\n", rank);
   } else {
     printf("[Rank %d] Survived failure\n", rank);
   }

Log with Timestamps
~~~~~~~~~~~~~~~~~~~

Add timestamps to track timing issues:

.. code-block:: cpp

   #include <time.h>
   #include <sys/time.h>

   void log_with_time(int rank, const char* msg) {
     struct timeval tv;
     gettimeofday(&tv, nullptr);
     printf("[%ld.%06ld] Rank %d: %s\n",
            tv.tv_sec, tv.tv_usec, rank, msg);
     fflush(stdout);
   }

   // Usage
   log_with_time(rank, "Starting checkpoint");
   fenix::data::checkpoint(group, fenix::data::SUBSET_FULL);
   log_with_time(rank, "Checkpoint complete");

Conditional Logging with Debug Macro
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add debug logs that can be toggled at compile time:

.. code-block:: cpp

   // At top of file
   #ifdef DEBUG
   #define DEBUG_LOG(rank, msg) \
     printf("[DEBUG] Rank %d: %s\n", rank, msg); fflush(stdout);
   #else
   #define DEBUG_LOG(rank, msg) // No-op
   #endif

   // Usage
   DEBUG_LOG(rank, "About to store member");
   fenix::data::member_store(group, member);

Compile with:

.. code-block:: bash

   mpicxx -DDEBUG my_app.cpp -o my_app -lfenix

Common Debugging Patterns
--------------------------

Verify Initialization
~~~~~~~~~~~~~~~~~~~~~

Check that Fenix initialized correctly:

.. code-block:: cpp

   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 2});

   if (fenix::error() != FENIX_SUCCESS) {
     fprintf(stderr, "Fenix init failed: %d\n", fenix::error());
     MPI_Abort(MPI_COMM_WORLD, 1);
   }

   int n_ranks;
   MPI_Comm_size(res_comm, &n_ranks);
   printf("Resilient comm has %d ranks\n", n_ranks);

Verify Data Group Creation
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Check that data groups were created:

.. code-block:: cpp

   int ret = fenix::data::group_create(group_id);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Rank %d: Group create failed: %d\n", rank, ret);
   }

   if (!fenix::data::group_created(group_id)) {
     fprintf(stderr, "Rank %d: Group %d not created!\n", rank, group_id);
   }

Verify Member Creation
~~~~~~~~~~~~~~~~~~~~~~

Check that data members exist:

.. code-block:: cpp

   int ret = fenix::data::member_create(group, member, &data, count, MPI_DOUBLE);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Rank %d: Member create failed: %d\n", rank, ret);
   }

   if (!fenix::data::member_created(group, member)) {
     fprintf(stderr, "Rank %d: Member %d not created!\n", rank, member);
   }

Check Available Snapshots
~~~~~~~~~~~~~~~~~~~~~~~~~

Verify checkpoints were created:

.. code-block:: cpp

   int num_snapshots = 0;
   Fenix_Data_group_get_number_of_snapshots(group, &num_snapshots);
   printf("Rank %d: Group %d has %d snapshots\n", rank, group, num_snapshots);

   if (num_snapshots == 0) {
     fprintf(stderr, "Rank %d: WARNING - No snapshots available!\n", rank);
   }

Validate Restored Data
~~~~~~~~~~~~~~~~~~~~~~

After restore, verify data makes sense:

.. code-block:: cpp

   fenix::DataSubset found;
   int ret = fenix::data::member_restore(group, member, nullptr, 0,
                                         FENIX_DATA_SNAPSHOT_LATEST, &found);

   if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
     fprintf(stderr, "Rank %d: Only partial restore\n", rank);
   }

   // Validate data
   bool valid = true;
   for (int i = 0; i < count; i++) {
     if (data[i] < 0 || data[i] > MAX_EXPECTED_VALUE) {
       fprintf(stderr, "Rank %d: Invalid data[%d] = %f\n", rank, i, data[i]);
       valid = false;
     }
   }

   if (!valid) {
     fprintf(stderr, "Rank %d: Data validation FAILED\n", rank);
   }

Troubleshooting Recovery Failures
----------------------------------

Buffer Pointer Issues
~~~~~~~~~~~~~~~~~~~~~

Problem: After ``std::vector::resize()``, restored data is garbage.

Solution: Update the buffer pointer after resize:

.. code-block:: cpp

   std::vector<double> data(initial_size);

   // Create member
   fenix::data::member_create(group, member, data.data(),
                              data.size(), MPI_DOUBLE);

   // ... later, after recovery ...
   data.resize(new_size);

   // CRITICAL: Update buffer pointer
   int flag;
   Fenix_Data_member_attr_set(
     group, member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     data.data(), &flag
   );

Missing Data Group on Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Problem: Recovered rank tries to restore but group doesn't exist.

Solution: Recreate group before restoring:

.. code-block:: cpp

   if (fenix::role() != fenix::INITIAL_RANK) {
     // Recovered or survivor rank
     if (!fenix::data::group_created(group)) {
       fenix::data::group_create(group);
     }

     fenix::data::member_restore(group, member);
   }

Partial Restore Warning
~~~~~~~~~~~~~~~~~~~~~~~

Problem: ``FENIX_WARNING_PARTIAL_RESTORE`` returned from restore.

Debug steps:

.. code-block:: cpp

   fenix::DataSubset found;
   int ret = fenix::data::member_restore(group, member, nullptr, 0,
                                         FENIX_DATA_SNAPSHOT_ALL, &found);

   if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
     // Check what was actually found
     printf("Rank %d: Partial restore - some data missing\n", rank);

     // Check subset details if needed
     // found will contain what was actually restored
   }

Common causes:

1. Never committed data before failure
2. Member was created but never stored
3. Catastrophic failure (too many ranks failed)

Testing Locally
---------------

Inject Failures for Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test recovery without manually killing processes:

.. code-block:: cpp

   #include <signal.h>
   #include <unistd.h>

   void maybe_inject_failure(int rank, int iteration) {
     // Fail rank 2 on iteration 10
     if (rank == 2 && iteration == 10) {
       printf("Rank %d: Injecting failure at iteration %d\n", rank, iteration);
       fflush(stdout);
       raise(SIGKILL);
     }
   }

   // In main loop
   for (int i = 0; i < num_iterations; i++) {
     maybe_inject_failure(rank, i);
     // ... work ...
   }

Use Environment Variables
~~~~~~~~~~~~~~~~~~~~~~~~~

Make failure injection configurable:

.. code-block:: cpp

   #include <cstdlib>

   bool should_inject_failure(int rank, int iteration) {
     const char* fail_rank = std::getenv("FENIX_TEST_FAIL_RANK");
     const char* fail_iter = std::getenv("FENIX_TEST_FAIL_ITER");

     if (fail_rank && fail_iter) {
       return (rank == atoi(fail_rank) && iteration == atoi(fail_iter));
     }
     return false;
   }

Run with:

.. code-block:: bash

   FENIX_TEST_FAIL_RANK=1 FENIX_TEST_FAIL_ITER=5 \
     mpiexec --with-ft mpi -n 4 ./my_app

Verify Recovery Worked
~~~~~~~~~~~~~~~~~~~~~~

After recovery, check that state is correct:

.. code-block:: cpp

   bool verify_state(int rank, int expected_iteration) {
     if (iteration != expected_iteration) {
       fprintf(stderr, "Rank %d: Expected iteration %d, got %d\n",
               rank, expected_iteration, iteration);
       return false;
     }

     // Check data validity
     for (int i = 0; i < data_size; i++) {
       if (data[i] != expected_value(i, iteration)) {
         fprintf(stderr, "Rank %d: Data[%d] incorrect\n", rank, i);
         return false;
       }
     }

     return true;
   }

Performance Debugging
---------------------

Time Checkpoint Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~

Measure how long checkpoints take:

.. code-block:: cpp

   #include <chrono>

   auto start = std::chrono::high_resolution_clock::now();

   fenix::data::checkpoint(group, fenix::data::SUBSET_FULL);

   auto end = std::chrono::high_resolution_clock::now();
   auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
     end - start
   ).count();

   printf("Rank %d: Checkpoint took %ld ms\n", rank, duration);

If too slow, see :doc:`optimize-checkpoints`.

Profile with Timing Sections
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto stage_start = std::chrono::high_resolution_clock::now();
   fenix::data::member_stage(group, member);
   auto stage_end = std::chrono::high_resolution_clock::now();

   auto store_start = std::chrono::high_resolution_clock::now();
   fenix::data::member_store(group, member);
   auto store_end = std::chrono::high_resolution_clock::now();

   auto commit_start = std::chrono::high_resolution_clock::now();
   fenix::data::commit_barrier(group);
   auto commit_end = std::chrono::high_resolution_clock::now();

   printf("Rank %d: stage=%ldms store=%ldms commit=%ldms\n", rank,
          duration_ms(stage_start, stage_end),
          duration_ms(store_start, store_end),
          duration_ms(commit_start, commit_end));

Next Steps
----------

- :doc:`/troubleshooting` - Solutions to common problems
- :doc:`test-locally` - Testing fault tolerance without real failures
- :doc:`optimize-checkpoints` - Improve checkpoint performance
- :doc:`/api/return-codes` - Complete list of error codes

See Also
--------

- GDB Documentation: https://sourceware.org/gdb/documentation/
- Open MPI Debugging: https://www.open-mpi.org/faq/?category=debugging
- :doc:`/guides/data-recovery` - Understanding data recovery concepts
