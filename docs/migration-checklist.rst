Migration Checklist
===================

Step-by-step guide for converting an existing MPI application to use Fenix fault tolerance.

.. contents:: Sections
   :local:
   :depth: 2

Overview
--------

**Estimated time:** 2-8 hours for basic process recovery, 1-3 days for data recovery

**Difficulty:**

- Basic process recovery: Easy
- With data recovery: Moderate
- With message logging: Advanced

**Phases:**

1. Preparation (30 minutes)
2. Add process recovery (1-2 hours)
3. Test basic recovery (1 hour)
4. Add data recovery (optional, 2-4 hours)
5. Add message logging (optional, 2-4 hours)
6. Performance tuning (2-8 hours)

Phase 1: Preparation
--------------------

1.1 Install Fenix and Dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- [ ] Open MPI 5.0+ with ULFM support installed
- [ ] Verified ULFM with: ``ompi_info | grep -i fault``
- [ ] Fenix library compiled and installed
- [ ] Test programs run with: ``mpiexec --with-ft mpi -n 2 hostname``

See: :doc:`installation`

1.2 Understand Your Application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Answer these questions about your application:**

Application characteristics:

- [ ] How long does it typically run? (minutes/hours/days)
- [ ] How many MPI ranks does it use? (10s/100s/1000s/10000s)
- [ ] What is the expected failure rate? (failures per day/week)
- [ ] Is the code primarily compute-bound or communication-bound?

Data characteristics:

- [ ] How much memory per rank? (<1GB/1-10GB/10-100GB/>100GB)
- [ ] What data needs to survive failures? (simulation state, counters, etc.)
- [ ] Is the data fixed-size or variable-size?
- [ ] How expensive is it to recompute data?

Code structure:

- [ ] Is there a main iterative loop?
- [ ] Are there clear checkpoint opportunities?
- [ ] Is the code C, C++, or mixed?
- [ ] Are there existing checkpointing mechanisms?

1.3 Set Up Version Control Branch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- [ ] Create a feature branch: ``git checkout -b add-fenix``
- [ ] Commit current working state
- [ ] Plan to commit after each migration phase

1.4 Choose Recovery Strategy
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Based on your application:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Strategy
     - Best For
     - Effort Level
   * - Process recovery only
     - Stateless apps, short runs
     - Low (2-4 hours)
   * - + Data recovery
     - Stateful apps, moderate runs
     - Moderate (1-2 days)
   * - + Message logging
     - Communication-heavy apps
     - High (2-3 days)

Decision:

- [ ] I will implement: ______________________________
- [ ] Expected implementation time: __________________

----

Phase 2: Add Process Recovery
------------------------------

2.1 Update Build System
~~~~~~~~~~~~~~~~~~~~~~~~

**CMakeLists.txt:**

.. code-block:: cmake

   find_package(MPI REQUIRED)
   find_library(FENIX_LIB fenix REQUIRED)

   add_executable(myapp myapp.cpp)
   target_link_libraries(myapp MPI::MPI_CXX ${FENIX_LIB})
   target_compile_features(myapp PRIVATE cxx_std_20)

**Verify:**

- [ ] Application compiles with Fenix linked
- [ ] No linker errors

2.2 Include Fenix Header
~~~~~~~~~~~~~~~~~~~~~~~~~

**C++ application:**

.. code-block:: cpp

   #include <fenix.hpp>  // Add at top of main file

**C application:**

.. code-block:: c

   #include <fenix.h>  // Add at top of main file

2.3 Replace MPI Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Before (original MPI):**

.. code-block:: cpp

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Application code

     MPI_Finalize();
   }

**After (with Fenix):**

.. code-block:: cpp

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix with N spare ranks
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 3});

     // Application code

     Fenix_Finalize();
     MPI_Finalize();
   }

**Checklist:**

- [ ] ``MPI_Init`` called before ``fenix::init``
- [ ] Spare count chosen (start with 2-3 for testing)
- [ ] ``Fenix_Finalize`` called before ``MPI_Finalize``
- [ ] Application compiles

