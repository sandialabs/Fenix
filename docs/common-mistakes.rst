Common Mistakes
===============

Learn from others' mistakes. This guide documents common pitfalls when using Fenix and how to avoid them.

.. contents:: Categories
   :local:
   :depth: 2

Initialization & Configuration
-------------------------------

Mistake: Forgetting ``--with-ft mpi`` Flag
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: bash

   # WRONG - No fault tolerance
   mpiexec -n 10 ./myapp

   # Result: Hangs, crashes, or runs without fault tolerance

**Correct:**

.. code-block:: bash

   # RIGHT - ULFM fault tolerance enabled
   mpiexec --with-ft mpi -n 10 ./myapp

**Why it matters:** Without ``--with-ft mpi``, MPI doesn't enable ULFM fault tolerance. Fenix cannot function correctly without ULFM.

**How to verify:**

.. code-block:: bash

   # Test if ULFM is working
   mpiexec --with-ft mpi -n 2 hostname

   # Should see output from both ranks
   # If it hangs, your MPI isn't configured for ULFM

Mistake: Using ``MPI_COMM_WORLD`` Instead of Resilient Communicator
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 3});

   // Using MPI_COMM_WORLD instead of res_comm
   MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

**Correct:**

.. code-block:: cpp

   // RIGHT
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 3});

   // Use resilient communicator
   MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, res_comm);

**Why it matters:** Fenix can only detect and recover from failures on its resilient communicator. Operations on ``MPI_COMM_WORLD`` will abort the entire application on failure.

**When to use** ``MPI_COMM_WORLD``: Only before ``Fenix_Init`` or for operations that don't need fault tolerance (e.g., one-time setup).

Mistake: Calling Fenix Functions Before Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Setting option before MPI_Init
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);
   MPI_Init(&argc, &argv);

**Correct:**

.. code-block:: cpp

   // RIGHT - MPI_Init first, then Fenix options
   MPI_Init(&argc, &argv);
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);
   fenix::init({.out_comm = &res_comm, .spares = 3});

**Exception:** ``fenix::set_option`` and ``fenix::initialized`` can be called before ``Fenix_Init``, but after ``MPI_Init``.

**Why it matters:** Fenix depends on MPI being initialized. Other Fenix functions require ``Fenix_Init`` to be called first.

Mistake: Not Checking ``fenix::initialized()``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Assuming Fenix is initialized
   fenix::data::group_create(GROUP_ID, {.comm = res_comm});

   // Result: FENIX_ERROR_UNINITIALIZED

**Correct:**

.. code-block:: cpp

   // RIGHT - Check initialization
   if (!fenix::initialized()) {
     fprintf(stderr, "Error: Fenix not initialized\n");
     return 1;
   }

   fenix::data::group_create(GROUP_ID, {.comm = res_comm});

**Why it matters:** Calling Fenix functions before initialization returns error codes that are easy to miss.

----

Process Recovery
----------------

Mistake: Not Registering Recovery Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - No callback registered
   fenix::init({.out_comm = &res_comm, .spares = 3});

   // After failure, recovered ranks have no state!

**Correct:**

.. code-block:: cpp

   // RIGHT - Register callback for recovery
   fenix::init({.out_comm = &res_comm, .spares = 3});

   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::role() == fenix::RECOVERED_RANK) {
       // Restore state
       fenix::data::member_restore(GROUP_ID, MEMBER_ID);
     }
   });

**Why it matters:** Recovered ranks (former spares) have no application state. Without callbacks to restore state, they have uninitialized data.

**Note:** Callbacks are only invoked on survivor ranks, but survivor ranks can restore state for recovered ranks using collective operations.

Mistake: Assuming Rank IDs Stay the Same
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Assuming rank IDs never change
   int my_rank;
   MPI_Comm_rank(res_comm, &my_rank);

   // Store rank-specific data using rank ID as key
   rank_data[my_rank] = my_data;

   // After spare depletion, rank IDs may change!

**Correct:**

