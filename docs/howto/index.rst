How-To Guides
=============

Task-focused guides for solving specific problems with Fenix. Each guide shows you how to accomplish a particular goal.

.. tip::
   Looking for learning materials? See :doc:`/tutorials/index` for step-by-step lessons.

   Want to understand concepts? See :doc:`/guides/index` for explanations.

Getting Started Guides
----------------------

.. toctree::
   :maxdepth: 1

   choose-recovery-pattern
   migrate-existing-app
   test-locally

Data Recovery Guides
--------------------

.. toctree::
   :maxdepth: 1

   checkpoint-data
   partial-checkpoints
   custom-recovery
   optimize-checkpoints

Advanced Topics
---------------

.. toctree::
   :maxdepth: 1

   message-logging
   inline-recovery-callbacks
   handle-cascading-failures
   performance-tuning

Build and Configuration
-----------------------

.. toctree::
   :maxdepth: 1

   configure-cmake
   integrate-cmake-project
   debug-fenix-app

Quick Reference
---------------

Choose a Recovery Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Which recovery pattern should I use?

**Quick Answer:**

- **Inline + Callbacks (Recommended):** Modern C++ applications, cleaner control flow
- **Longjmp:** Legacy C code, simple restart patterns
- **No Recovery:** Just use communicator repair, handle recovery yourself

:doc:`Full guide → <choose-recovery-pattern>`

Migrate an Existing MPI App
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** I have an MPI application and want to add fault tolerance.

**Quick Answer:**

1. Change ``MPI_Init`` to also call ``fenix::init()``
2. Use Fenix communicator instead of ``MPI_COMM_WORLD``
3. Add data checkpointing for stateful data
4. Optionally add message logging

:doc:`Full guide → <migrate-existing-app>`

Test Fault Tolerance Locally
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** How do I test my fault-tolerant app without killing processes?

**Quick Answer:**

Use Fenix's test utilities or inject failures programmatically:

.. code-block:: cpp

   if (should_fail) {
     raise(SIGKILL);  // Simulate failure
   }

:doc:`Full guide → <test-locally>`

Debug a Fenix Application
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** My Fenix app crashes or hangs. How do I debug it?

**Quick Answer:**

1. Build with ``-DCMAKE_BUILD_TYPE=Debug``
2. Run under gdb with ``mpiexec``
3. Check return codes and ``fenix::error()``
4. Enable verbose logging

:doc:`Full guide → <debug-fenix-app>`

Checkpoint Application Data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** How do I checkpoint my application state?

**Quick Answer:**

.. code-block:: cpp

   using namespace fenix::data;

   // Create group and members
   group_create(GROUP_ID);
   member_create(GROUP_ID, MEMBER_ID, &my_data, size, MPI_DOUBLE);

   // Store data
   member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
   commit_barrier(GROUP_ID);

   // Restore after recovery
   member_restore(GROUP_ID, MEMBER_ID);

:doc:`Full guide → <checkpoint-data>`

Checkpoint Only Part of Data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** My array is huge. Can I checkpoint only part of it?

**Quick Answer:**

Yes! Use data subsets. A **subset** specifies which element ranges to checkpoint instead of checkpointing the entire array:

.. code-block:: cpp

   // Checkpoint only elements 0-99 and 500-599 (skip 100-499, 600+)
   Fenix_Data_subset subset;
   Fenix_Data_subset_createv(2,
     (int[]){0, 500},      // Start indices
     (int[]){99, 599},     // End indices (inclusive)
     &subset);

   member_store(GROUP_ID, MEMBER_ID, subset);

:doc:`Full guide → <partial-checkpoints>`

Use Message Logging
~~~~~~~~~~~~~~~~~~~

**Problem:** How do I enable automatic message replay?

**Quick Answer:**

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   // Create and activate message log
   mlog::create(LOG_ID, comm, num_regions);
   mlog::activate(LOG_ID);

   // Use regions for checkpointing
   mlog::begin_region(LOG_ID, iteration);
   // ... MPI communication ...

:doc:`Full guide → <message-logging>`

Set Up Inline Recovery Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** How do I recover inline without longjmp?

**Quick Answer:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     // Restore application state
     data::group_create(GROUP_ID);
     data::member_restore(GROUP_ID, STATE_MEMBER, NULL, 0);

     // Continue from here - no longjmp!
     printf("Recovered, continuing inline\\n");
   });

:doc:`Full guide → <inline-recovery-callbacks>`

Handle Cascading Failures
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** What if failures happen during recovery?

**Quick Answer:**

Wrap recovery in retry loop with exception handling:

.. code-block:: cpp

   while (true) {
     try {
       // Attempt recovery
       data::member_restore(GROUP_ID, MEMBER_ID);
       break;  // Success
     } catch (fenix::CommException& e) {
       // Another failure during recovery - retry
       continue;
     }
   }

:doc:`Full guide → <handle-cascading-failures>`

Optimize Performance
~~~~~~~~~~~~~~~~~~~~

**Problem:** Fenix is too slow. How do I optimize?

**Quick Answer:**

1. Reduce checkpoint frequency
2. Use partial checkpoints (subsets)
3. Choose efficient redundancy policy (IMR vs RAID)
4. Tune message log window size
5. Profile with minimal overhead build

:doc:`Full guide → <performance-tuning>`

Integration Guides
------------------

Use Fenix with CMake
~~~~~~~~~~~~~~~~~~~~

Add to ``CMakeLists.txt``:

.. code-block:: cmake

   find_package(fenix REQUIRED)
   target_link_libraries(my_app fenix)

Set ``CMAKE_PREFIX_PATH`` to your Fenix installation.

:doc:`Full guide → <integrate-cmake-project>`

Configure Build Options
~~~~~~~~~~~~~~~~~~~~~~~~

Control Fenix features at build time:

.. code-block:: bash

   cmake ../ \
     -DFENIX_SYSTEM_INC_FIX=ON \
     -DFENIX_CPP_CATCH_RUNTIME_EXCEPTIONS=ON \
     -DCMAKE_BUILD_TYPE=Release

:doc:`Full guide → <configure-cmake>`

Can't Find What You Need?
--------------------------

- 📚 **Learning:** Try :doc:`/tutorials/index` for guided lessons
- 💡 **Understanding:** See :doc:`/guides/index` for concept explanations
- 📖 **Reference:** Check :doc:`/api/index` for complete API docs
- 🐛 **Problems:** Visit :doc:`/troubleshooting` for common issues
- ❓ **Questions:** See :doc:`/faq` for frequently asked questions