2.4 Update Communicator Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Find all uses of MPI_COMM_WORLD:**

.. code-block:: bash

   grep -r "MPI_COMM_WORLD" src/

**Replace with resilient communicator:**

**Before:**

.. code-block:: cpp

   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
   MPI_Barrier(MPI_COMM_WORLD);

**After:**

.. code-block:: cpp

   int rank, size;
   MPI_Comm_rank(res_comm, &rank);
   MPI_Comm_size(res_comm, &size);

   MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, res_comm);
   MPI_Barrier(res_comm);

**Checklist:**

- [ ] All ``MPI_Comm_rank(MPI_COMM_WORLD, ...)`` → ``MPI_Comm_rank(res_comm, ...)``
- [ ] All ``MPI_Comm_size(MPI_COMM_WORLD, ...)`` → ``MPI_Comm_size(res_comm, ...)``
- [ ] All MPI operations use ``res_comm`` instead of ``MPI_COMM_WORLD``
- [ ] Derived communicators (if any) created from ``res_comm``
- [ ] Application compiles and runs

2.5 Configure Recovery Mode (C++ Only)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**For C++ applications, use inline recovery:**

.. code-block:: cpp

   MPI_Init(&argc, &argv);

   // Set inline recovery with exceptions
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   fenix::init({.out_comm = &res_comm, .spares = 3});

**Checklist:**

- [ ] ``fenix::set_option`` called after ``MPI_Init``
- [ ] Set to ``fenix::THROW`` for C++ (recommended)
- [ ] Or ``fenix::RETURN`` for manual error handling

**For C applications:** Can use default (longjmp) or set to ``FENIX_RESUME_RETURN`` for inline.

2.6 Add Basic Exception Handling (C++ with THROW)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Wrap main loop in try-catch:**

.. code-block:: cpp

   try {
     // Main application logic here
     for (int iter = 0; iter < max_iter; iter++) {
       compute();
       communicate(res_comm);
     }
   } catch (fenix::CommException& e) {
     printf("Recovered from failure\n");
     // For now, just exit - we'll improve this later
   }

**Checklist:**

- [ ] Main loop wrapped in try-catch
- [ ] Catches ``fenix::CommException``
- [ ] Application compiles

2.7 Commit Progress
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   git add -A
   git commit -m "Add basic Fenix process recovery"

**Verify:**

- [ ] Application compiles without errors
- [ ] Application runs without crashes (no failures injected yet)

----

Phase 3: Test Basic Recovery
-----------------------------

3.1 Test Without Failures
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Run with spares (7 active + 3 spares = 10 total)
   mpiexec --with-ft mpi -n 10 ./myapp

**Checklist:**

- [ ] Application runs to completion
- [ ] Output looks correct
- [ ] No errors in log
- [ ] 7 active ranks + 3 spares (check output if your app prints ranks)

3.2 Test With Failure Injection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Terminal 1: Run application**

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp

**Terminal 2: Kill a rank**

.. code-block:: bash

   # Find PIDs
   ps aux | grep myapp

   # Kill one rank (not rank 0 for easier testing)
   kill -9 <pid_of_rank_3>

**Checklist:**

- [ ] Application detects failure (may see error messages)
- [ ] Application attempts recovery
- [ ] For now: May crash or hang (expected without callbacks)
- [ ] Note what happens for next step

3.3 Add Recovery Callback
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Simple callback to verify recovery works:**

.. code-block:: cpp

   fenix::init({.out_comm = &res_comm, .spares = 3});

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     printf("[Rank %d] Recovery callback invoked\n", my_rank);
     printf("[Rank %d] Role: %d\n", my_rank, fenix::role());
     printf("[Rank %d] Spares remaining: %d\n", my_rank, fenix::nspare());

     // Update rank info
     MPI_Comm_rank(repaired, &my_rank);
     MPI_Comm_size(repaired, &num_ranks);
   });