.. code-block:: cpp

   // RIGHT - Update rank ID after recovery
   int my_rank;
   MPI_Comm_rank(res_comm, &my_rank);

   fenix::callback_register([&](MPI_Comm comm, int err) {
     // Update rank ID after recovery
     MPI_Comm_rank(comm, &my_rank);

     // Check if communicator shrank
     if (fenix::error() == FENIX_WARNING_SPARE_RANKS_DEPLETED) {
       int new_size;
       MPI_Comm_size(comm, &new_size);
       // Adjust algorithm for new size
     }
   });

**Why it matters:** When spares are depleted, Fenix shrinks the communicator. Ranks may get new rank IDs.

**Best practice:** Always query rank ID after recovery. Don't cache rank IDs across failures.

Mistake: Not Checking for Failures in Long Compute Loops
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Long compute loop with no MPI calls
   for (int iter = 0; iter < 10000; iter++) {
     expensive_computation();  // 1 second per iteration
     // No MPI calls for ~3 hours!
   }

   MPI_Barrier(res_comm);  // Failure detected only here

**Correct:**

.. code-block:: cpp

   // RIGHT - Periodically check for failures
   for (int iter = 0; iter < 10000; iter++) {
     expensive_computation();

     // Check every 100 iterations (~100 seconds)
     if (iter % 100 == 0) {
       fenix::detect_failures(true);
     }
   }

   MPI_Barrier(res_comm);

**Why it matters:** Failures can only be detected during MPI calls. Long compute phases delay recovery, potentially allowing more failures to accumulate.

Mistake: Using Longjmp with C++ Objects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Longjmp with C++ objects
   fenix::set_option(fenix::RESUME_MODE, fenix::JUMP);  // Default

   fenix::init({.out_comm = &res_comm, .spares = 2});

   {
     std::vector<double> data(1000);  // Will leak on longjmp!
     std::unique_ptr<State> state = std::make_unique<State>();

     // If failure occurs here, longjmp skips destructors
     MPI_Barrier(res_comm);

     // Destructors called here if no failure
   }

**Correct:**

.. code-block:: cpp

   // RIGHT - Use inline recovery with exceptions
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   fenix::init({.out_comm = &res_comm, .spares = 2});

   try {
     std::vector<double> data(1000);
     std::unique_ptr<State> state = std::make_unique<State>();

     MPI_Barrier(res_comm);

     // Destructors called normally
   } catch (fenix::CommException& e) {
     // Handle recovery
   }

**Why it matters:** ``longjmp`` bypasses normal C++ stack unwinding. Destructors are not called, leading to resource leaks and undefined behavior.

**Exception:** If using longjmp, allocate resources before ``Fenix_Init`` or use manual memory management.

Mistake: Ignoring ``fenix::error()`` in Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Not checking error
   fenix::callback_register([&](MPI_Comm comm, int err) {
     // Assume restore always succeeds
     fenix::data::member_restore(GROUP_ID, MEMBER_ID);

     // If restore failed, continue with garbage data!
   });

**Correct:**

.. code-block:: cpp

   // RIGHT - Check error code
   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::error() != FENIX_SUCCESS) {
       fprintf(stderr, "Recovery error: %d\n", fenix::error());
       return;  // Don't proceed with failed recovery
     }

     int ret = fenix::data::member_restore(GROUP_ID, MEMBER_ID);
     if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Restore error: %d\n", ret);
       MPI_Abort(comm, ret);
     }
   });

**Why it matters:** Recovery operations can fail (e.g., no checkpoint available, corrupted data). Proceeding with invalid state leads to incorrect results or crashes.

----

Data Recovery
-------------

