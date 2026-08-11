Cheat Sheet
===========

Printable one-page reference for Fenix development. For complete details, see :doc:`api-quick-ref` and :doc:`api/index`.

Quick Start
-----------

**Minimal Fenix Program:**

.. code-block:: cpp

   #include <fenix.hpp>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize with 3 spare ranks
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 3});

     // Your MPI code here (use res_comm)

     Fenix_Finalize();
     MPI_Finalize();
   }

**Compile & Run:**

.. code-block:: bash

   mpicxx -std=c++20 myapp.cpp -lfenix -o myapp
   mpiexec --with-ft mpi -n 10 ./myapp  # 7 active + 3 spares

Essential Code Snippets
-----------------------

**1. Basic Initialization (C++)**

.. code-block:: cpp

   MPI_Comm res_comm;
   fenix::init({
     .out_comm = &res_comm,
     .spares = 3
   });

**2. Register Recovery Callback**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::role() == fenix::RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, MEMBER_ID);
     }
   });

**3. Setup Data Recovery**

.. code-block:: cpp

   using namespace fenix::data;

   const int GROUP_ID = 1, MEMBER_ID = 1;

   group_create(GROUP_ID, {.comm = res_comm});
   member_create(GROUP_ID, MEMBER_ID,
                 data.data(), data.size(), MPI_DOUBLE);

**4. Checkpoint Data**

.. code-block:: cpp

   // Simple: checkpoint all members
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

   // Or: store and commit separately
   fenix::data::member_store(GROUP_ID, MEMBER_ID);
   int time_stamp;
   fenix::data::commit_barrier(GROUP_ID, &time_stamp);

**5. Restore Data After Recovery**

.. code-block:: cpp

   // Repair and load
   fenix::data::member_repair(GROUP_ID, MEMBER_ID);
   fenix::data::member_load(GROUP_ID, MEMBER_ID);

   // Or: combined restore
   fenix::data::member_restore(GROUP_ID, MEMBER_ID);

**6. Inline Recovery with Exceptions**

.. code-block:: cpp

   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   try {
     // MPI operations
   } catch (fenix::CommException& e) {
     // Handle recovery
     if (fenix::role() == fenix::RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, MEMBER_ID);
     }
   }

Recovery Modes
--------------

.. code-block:: cpp

   // Set before or after Fenix_Init:

   // Inline with exceptions (C++ recommended)
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   // Inline with return codes
   fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);

   // Longjmp back to Fenix_Init (default)
   fenix::set_option(fenix::RESUME_MODE, fenix::JUMP);

Critical mpiexec Flags
----------------------

.. code-block:: bash

   # REQUIRED: Enable ULFM fault tolerance
   --with-ft mpi

   # Recommended: Allow oversubscription (for testing)
   --map-by :oversubscribe

   # Recommended: Async finalize (cleaner shutdown)
   --mca async_mpi_finalize 1

   # Full command:
   mpiexec --with-ft mpi \
           --map-by :oversubscribe \
           --mca async_mpi_finalize 1 \
           -n 10 ./myapp

Rank Roles
----------

.. code-block:: cpp

   fenix::INITIAL_RANK    // No failures yet
   fenix::RECOVERED_RANK  // Was spare, now active
   fenix::SURVIVOR_RANK   // Survived a failure
   fenix::SPARE_RANK      // Currently a spare

   // Check role:
   if (fenix::role() == fenix::RECOVERED_RANK) {
     // Restore state
   }

Common Error Codes
------------------

.. code-block:: cpp

   FENIX_SUCCESS                       // 0
   FENIX_ERROR_UNINITIALIZED           // Not initialized
   FENIX_ERROR_INVALID_GROUPID         // Group doesn't exist
   FENIX_ERROR_INVALID_MEMBERID        // Member doesn't exist
   FENIX_ERROR_NODATA_FOUND            // No checkpoint available
   FENIX_WARNING_SPARE_RANKS_DEPLETED  // Out of spares

   // Check last error:
   int err = fenix::error();

Key Constants
-------------