**Checklist:**

- [ ] Callback registered after ``fenix::init``
- [ ] Callback updates ``my_rank`` from repaired communicator
- [ ] Application compiles

3.4 Test Recovery Again
~~~~~~~~~~~~~~~~~~~~~~~~

**Run with failure injection again:**

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp
   # In another terminal: kill -9 <pid>

**Expected behavior:**

- [ ] Application detects failure
- [ ] Callback prints messages
- [ ] Application continues execution
- [ ] Application completes (results may be wrong - that's ok for now)

**Common issues at this stage:**

- Hangs: May need to detect failures more frequently (see section 2.8)
- Crashes: Check error codes, verify all ranks return from callback
- Wrong results: Expected without data recovery

3.5 Add Failure Detection for Long Compute Loops (If Needed)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**If your application has long periods without MPI calls:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iter; iter++) {
     expensive_compute();  // 1 second per iteration

     // Check for failures every 10 iterations
     if (iter % 10 == 0) {
       fenix::detect_failures(true);
     }
   }

**Checklist:**

- [ ] Identify long compute loops (>10 seconds without MPI)
- [ ] Add ``fenix::detect_failures()`` calls periodically
- [ ] Test that recovery is faster with these calls

3.6 Commit Progress
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   git add -A
   git commit -m "Add recovery callback and testing"

**Current state:**

- [ ] Application survives single rank failure
- [ ] Callback executes on recovery
- [ ] Application continues (even if results wrong)

----

Phase 4: Add Data Recovery (Optional)
--------------------------------------

**Skip this phase if:** Your application is stateless or can reconstruct state easily.

4.1 Identify Critical State
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**List all state that must survive failures:**

.. list-table::
   :header-rows: 1
   :widths: 30 30 20 20

   * - Variable
     - Type
     - Size (per rank)
     - Checkpoint?
   * - ``simulation_state``
     - ``std::vector<double>``
     - 1 GB
     - YES - evolves over time
   * - ``iteration_count``
     - ``int``
     - 4 bytes
     - YES - needed for resume
   * - ``temp_buffer``
     - ``std::vector<double>``
     - 500 MB
     - NO - scratch space
   * - ``input_data``
     - ``const std::vector<double>``
     - 100 MB
     - NO - re-read from file

Fill in your application's state:

.. code-block:: text

   Critical state to checkpoint:
   1. _________________________________ (size: _________)
   2. _________________________________ (size: _________)
   3. _________________________________ (size: _________)

4.2 Create Data Group
~~~~~~~~~~~~~~~~~~~~~

**After** ``fenix::init``:

.. code-block:: cpp

   using namespace fenix::data;

   const int GROUP_ID = 1;

   int ret = group_create(GROUP_ID, {
     .comm = res_comm,
     .start_time_stamp = 0,
     .depth = 1,  // Keep 1 old snapshot + latest
     .policy_name = FENIX_DATA_POLICY_IMR
     // policy_value defaults to Mode 1
   });

   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: group_create failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

**Checklist:**

- [ ] Group created after ``fenix::init``
- [ ] Unique group ID chosen
- [ ] Return code checked
- [ ] Application compiles

4.3 Create Data Members
~~~~~~~~~~~~~~~~~~~~~~~~

**For each critical state variable:**

.. code-block:: cpp

   const int STATE_ID = 1, ITER_ID = 2;

   // Fixed-size member
   ret = member_create(GROUP_ID, STATE_ID,
                       simulation_state.data(),
                       simulation_state.size(),
                       MPI_DOUBLE);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: member_create failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

   // Small metadata
   ret = member_create(GROUP_ID, ITER_ID,
                       &iteration_count, 1, MPI_INT);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: member_create failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

**For variable-size data:**

.. code-block:: cpp

   ret = member_create(GROUP_ID, DYNAMIC_ID,
                       dynamic_data.data(),
                       FENIX_RESIZEABLE,  // Variable size
                       MPI_DOUBLE);