Mistake: Not Updating Buffer Pointers After Resize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Resize without updating Fenix
   std::vector<double> data(1000);
   fenix::data::member_create(GROUP_ID, MEMBER_ID,
                              data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // Checkpoint
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

   // Resize
   data.resize(2000);  // Pointer may change!

   // Next checkpoint uses old pointer - WRONG!
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

**Correct:**

.. code-block:: cpp

   // RIGHT - Update buffer pointer after resize
   std::vector<double> data(1000);
   fenix::data::member_create(GROUP_ID, MEMBER_ID,
                              data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

   // Resize
   data.resize(2000);

   // Update Fenix with new pointer
   int flag;
   Fenix_Data_member_attr_set(
     GROUP_ID, MEMBER_ID,
     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     data.data(),  // New pointer
     &flag
   );

   // Now safe to checkpoint again
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

**Why it matters:** Vector resizing may reallocate memory, invalidating the old pointer. Fenix will checkpoint from the wrong location.

**Best practice:** Always update buffer pointer after any operation that might reallocate (resize, assign, swap, etc.).

Mistake: Checkpointing Too Frequently
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Checkpoint every iteration
   for (int iter = 0; iter < 10000; iter++) {
     compute();  // 0.1 seconds

     // Checkpoint every iteration (also 0.1 seconds)
     fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

     // 50% overhead!
   }

**Correct:**

.. code-block:: cpp

   // RIGHT - Checkpoint every N iterations
   for (int iter = 0; iter < 10000; iter++) {
     compute();

     // Checkpoint every 100 iterations (~10 seconds)
     if (iter % 100 == 0) {
       fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
     }
     // ~1% overhead
   }

**Why it matters:** Checkpointing has overhead (CPU, memory, network). Too frequent checkpointing wastes resources.

**Rule of thumb:** Checkpoint overhead should be < 5-10% of execution time. Measure and adjust.

Mistake: Checkpointing Everything
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Checkpoint unnecessary data
   std::vector<double> state;           // NEED to checkpoint
   std::vector<double> temp_buffer;     // DON'T need to checkpoint
   std::vector<double> workspace;       // DON'T need to checkpoint
   const std::vector<double> constants; // DON'T need to checkpoint

   // Checkpoint all of them
   member_create(GROUP_ID, STATE_ID, state.data(), ...);
   member_create(GROUP_ID, TEMP_ID, temp_buffer.data(), ...);
   member_create(GROUP_ID, WORK_ID, workspace.data(), ...);
   member_create(GROUP_ID, CONST_ID, constants.data(), ...);

   checkpoint(GROUP_ID, SUBSET_FULL);  // Huge, slow checkpoint

**Correct:**

.. code-block:: cpp

   // RIGHT - Checkpoint only critical state
   std::vector<double> state;           // Checkpoint this
   std::vector<double> temp_buffer;     // Re-allocate after recovery
   std::vector<double> workspace;       // Re-allocate after recovery
   const std::vector<double> constants; // Re-read from file

   // Only checkpoint state
   member_create(GROUP_ID, STATE_ID, state.data(), ...);

   checkpoint(GROUP_ID, SUBSET_FULL);  // Fast, small checkpoint

**Why it matters:** Larger checkpoints = more memory, more network traffic, slower checkpointing.

**Rule:** Only checkpoint data that is (1) expensive to recompute and (2) changes over time.

Mistake: Not Checking ``FENIX_WARNING_PARTIAL_RESTORE``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Assume all data restored
   fenix::data::member_restore(GROUP_ID, MEMBER_ID);

   // Continue with partially restored data - WRONG!
   compute(data);

**Correct:**

.. code-block:: cpp

   // RIGHT - Check if restore was partial
   Fenix_Data_subset found_data;
   int ret = fenix::data::member_load(GROUP_ID, MEMBER_ID,
                                      FENIX_DATA_SNAPSHOT_ALL,
                                      found_data);

   if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
     fprintf(stderr, "Warning: Only partial data restored\n");

     // Options:
     // 1. Recompute missing data
     // 2. Interpolate from neighbors
     // 3. Abort if critical data missing
   }

   Fenix_Data_subset_delete(&found_data);

**Why it matters:** Not all data may be restorable (e.g., more failures than redundancy can handle). Using partially restored data produces incorrect results.

Mistake: Creating Members Without a Group
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Create member before group
   fenix::data::member_create(GROUP_ID, MEMBER_ID,
                              data.data(), data.size(), MPI_DOUBLE);

   // Result: FENIX_ERROR_INVALID_GROUPID

**Correct:**

.. code-block:: cpp

   // RIGHT - Create group first
   fenix::data::group_create(GROUP_ID, {.comm = res_comm});

   fenix::data::member_create(GROUP_ID, MEMBER_ID,
                              data.data(), data.size(), MPI_DOUBLE);

**Why it matters:** Members must belong to a group. Creating members without a group fails.

**Order:** Always create groups before creating members in those groups.

Mistake: Forgetting to Commit
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Store without commit
   fenix::data::member_store(GROUP_ID, MEMBER_ID);

   // Data not committed yet!
   // Failure here = no recovery data available

**Correct:**

.. code-block:: cpp

   // RIGHT - Store and commit
   fenix::data::member_store(GROUP_ID, MEMBER_ID);
   fenix::data::commit_barrier(GROUP_ID);  // Make data recoverable

   // Or use checkpoint (stores all members + commits)
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

**Why it matters:** Only committed data can be recovered. Stored but not committed data is lost on failure.

**Remember:** Store is like a transaction; commit makes it durable.

----

Message Logging
---------------

Mistake: Not Activating Message Logger
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Create but don't activate
   fenix::mlog::create(LOG_ID, res_comm, 100);

   // Messages not logged!
   for (int iter = 0; iter < 1000; iter++) {
     MPI_Send(...);  // Not logged
   }

**Correct:**

.. code-block:: cpp

   // RIGHT - Create and activate
   fenix::mlog::create(LOG_ID, res_comm, 100);
   fenix::mlog::activate(LOG_ID);  // Start logging

   for (int iter = 0; iter < 1000; iter++) {
     MPI_Send(...);  // Now logged
   }

**Why it matters:** Only the active message logger records messages. Creating a logger without activating it has no effect.

Mistake: Not Beginning Regions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - No region management
   fenix::mlog::create(LOG_ID, res_comm, 100);
   fenix::mlog::activate(LOG_ID);

   for (int iter = 0; iter < 1000; iter++) {
     MPI_Send(...);  // All logged in one giant region
   }

   // Can't replay from specific iteration!

**Correct:**

.. code-block:: cpp

   // RIGHT - Begin region for each iteration
   fenix::mlog::create(LOG_ID, res_comm, 100);
   fenix::mlog::activate(LOG_ID);

   for (int iter = 0; iter < 1000; iter++) {
     fenix::mlog::begin_region(LOG_ID, iter);
     MPI_Send(...);  // Logged in region 'iter'
   }

   // Can replay from any iteration

**Why it matters:** Regions provide granularity for replay. Without regions, you can only replay from the beginning.

**Best practice:** Begin a new region for each logical unit of work (e.g., iteration, time step).

Mistake: Not Syncing After Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Recover without syncing logs
   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::role() == fenix::RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, MEMBER_ID);
       // No mlog sync - messages not replayed!
     }
   });

   // Recovered rank is out of sync with others