.. code-block:: cpp

   // Data subsets (specify which element ranges to checkpoint)
   fenix::data::SUBSET_FULL        // All elements
   fenix::data::SUBSET_EMPTY       // No elements (placeholder)
   fenix::data::SUBSET_PRESTAGED   // Elements from member_stage()

   // Snapshots
   FENIX_DATA_SNAPSHOT_LATEST      // -1
   FENIX_DATA_SNAPSHOT_ALL         // -2

   // Member sizes
   FENIX_RESIZEABLE                // Variable size

   // Policies
   FENIX_DATA_POLICY_IMR           // In-Memory RAID

Troubleshooting Quick Checks
-----------------------------

**Problem: Hangs at MPI_Init or Fenix_Init**

.. code-block:: bash

   # Check: ULFM flag present?
   mpiexec --with-ft mpi -n 2 hostname

   # Check: MPI built with ULFM?
   ompi_info | grep -i fault

**Problem: Segfault in MPI calls**

.. code-block:: bash

   # Check: Multiple MPI versions?
   ldd ./myapp | grep libmpi

   # Fix: Enable system include fix
   cmake -DFENIX_SYSTEM_INC_FIX=ON

**Problem: Tests timeout**

.. code-block:: bash

   # Reduce timeout for testing
   ctest -V --timeout 20

   # Check specific test
   ctest -R test_name -V

**Problem: Recovered data is wrong**

.. code-block:: cpp

   // After resizing:
   data.resize(new_size);

   // Update buffer pointer
   int flag;
   Fenix_Data_member_attr_set(
     GROUP_ID, MEMBER_ID,
     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     data.data(), &flag
   );

**Problem: Recovery fails silently**

.. code-block:: cpp

   // Check error in callback
   fenix::callback_register([&](MPI_Comm comm, int err) {
     printf("Recovery error: %d\n", fenix::error());
     if (fenix::error() != FENIX_SUCCESS) {
       return;  // Don't proceed with bad recovery
     }
     // Recovery logic...
   });

**Problem: Out of spares**

.. code-block:: cpp

   // Check spare count
   printf("Remaining spares: %d\n", fenix::nspare());

   // Check for warning
   if (fenix::error() == FENIX_WARNING_SPARE_RANKS_DEPLETED) {
     printf("WARNING: Spares depleted!\n");
   }

Where to Look for What
----------------------

**I want to...**

- **Get started quickly** → :doc:`quickstart`
- **Learn Fenix concepts** → :doc:`tutorials/index`
- **Solve a specific problem** → :doc:`howto/index`
- **Look up a function** → :doc:`api/index`
- **Understand how Fenix works** → :doc:`guides/index`
- **Fix a problem** → :doc:`troubleshooting`
- **See working code** → :doc:`examples/index`
- **Look up a term** → :doc:`glossary`

**Common How-To Guides:**

- :doc:`howto/checkpoint-data` - How to checkpoint data
- :doc:`howto/choose-recovery-pattern` - longjmp vs inline
- :doc:`howto/partial-checkpoints` - Reduce checkpoint overhead
- :doc:`howto/message-logging` - Setup message logging
- :doc:`howto/debug-fenix-app` - Debug Fenix applications
- :doc:`howto/migrate-existing-app` - Convert existing MPI app

**Common Guides:**

- :doc:`guides/process-recovery` - How process recovery works
- :doc:`guides/data-recovery` - How data recovery works
- :doc:`guides/imr-policy` - In-Memory RAID policy details
- :doc:`guides/architecture` - Fenix architecture overview

Complete Examples
-----------------

**Example 1: Basic Recovery Loop**

.. code-block:: cpp

   #include <fenix.hpp>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     for (int iter = 0; iter < 100; iter++) {
       if (rank == 0) {
         printf("Iteration %d\n", iter);
       }

       // Your computation and communication here
       double result;
       MPI_Allreduce(&local_value, &result, 1, MPI_DOUBLE,
                     MPI_SUM, res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
   }

**Example 2: Recovery with Checkpointing**