**Checklist:**

- [ ] All critical state registered as members
- [ ] Unique member IDs chosen
- [ ] Correct MPI datatypes used
- [ ] Variable-size data uses ``FENIX_RESIZEABLE``
- [ ] Return codes checked
- [ ] Application compiles

4.4 Add Checkpointing
~~~~~~~~~~~~~~~~~~~~~

**In main loop, checkpoint periodically:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iter; iter++) {
     // Computation
     compute(simulation_state);
     communicate(res_comm, simulation_state);

     iteration_count = iter;

     // Checkpoint every 10 iterations
     if (iter % 10 == 0) {
       int ret = checkpoint(GROUP_ID, SUBSET_FULL);
       if (ret != FENIX_SUCCESS) {
         fprintf(stderr, "Error: checkpoint failed: %d\n", ret);
         MPI_Abort(res_comm, ret);
       }

       if (my_rank == 0) {
         printf("Checkpoint at iteration %d\n", iter);
       }
     }
   }

**Checklist:**

- [ ] Checkpoint called periodically (every N iterations)
- [ ] Frequency chosen (start with every 10-50 iterations)
- [ ] Return code checked
- [ ] Application compiles and runs

4.5 Update Recovery Callback to Restore
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Update callback from Phase 3:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     printf("[Rank %d] Recovery callback invoked\n", my_rank);

     // Check for errors
     if (fenix::error() != FENIX_SUCCESS) {
       fprintf(stderr, "Recovery error: %d\n", fenix::error());
       return;
     }

     // Update rank
     MPI_Comm_rank(repaired, &my_rank);

     // Recovered ranks restore state
     if (fenix::role() == fenix::RECOVERED_RANK) {
       printf("[Rank %d] I'm recovered, restoring state\n", my_rank);

       int ret = member_restore(GROUP_ID, STATE_ID);
       if (ret != FENIX_SUCCESS) {
         fprintf(stderr, "Error: restore failed: %d\n", ret);
         MPI_Abort(repaired, ret);
       }

       ret = member_restore(GROUP_ID, ITER_ID);
       if (ret != FENIX_SUCCESS) {
         fprintf(stderr, "Error: restore failed: %d\n", ret);
         MPI_Abort(repaired, ret);
       }

       printf("[Rank %d] State restored, iteration_count=%d\n",
              my_rank, iteration_count);
     }
   });

**Checklist:**

- [ ] Callback checks ``fenix::error()``
- [ ] Recovered ranks restore all critical state
- [ ] Return codes checked
- [ ] Application compiles

4.6 Handle Variable-Size Data (If Applicable)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**If using** ``FENIX_RESIZEABLE``:

.. code-block:: cpp

   // After resize
   dynamic_data.resize(new_size);

   // Update buffer pointer
   int flag;
   ret = Fenix_Data_member_attr_set(
     GROUP_ID, DYNAMIC_ID,
     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     dynamic_data.data(),
     &flag
   );

   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: attr_set failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

**Checklist:**

- [ ] Buffer pointer updated after every resize
- [ ] Return code checked
- [ ] Application compiles and runs

4.7 Test Data Recovery
~~~~~~~~~~~~~~~~~~~~~~

**Run with checkpoint and recovery:**

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp

**In another terminal:**

.. code-block:: bash

   # Wait for several checkpoints
   sleep 30

   # Kill a rank
   kill -9 <pid_of_rank>

**Expected behavior:**

- [ ] Application checkpoints periodically
- [ ] Application detects failure
- [ ] Recovered rank restores state
- [ ] Application continues with correct state
- [ ] Results are correct

**If results are wrong:**

- Check that all critical state is checkpointed
- Check that recovered ranks restore all members
- Validate restored data (print checksums, ranges, etc.)

4.8 Tune Checkpoint Frequency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Measure checkpoint time:**