**Correct:**

.. code-block:: cpp

   // RIGHT - Sync message logs after recovery
   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::role() == fenix::RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, MEMBER_ID);
       fenix::mlog::sync(LOG_ID, last_checkpoint_region);
     }
   });

   // Or use automatic sync:
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

**Why it matters:** After restoring from a checkpoint, recovered ranks need to replay logged messages to catch up to current state.

----

Error Handling
--------------

Mistake: Not Checking Return Codes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Ignore return codes
   fenix::data::group_create(GROUP_ID, {.comm = res_comm});
   fenix::data::member_create(GROUP_ID, MEMBER_ID, ...);
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

   // If any failed, continue with corrupted state

**Correct:**

.. code-block:: cpp

   // RIGHT - Check return codes
   int ret = fenix::data::group_create(GROUP_ID, {.comm = res_comm});
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: group_create failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

   ret = fenix::data::member_create(GROUP_ID, MEMBER_ID, ...);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: member_create failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

   ret = fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: checkpoint failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

**Why it matters:** Fenix functions can fail for many reasons. Continuing after failure leads to undefined behavior.

**Minimum:** Check critical operations (create, checkpoint, restore).

Mistake: Treating Warnings as Errors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: cpp

   // WRONG - Abort on warning
   int ret = fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
   if (ret != FENIX_SUCCESS) {
     MPI_Abort(res_comm, ret);  // Aborts on warnings too!
   }