.. code-block:: cpp

   #include <fenix.hpp>
   #include <vector>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 3});

     std::vector<double> data(1000);
     const int GROUP_ID = 1, MEMBER_ID = 1;

     // Setup data recovery
     fenix::data::group_create(GROUP_ID, {.comm = res_comm});
     fenix::data::member_create(GROUP_ID, MEMBER_ID,
                                data.data(), data.size(), MPI_DOUBLE);

     // Register callback
     fenix::callback_register([&](MPI_Comm comm, int err) {
       if (fenix::role() == fenix::RECOVERED_RANK) {
         fenix::data::member_restore(GROUP_ID, MEMBER_ID);
       }
     });

     // Main loop
     for (int iter = 0; iter < 1000; iter++) {
       // Computation
       compute(data);

       // Checkpoint every 10 iterations
       if (iter % 10 == 0) {
         fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
   }

**Example 3: Inline Recovery with Exceptions**

.. code-block:: cpp

   #include <fenix.hpp>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

     try {
       for (int iter = 0; iter < 1000; iter++) {
         // MPI operations that may fail
         MPI_Barrier(res_comm);
       }
     } catch (fenix::CommException& e) {
       printf("Recovered from failure\n");

       // Restore state if needed
       if (fenix::role() == fenix::RECOVERED_RANK) {
         // Restore application state
       }

       // Could restart loop or continue as appropriate
     }

     Fenix_Finalize();
     MPI_Finalize();
   }

Performance Tips
----------------

**Reduce Checkpoint Overhead:**

1. Checkpoint less frequently
2. Use partial checkpoints (subsets)
3. Use ``member_stage()`` to decouple serialization from network
4. Reduce checkpoint depth (keep fewer snapshots)
5. Choose appropriate IMR policy mode

**Reduce Recovery Time:**

1. Checkpoint more frequently (less to recompute)
2. Enable message logging (replay instead of recompute)
3. Use faster network interconnect
4. Optimize data member layout

**General:**

1. Build with ``-DCMAKE_BUILD_TYPE=Release``
2. Profile to identify bottlenecks
3. Tune spare count based on failure rate
4. Monitor memory usage (checkpoints stored in RAM)

Quick Reference Card
--------------------

**Include:**

.. code-block:: cpp

   #include <fenix.hpp>     // C++ API
   #include <fenix.h>       // C API

**Namespace:**

.. code-block:: cpp

   using namespace fenix;        // Process recovery
   using namespace fenix::data;  // Data recovery
   namespace mlog = fenix::mlog; // Message logging

**Most Used Functions:**

.. code-block:: cpp

   // Initialization
   fenix::init({.out_comm = &comm, .spares = N});
   Fenix_Finalize();

   // Configuration
   fenix::set_option(setting, option);
   fenix::role();
   fenix::error();

   // Callbacks
   fenix::callback_register(callback_fn);

   // Data recovery
   data::group_create(group_id, {.comm = comm});
   data::member_create(group_id, member_id, buf, count, type);
   data::checkpoint(group_id, SUBSET_FULL);
   data::member_restore(group_id, member_id);

**Must Remember:**

1. Always use ``--with-ft mpi`` with mpiexec
2. Use resilient communicator, not MPI_COMM_WORLD
3. Update buffer pointers after resizing data
4. Check ``fenix::error()`` in callbacks
5. Recovered ranks have no registered callbacks

Testing Checklist
-----------------

Before deploying:

- [ ] Compiles without warnings
- [ ] Runs with ``--with-ft mpi`` flag
- [ ] Survives single rank failure
- [ ] Survives multiple simultaneous failures
- [ ] Recovers correctly with checkpointing enabled
- [ ] Data validation passes after recovery
- [ ] Performance overhead is acceptable
- [ ] Spare count is sufficient for expected failure rate
- [ ] Recovery callbacks tested and working
- [ ] Tested with depleted spares scenario

Getting Help
------------

**Documentation:** https://fenix.readthedocs.io

**GitHub Issues:** https://github.com/sandialabs/Fenix/issues

**Include in bug reports:**

- Fenix version (``git rev-parse HEAD``)
- Open MPI version (``mpiexec --version``)
- Minimal reproducible example
- Full error output

**Before asking:**

1. Check :doc:`troubleshooting`
2. Check :doc:`faq`
3. Search GitHub issues
4. Review :doc:`examples/index`

See Also
--------

- :doc:`api-quick-ref` - Extended API reference
- :doc:`glossary` - Term definitions
- :doc:`best-practices` - Production deployment checklist
- :doc:`common-mistakes` - Avoid common pitfalls