.. code-block:: cpp

   double start = MPI_Wtime();
   checkpoint(GROUP_ID, SUBSET_FULL);
   double ckpt_time = MPI_Wtime() - start;

   if (my_rank == 0) {
     printf("Checkpoint took %.3f seconds\n", ckpt_time);
   }

**Adjust frequency:**

- If checkpoint takes < 1 second and happens every minute: Good
- If checkpoint takes 10 seconds: Consider less frequent or partial checkpoints
- Target: Checkpoint overhead < 5-10% of total runtime

**Checklist:**

- [ ] Checkpoint time measured
- [ ] Frequency adjusted based on overhead
- [ ] Overhead acceptable (< 5-10%)

4.9 Commit Progress
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   git add -A
   git commit -m "Add data recovery"

**Current state:**

- [ ] Critical state identified and checkpointed
- [ ] Recovered ranks restore state
- [ ] Application produces correct results after recovery

----

Phase 5: Add Message Logging (Optional)
----------------------------------------

**Skip this phase if:** Your application is compute-bound or has infrequent communication.

**Use this phase if:** Communication is expensive and you want faster recovery.

5.1 Create Message Logger
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   const int LOG_ID = 1;
   const int WINDOW_SIZE = 100;  // Keep 100 regions

   int ret = mlog::create(LOG_ID, res_comm, WINDOW_SIZE);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: mlog create failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

   ret = mlog::activate(LOG_ID);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: mlog activate failed: %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

**Checklist:**

- [ ] Logger created after ``fenix::init``
- [ ] Window size chosen (start with 50-100)
- [ ] Logger activated
- [ ] Return codes checked

5.2 Add Region Management
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   for (int iter = 0; iter < max_iter; iter++) {
     // Begin region for this iteration
     mlog::begin_region(LOG_ID, iter);

     // Computation and communication
     compute(simulation_state);
     communicate(res_comm, simulation_state);  // Logged

     // Checkpoint periodically
     if (iter % 50 == 0) {
       checkpoint(GROUP_ID, SUBSET_FULL);
     }
   }

**Checklist:**

- [ ] Region begins at start of each iteration
- [ ] Region ID is unique (iteration number works well)
- [ ] All communication happens within regions

5.3 Update Recovery to Sync Logs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     // ... existing recovery logic ...

     if (fenix::role() == fenix::RECOVERED_RANK) {
       // Restore from checkpoint
       member_restore(GROUP_ID, STATE_ID);
       member_restore(GROUP_ID, ITER_ID);

       // Sync message logs from checkpoint iteration
       int ret = mlog::sync(LOG_ID, iteration_count);
       if (ret != FENIX_SUCCESS) {
         fprintf(stderr, "Error: mlog sync failed: %d\n", ret);
         MPI_Abort(repaired, ret);
       }

       printf("[Rank %d] Logs synced from iteration %d\n",
              my_rank, iteration_count);
     }
   });

**Checklist:**

- [ ] Logs synced after restoring checkpoint
- [ ] Sync starts from checkpointed iteration
- [ ] Return code checked

5.4 Enable Automatic Sync (Alternative)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Instead of manual sync in callback:**

.. code-block:: cpp

   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

**This automatically syncs logs after callbacks.**

5.5 Test Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~

**Run with message logging:**

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp

**Inject failure during execution:**

.. code-block:: bash

   sleep 30
   kill -9 <pid_of_rank>

**Expected behavior:**

- [ ] Application logs messages
- [ ] On recovery, logs are synced
- [ ] Recovery is faster than without message logging
- [ ] Results are correct