**Correct:**

.. code-block:: cpp

   // RIGHT - Handle warnings differently
   int ret = fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);

   if (ret < 0) {  // Negative = error
     fprintf(stderr, "Error: checkpoint failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   } else if (ret > 0) {  // Positive = warning
     fprintf(stderr, "Warning: checkpoint warning: %d\n", ret);
     // Continue, but take note
   }
   // ret == 0 (FENIX_SUCCESS) = all good

**Why it matters:** Warnings indicate degraded but still functional state. Aborting on warnings defeats the purpose of fault tolerance.

**Examples of warnings:**

- ``FENIX_WARNING_SPARE_RANKS_DEPLETED``: Out of spares, communicator shrank
- ``FENIX_WARNING_PARTIAL_RESTORE``: Not all data restored

----

Building & Running
------------------

Mistake: Building Without Release Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: bash

   # WRONG - Debug build for production
   cmake ..
   make

   # Result: 2-10x slower, assertions enabled

**Correct:**

.. code-block:: bash

   # RIGHT - Release build for production
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make

**Why it matters:** Debug builds include assertions, extra checks, and no optimization. Performance overhead is much higher.

**Use debug builds for:** Development and debugging only.

Mistake: Not Checking for Multiple MPI Versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: bash

   # Fenix compiled against MPI A
   # Runtime uses MPI B
   # Result: Segfault in MPI calls

**How to check:**

.. code-block:: bash

   # Check which MPI the binary uses
   ldd ./myapp | grep libmpi

   # Should show ONE libmpi.so path
   # If multiple paths, you have a version mismatch

**Solution:**

.. code-block:: bash

   # Rebuild with system include fix
   cd build
   cmake -DFENIX_SYSTEM_INC_FIX=ON ..
   make clean && make

**Why it matters:** Multiple MPI versions cause symbol conflicts and crashes.

Mistake: Insufficient Timeout for Tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:**

.. code-block:: bash

   # WRONG - Default timeout too short
   ctest

   # Tests timeout even though they're working

**Correct:**

.. code-block:: bash

   # RIGHT - Increase timeout
   ctest --timeout 60

   # Or for specific test
   ctest -R test_name -V --timeout 60

**Why it matters:** Fault tolerance tests can take time (spawn spares, recover, etc.). Default timeouts may be too short.

----

Summary: Top 10 Mistakes
-------------------------

1. **Forgetting ``--with-ft mpi`` flag** - Application won't be fault tolerant
2. **Using ``MPI_COMM_WORLD`` instead of resilient communicator** - Failures will abort
3. **Not updating buffer pointers after resize** - Corrupt checkpoints
4. **Checkpointing too frequently** - High overhead
5. **Not checking error codes** - Silent failures
6. **Using longjmp with C++ objects** - Resource leaks
7. **Not registering recovery callbacks** - Recovered ranks have no state
8. **Not committing after store** - No recovery data available
9. **Ignoring warnings** - Miss important information about degraded state
10. **Building without Release mode** - Unnecessary performance overhead

Avoiding These Mistakes
-----------------------

**Before every run:**

- [ ] Using ``--with-ft mpi`` flag?
- [ ] Using resilient communicator?
- [ ] Callbacks registered?
- [ ] Return codes checked?

**For checkpointing:**

- [ ] Frequency tuned? (not too often)
- [ ] Only critical state? (not everything)
- [ ] Committing after store?
- [ ] Buffer pointers updated after resize?

**For C++ applications:**

- [ ] Using inline recovery (not longjmp)?
- [ ] Checking for exceptions?
- [ ] Proper RAII (no resource leaks)?

**For production:**

- [ ] Release build?
- [ ] Single MPI version?
- [ ] Error handling comprehensive?
- [ ] Tested with fault injection?

See Also
--------

- :doc:`best-practices` - What you should do
- :doc:`troubleshooting` - Fix problems
- :doc:`cheat-sheet` - Quick reference
- :doc:`faq` - Common questions