5.6 Checkpoint Message Logs (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**To survive multiple failures:**

.. code-block:: cpp

   // Link mlog to data group
   mlog::create_data_member(LOG_ID, GROUP_ID, MLOG_MEMBER_ID);

   // Checkpoint as normal (mlog is checkpointed with group)
   checkpoint(GROUP_ID, SUBSET_FULL);

**Checklist:**

- [ ] Message log linked to data group
- [ ] Checkpointed with other members
- [ ] Restored automatically

5.7 Commit Progress
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   git add -A
   git commit -m "Add message logging"

**Current state:**

- [ ] Messages logged during execution
- [ ] Logs synced on recovery
- [ ] Recovery faster than without logging

----

Phase 6: Performance Tuning
----------------------------

6.1 Measure Baseline Performance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Run without Fenix:**

.. code-block:: bash

   # Temporarily disable Fenix for baseline
   mpiexec -n 7 ./myapp_no_fenix

   # Time: __________ seconds

**Run with Fenix (no checkpointing):**

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp  # 7 active + 3 spares

   # Time: __________ seconds
   # Overhead: __________ %

**Run with checkpointing:**

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp

   # Time: __________ seconds
   # Overhead: __________ %

**Target overheads:**

- Process recovery only: < 1%
- With checkpointing: < 5-10%
- With message logging: < 10-15%

6.2 Profile Checkpoint Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   double total_ckpt_time = 0.0;
   int num_checkpoints = 0;

   for (int iter = 0; iter < max_iter; iter++) {
     compute();

     if (iter % checkpoint_interval == 0) {
       double t_start = MPI_Wtime();
       checkpoint(GROUP_ID, SUBSET_FULL);
       double t_ckpt = MPI_Wtime() - t_start;

       total_ckpt_time += t_ckpt;
       num_checkpoints++;

       if (my_rank == 0) {
         printf("Checkpoint %d took %.3f s\n", num_checkpoints, t_ckpt);
       }
     }
   }

   if (my_rank == 0) {
     printf("Average checkpoint time: %.3f s\n",
            total_ckpt_time / num_checkpoints);
     printf("Total checkpoint overhead: %.1f%%\n",
            (total_ckpt_time / total_runtime) * 100.0);
   }

6.3 Optimize Checkpoint Frequency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**If overhead too high:**

- [ ] Checkpoint less frequently (e.g., every 50 instead of 10)
- [ ] Use partial checkpoints (only changed data)
- [ ] Use staging to overlap computation and checkpointing

**If recovery too slow:**

- [ ] Checkpoint more frequently
- [ ] Enable message logging
- [ ] Reduce data size per checkpoint

6.4 Try Partial Checkpoints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**If only some data changes frequently:**

.. code-block:: cpp

   // Full checkpoint every 100 iterations
   if (iter % 100 == 0) {
     checkpoint(GROUP_ID, SUBSET_FULL);
   }
   // Partial checkpoint every 10 iterations
   else if (iter % 10 == 0) {
     // Create subset for frequently-changing data
     Fenix_Data_subset hot_data;
     Fenix_Data_subset_create(1, 0, hot_data_size-1, 0, &hot_data);

     member_store(GROUP_ID, HOT_MEMBER_ID, hot_data);
     commit_barrier(GROUP_ID);

     Fenix_Data_subset_delete(&hot_data);
   }

6.5 Choose IMR Policy Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Test Mode 1 vs Mode 5:**

.. code-block:: cpp

   // Mode 1 (current)
   group_create(GROUP_ID, {
     .policy_value = &(Fenix_Data_policy_in_memory_raid){
       .mode = 1,
       .level = 1
     }
   });

   // Time: __________ seconds

.. code-block:: cpp

   // Mode 5
   group_create(GROUP_ID, {
     .policy_value = &(Fenix_Data_policy_in_memory_raid){
       .mode = 5,
       .level = 5
     }
   });

   // Time: __________ seconds

**Choose based on:**

- Mode 1: Faster, simpler, 2x memory
- Mode 5: Slower, complex, 1.25x memory (for level 5)

6.6 Verify Release Build
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make clean && make

**Test performance again:**

- [ ] Release build significantly faster than Debug
- [ ] Performance acceptable for production

6.7 Final Performance Checklist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- [ ] Overhead < 10% of baseline runtime
- [ ] Checkpoint time reasonable (< 5% of checkpoint interval)
- [ ] Memory usage acceptable (check with ``top`` or ``ps``)
- [ ] Recovery time < 1 minute for typical failure

6.8 Document Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Create a configuration file:**

.. code-block:: cpp

   // fenix_config.hpp
   namespace FenixConfig {
     // Spares
     constexpr int NUM_SPARES = 50;  // 5% of 1000 ranks

     // Checkpointing
     constexpr int CHECKPOINT_INTERVAL = 100;  // Every 100 iterations
     constexpr int CHECKPOINT_DEPTH = 1;        // Keep 1 old + latest

     // IMR Policy
     constexpr int IMR_MODE = 5;
     constexpr int IMR_LEVEL = 5;

     // Message Logging
     constexpr bool USE_MLOG = true;
     constexpr int MLOG_WINDOW = 100;

     // Performance
     // - Checkpoint overhead: ~3%
     // - Memory per rank: ~600 MB for checkpoints
     // - Recovery time: ~30 seconds typical
   }

----

Final Checklist
---------------

Code Changes:

- [ ] Fenix headers included
- [ ] ``fenix::init`` called with appropriate spares
- [ ] All ``MPI_COMM_WORLD`` replaced with resilient communicator
- [ ] Recovery callbacks registered
- [ ] Data groups and members created (if using data recovery)
- [ ] Checkpointing added (if using data recovery)
- [ ] State restoration in callbacks (if using data recovery)
- [ ] Message logging configured (if using)
- [ ] ``Fenix_Finalize`` called before ``MPI_Finalize``

Testing:

- [ ] Compiles without errors or warnings
- [ ] Runs without failures (sanity check)
- [ ] Survives single rank failure
- [ ] Survives multiple simultaneous failures
- [ ] Produces correct results after recovery
- [ ] Tested with failures at different times (early, middle, late)
- [ ] Tested with failures during checkpoint
- [ ] Performance overhead measured and acceptable

Documentation:

- [ ] Configuration documented
- [ ] Recovery strategy explained
- [ ] Known limitations noted
- [ ] Users know how to run (``--with-ft mpi`` flag)

Version Control:

- [ ] All changes committed
- [ ] Feature branch ready for merge
- [ ] Tag created for this milestone

----

Troubleshooting Migration Issues
---------------------------------

Application Hangs After Migration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible causes:**

1. Forgot ``--with-ft mpi`` flag
2. Long compute loops without failure detection
3. Deadlock in recovery callback

**Solutions:**

- Verify mpiexec command includes ``--with-ft mpi``
- Add ``fenix::detect_failures()`` in long loops
- Check that all ranks exit callbacks

Application Crashes After Migration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible causes:**

1. Multiple MPI versions
2. Unchecked error codes
3. Buffer pointer not updated after resize

**Solutions:**

- Check with ``ldd ./myapp | grep libmpi``
- Enable ``-DFENIX_SYSTEM_INC_FIX=ON``
- Add error checking to all Fenix calls
- Update buffer pointers after resize

Wrong Results After Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible causes:**

1. Not all critical state checkpointed
2. State not restored in callback
3. Partial restore not handled

**Solutions:**

- Review what state is checkpointed
- Ensure all members restored in callback
- Check for ``FENIX_WARNING_PARTIAL_RESTORE``

Performance Worse Than Expected
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible causes:**

1. Debug build instead of Release
2. Checkpointing too frequently
3. Checkpointing too much data

**Solutions:**

- Rebuild with ``-DCMAKE_BUILD_TYPE=Release``
- Reduce checkpoint frequency
- Checkpoint only critical state
- Use partial checkpoints

See Also
--------

- :doc:`howto/migrate-existing-app` - Detailed migration guide
- :doc:`best-practices` - Production deployment checklist
- :doc:`common-mistakes` - Avoid pitfalls
- :doc:`troubleshooting` - Fix problems
- :doc:`examples/index` - Example programs
