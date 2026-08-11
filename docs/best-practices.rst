Best Practices
==============

Production deployment checklist and guidelines for building robust fault-tolerant applications with Fenix.

.. contents:: Sections
   :local:
   :depth: 2

Quick Checklist
---------------

Before deploying your Fenix application to production:

**Process Recovery:**

- [ ] Using resilient communicator instead of MPI_COMM_WORLD
- [ ] Spare rank count sized appropriately (5-10% for large jobs)
- [ ] Recovery callbacks registered and tested
- [ ] Tested recovery from single rank failure
- [ ] Tested recovery from multiple simultaneous failures
- [ ] Tested spare depletion scenario
- [ ] Long compute phases include periodic ``detect_failures()`` calls
- [ ] Using inline recovery (C++) or careful with longjmp (C)

**Data Recovery:**

- [ ] Checkpoint frequency tuned (not too often, not too rare)
- [ ] Only checkpointing critical state (not temporary data)
- [ ] Partial checkpoints used where appropriate
- [ ] Data member buffer pointers updated after resizing
- [ ] Checkpoint depth appropriate for memory constraints
- [ ] IMR policy mode chosen (Mode 1 vs Mode 5)
- [ ] Tested data restoration after recovery
- [ ] Data validation after recovery passes

**Error Handling:**

- [ ] Return codes checked for all Fenix functions
- [ ] Recovery callback checks ``fenix::error()``
- [ ] Handling ``FENIX_WARNING_SPARE_RANKS_DEPLETED``
- [ ] Handling ``FENIX_WARNING_PARTIAL_RESTORE``
- [ ] Error paths tested and working

**Testing:**

- [ ] Tested with fault injection (kill ranks during execution)
- [ ] Tested with varying failure timing (early, middle, late)
- [ ] Tested with failures during checkpointing
- [ ] Tested with failures during recovery
- [ ] Performance overhead measured and acceptable
- [ ] Memory usage monitored (checkpoints in RAM)

**Performance:**

- [ ] Built with Release mode (``-DCMAKE_BUILD_TYPE=Release``)
- [ ] Profiled to identify bottlenecks
- [ ] Checkpoint overhead < 10% of total runtime
- [ ] Recovery time < 5% of checkpoint interval
- [ ] Spare ranks appropriate for system failure rate

**Documentation:**

- [ ] Recovery strategy documented for users
- [ ] Checkpoint frequency documented
- [ ] Spare rank recommendations documented
- [ ] Known limitations documented

----

Process Recovery Best Practices
--------------------------------

Use the Resilient Communicator
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Always use the resilient communicator returned by** ``Fenix_Init``, never ``MPI_COMM_WORLD``:

.. code-block:: cpp

   // GOOD
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 3});
   MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, res_comm);

   // BAD - Won't be fault tolerant!
   MPI_Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

**Rationale:** Only operations on the resilient communicator (and communicators derived from it) are protected by Fenix.

Size Spare Ranks Appropriately
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Rule of thumb:** 5-10% of total ranks for large, long-running jobs.

**Consider:**

- **Job duration:** Longer jobs need more spares (higher cumulative failure probability)
- **System reliability:** Less reliable hardware needs more spares
- **Tolerance for shrinkage:** How many failures can your algorithm handle with reduced ranks?
- **Recovery cost:** Spare ranks sit idle - balance against recovery overhead

**Examples:**

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 40

   * - Job Size
     - Duration
     - Recommended Spares
     - Notes
   * - 100 ranks
     - 1 hour
     - 2-3
     - Low failure probability
   * - 1000 ranks
     - 8 hours
     - 25-50
     - Moderate probability
   * - 10000 ranks
     - 48 hours
     - 500-1000
     - High probability, long window
   * - 100000 ranks
     - 24 hours
     - 5000-10000
     - Very high scale

**Monitor and adjust:** Track actual failure rates and adjust spare counts in future runs.

Handle Spare Depletion Gracefully
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Check for the spare depletion warning:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::error() == FENIX_WARNING_SPARE_RANKS_DEPLETED) {
       printf("WARNING: Out of spares! Communicator has shrunk.\n");
       printf("Remaining spares: %d\n", fenix::nspare());

       // Options:
       // 1. Continue with reduced ranks
       // 2. Checkpoint to disk and abort gracefully
       // 3. Adjust algorithm for new communicator size
     }

     // Normal recovery...
   });

**Consider:**

- Can your algorithm handle a smaller communicator?
- Do ranks need to rebalance work after shrinkage?
- Should you checkpoint to disk as a backup when spares are low?

Use Inline Recovery (C++)
~~~~~~~~~~~~~~~~~~~~~~~~~~

**For C++ applications, prefer inline recovery with exceptions:**

.. code-block:: cpp

   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   try {
     // Application code
     for (int iter = 0; iter < max_iter; iter++) {
       compute();
       communicate(res_comm);
     }
   } catch (fenix::CommException& e) {
     // Handle recovery
     handle_recovery();
     // Restart or continue as appropriate
   }

**Advantages:**

- No undefined behavior from longjmp
- Proper C++ destructor calls
- Predictable with compiler optimizations
- Cleaner error handling

**Rationale:** ``longjmp`` (a C library function that jumps to a saved program state) bypasses normal C++ control flow. This means C++ destructors may not be called when leaving scope via longjmp, RAII objects (smart pointers, mutexes, etc.) may leak resources, and compiler optimizations may make variable values unpredictable. Exceptions provide well-defined C++ semantics.

Detect Failures During Long Compute Phases
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**For applications with long periods between MPI calls, periodically check for failures:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iter; iter++) {
     // Long computation with no MPI calls
     expensive_computation();

     // Check for failures every N iterations
     if (iter % 100 == 0) {
       fenix::detect_failures(true);  // Recover if detected
     }
   }

**Rationale:** Failures can only be detected during MPI operations. Long compute phases delay recovery, potentially allowing more failures to accumulate.

Register Recovery Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Always register callbacks for recovery actions:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     // Check for errors first
     if (fenix::error() != FENIX_SUCCESS) {
       printf("Recovery error: %d\n", fenix::error());
       return;
     }

     // Recovered ranks need to restore state
     if (fenix::role() == fenix::RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, STATE_MEMBER_ID);
       fenix::data::member_restore(GROUP_ID, VELOCITY_MEMBER_ID);
     }

     // All ranks may need to update communicator-dependent state
     MPI_Comm_rank(repaired, &my_rank);
     MPI_Comm_size(repaired, &comm_size);
   });

**Important:** Recovered ranks (former spares) do not have callbacks registered, so they must restore state inside callbacks registered by survivor ranks or through other means.

----

Data Recovery Best Practices
-----------------------------

Checkpoint Only Critical State
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Don't checkpoint everything - only what's necessary for recovery:**

**DO checkpoint:**

- State that changes over iterations
- Data expensive to recompute
- Initial conditions (if non-trivial)
- Iteration counters and convergence state

**DON'T checkpoint:**

- Temporary/scratch buffers
- Data that's quick to recompute
- Read-only input data (re-read from disk)
- Derived values (recalculate after restore)

**Example:**

.. code-block:: cpp

   // Application state
   std::vector<double> state;           // CHECKPOINT - evolves over time
   std::vector<double> velocity;        // CHECKPOINT - evolves over time
   std::vector<double> temp_buffer;     // DON'T - temporary
   const std::vector<double> input;     // DON'T - read-only, re-read from file
   double derived_value;                // DON'T - recompute from state

**Rationale:** Smaller checkpoints = less memory, less network traffic, faster checkpointing.

Tune Checkpoint Frequency
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Balance checkpoint overhead vs. recovery time:**

- **Checkpoint too often:** High overhead during normal execution
- **Checkpoint too rarely:** Long recovery time (more to recompute)

**Guidelines:**

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Application Type
     - Suggested Frequency
     - Notes
   * - Compute-intensive
     - Every 5-10 minutes wall time
     - Computation expensive to repeat
   * - Communication-intensive
     - Every 1-2 minutes
     - Combine with message logging
   * - Large data (>1GB/rank)
     - Less frequent (10-20 min)
     - Network transfer overhead
   * - Small data (<100MB/rank)
     - More frequent (2-5 min)
     - Low overhead

**Measure and adjust:**

.. code-block:: cpp

   // Measure checkpoint time
   double start = MPI_Wtime();
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
   double checkpoint_time = MPI_Wtime() - start;

   printf("Checkpoint took %.3f seconds\n", checkpoint_time);

**Target:** Checkpoint overhead should be < 5-10% of time between checkpoints.

Use Partial Checkpoints
~~~~~~~~~~~~~~~~~~~~~~~~

**Checkpoint only changed or critical portions of data:**

.. code-block:: cpp

   // Example: Stencil code - only checkpoint boundaries
   const int n = 1000;
   const int boundary_width = 10;

   Fenix_Data_subset boundary;
   Fenix_Data_subset_createv(2,
     (int[]){0, n - boundary_width},           // Start offsets
     (int[]){boundary_width - 1, n - 1},       // End offsets
     &boundary);

   // Checkpoint only boundaries (20 elements vs 1000)
   fenix::data::member_store(GROUP_ID, MEMBER_ID, boundary);
   fenix::data::commit_barrier(GROUP_ID);

   // Cleanup
   Fenix_Data_subset_delete(&boundary);

**Use cases:**

- Stencil codes: Checkpoint boundaries, recompute interior
- Sparse data: Checkpoint only non-zero regions
- Hierarchical data: Checkpoint coarse grid, interpolate fine grid

**Rationale:** Can reduce checkpoint size by 10-100x in some applications.

Update Buffer Pointers After Resizing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Critical for resizable data members:**

.. code-block:: cpp

   // Create resizable member
   std::vector<double> data;
   fenix::data::member_create(GROUP_ID, MEMBER_ID,
                              data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // ... checkpoint ...

   // Later: resize
   data.resize(new_size);

   // MUST update buffer pointer
   int flag;
   Fenix_Data_member_attr_set(
     GROUP_ID, MEMBER_ID,
     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     data.data(),  // New pointer after resize
     &flag
   );

**Rationale:** Vector resizing may reallocate memory, invalidating old pointers. Fenix must know the current location.

Choose Appropriate IMR Policy Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Mode 1 (RAID-1 style mirroring):**

- Memory: 2x per checkpoint
- Recovery: Fast (simple copy)
- Best for: Small-to-moderate data, simple setup

**Mode 5 (RAID-5 style parity):**

- Memory: (N/(N-1))x per checkpoint (e.g., 1.25x for group size 5)
- Recovery: Slower (parity reconstruction)
- Best for: Large data, memory-constrained systems

**Example:**

.. code-block:: cpp

   // Mode 1: Simple mirroring
   Fenix_Data_group_create(GROUP_ID, res_comm, 0, depth,
                           FENIX_DATA_POLICY_IMR,
                           &(Fenix_Data_policy_in_memory_raid){
                             .mode = 1,
                             .level = 1
                           }, &flag);

   // Mode 5: Parity groups of 5
   Fenix_Data_group_create(GROUP_ID, res_comm, 0, depth,
                           FENIX_DATA_POLICY_IMR,
                           &(Fenix_Data_policy_in_memory_raid){
                             .mode = 5,
                             .level = 5
                           }, &flag);

**Decision factors:**

- Available memory per rank
- Recovery time requirements
- Network bandwidth
- Failure patterns (single vs. multiple simultaneous)

Limit Checkpoint Depth
~~~~~~~~~~~~~~~~~~~~~~~

**Keep only as many historical snapshots as needed:**

.. code-block:: cpp

   // Depth 0: Keep only latest (minimal memory)
   fenix::data::group_create(GROUP_ID, {.depth = 0});

   // Depth 1: Keep latest + 1 previous (allows rollback)
   fenix::data::group_create(GROUP_ID, {.depth = 1});

**Rationale:** Each retained snapshot consumes memory. Most applications only need the latest snapshot.

**Use higher depth when:**

- You want to recover to an earlier state (e.g., if latest is corrupted)
- Implementing multi-level checkpointing
- Debugging recovery issues

Use Staging for Performance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decouple data serialization from network communication:**

.. code-block:: cpp

   // Stage locally (fast, no communication)
   fenix::data::member_stage(GROUP_ID, MEMBER_ID);

   // Continue computation while staged...

   // Later: store pre-staged data (collective, network communication)
   fenix::data::member_store(GROUP_ID, MEMBER_ID,
                             fenix::data::SUBSET_PRESTAGED);
   fenix::data::commit_barrier(GROUP_ID);

**Benefits:**

- Overlaps staging with computation
- Reduces time spent in collective operations
- Improves scalability on large systems

**Best for:** Large data members where serialization is significant.

----

Error Handling Best Practices
------------------------------

Check All Return Codes
~~~~~~~~~~~~~~~~~~~~~~~

**Never ignore Fenix return codes:**

.. code-block:: cpp

   // BAD
   fenix::data::member_restore(GROUP_ID, MEMBER_ID);

   // GOOD
   int ret = fenix::data::member_restore(GROUP_ID, MEMBER_ID);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Error: member_restore failed with code %d\n", ret);
     MPI_Abort(res_comm, ret);
   }

**At minimum, check critical operations:**

- Data group/member creation
- Checkpoint commits
- Data restoration
- Message log operations

Validate Restored Data
~~~~~~~~~~~~~~~~~~~~~~

**Verify data integrity after restoration:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (fenix::role() == fenix::RECOVERED_RANK) {
       Fenix_Data_subset found_data;
       int ret = fenix::data::member_load(GROUP_ID, MEMBER_ID,
                                          FENIX_DATA_SNAPSHOT_ALL,
                                          found_data);

       if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
         fprintf(stderr, "Warning: Only partial data restored\n");
         // Decide: continue, recompute missing, or abort
       }

       // Validate data makes sense
       if (!validate_state(data)) {
         fprintf(stderr, "Error: Restored data validation failed\n");
         MPI_Abort(comm, 1);
       }

       Fenix_Data_subset_delete(&found_data);
     }
   });

**Validation examples:**

- Check ranges (values within expected bounds)
- Check conservation laws (e.g., total mass preserved)
- Check checksums or hashes
- Check data structure invariants

Handle Warnings Appropriately
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Don't ignore warnings - they indicate degraded state:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm comm, int err) {
     int fenix_err = fenix::error();

     if (fenix_err == FENIX_WARNING_SPARE_RANKS_DEPLETED) {
       printf("WARNING: Spare ranks depleted\n");
       printf("Remaining spares: %d\n", fenix::nspare());

       // Take action:
       // - Checkpoint to disk immediately
       // - Reduce checkpoint interval
       // - Alert monitoring system
     }

     if (fenix_err == FENIX_WARNING_PARTIAL_RESTORE) {
       printf("WARNING: Only partial data restored\n");

       // Options:
       // - Recompute missing data
       // - Interpolate from neighbors
       // - Abort if critical data missing
     }

     // Normal recovery...
   });

Implement Fallback Strategies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Have a backup plan if Fenix recovery fails:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm comm, int err) {
     // Try Fenix recovery first
     int ret = fenix::data::member_restore(GROUP_ID, MEMBER_ID);

     if (ret != FENIX_SUCCESS) {
       printf("Fenix recovery failed, attempting fallback\n");

       // Fallback option 1: Read from disk checkpoint
       if (disk_checkpoint_available()) {
         read_from_disk_checkpoint(data);
       }
       // Fallback option 2: Recompute from initial conditions
       else if (can_restart_from_beginning()) {
         initialize_state(data);
       }
       // Fallback option 3: Abort gracefully
       else {
         fprintf(stderr, "All recovery options exhausted\n");
         MPI_Abort(comm, 1);
       }
     }
   });

----

Testing Best Practices
----------------------

Test with Fault Injection
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Regularly test recovery by killing ranks:**

.. code-block:: bash

   # Terminal 1: Run application
   mpiexec --with-ft mpi -n 10 ./myapp

   # Terminal 2: Find and kill a rank
   ps aux | grep myapp
   kill -9 <pid_of_one_rank>

**Test different scenarios:**

- Kill single rank
- Kill multiple ranks simultaneously
- Kill ranks during checkpoint
- Kill ranks during recovery
- Kill different ranks (first, middle, last)
- Repeated failures (multiple failure-recovery cycles)

**Automate testing:**

.. code-block:: bash

   #!/bin/bash
   # test_recovery.sh

   mpiexec --with-ft mpi -n 10 ./myapp &
   APP_PID=$!

   sleep 5
   RANK_PID=$(pgrep -f myapp | head -1)
   echo "Killing rank $RANK_PID"
   kill -9 $RANK_PID

   wait $APP_PID
   EXIT_CODE=$?

   if [ $EXIT_CODE -eq 0 ]; then
     echo "PASS: Application recovered successfully"
   else
     echo "FAIL: Application did not recover (exit code $EXIT_CODE)"
   fi

Test Spare Depletion
~~~~~~~~~~~~~~~~~~~~

**Verify behavior when spares run out:**

.. code-block:: bash

   # Run with limited spares
   mpiexec --with-ft mpi -n 8 ./myapp  # 5 active + 3 spares

   # Kill more ranks than spares available
   # Watch for FENIX_WARNING_SPARE_RANKS_DEPLETED

**Verify:**

- Application continues with reduced ranks
- Warning is detected and logged
- Rank IDs updated correctly
- Algorithm adapts to new communicator size

Measure Performance Overhead
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Quantify Fenix overhead before production deployment:**

.. code-block:: cpp

   // Measure checkpoint overhead
   double total_time = 0.0;
   int checkpoint_count = 0;

   for (int iter = 0; iter < max_iter; iter++) {
     compute();

     if (iter % checkpoint_interval == 0) {
       double start = MPI_Wtime();
       fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
       double ckpt_time = MPI_Wtime() - start;

       total_time += ckpt_time;
       checkpoint_count++;
     }
   }

   double avg_ckpt_time = total_time / checkpoint_count;
   double ckpt_overhead = (total_time / total_runtime) * 100.0;

   printf("Average checkpoint time: %.3f s\n", avg_ckpt_time);
   printf("Checkpoint overhead: %.1f%%\n", ckpt_overhead);

**Target:** < 5-10% overhead for checkpointing.

**Compare:**

- Runtime with vs. without checkpointing
- Different checkpoint frequencies
- Different IMR policy modes
- Different checkpoint sizes (full vs. partial)

Monitor Memory Usage
~~~~~~~~~~~~~~~~~~~~

**Track memory consumption of checkpoints:**

.. code-block:: cpp

   // Query memory usage
   void print_memory_usage() {
     struct rusage usage;
     getrusage(RUSAGE_SELF, &usage);
     printf("Memory usage: %.1f MB\n", usage.ru_maxrss / 1024.0);
   }

   // Before and after checkpoint
   print_memory_usage();
   fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
   print_memory_usage();

**Watch for:**

- Memory growth with each checkpoint
- Memory spikes during recovery
- Unexpected memory leaks

**Rationale:** Checkpoints are stored in RAM. Large or frequent checkpoints can exhaust memory.

----

Deployment Best Practices
--------------------------

Document Recovery Strategy
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Provide clear documentation for users:**

.. code-block:: text

   # MyApp Fault Tolerance Configuration

   ## Spare Ranks
   - Recommended: 50 spares for 1000 rank jobs
   - Adjust based on observed failure rates

   ## Checkpointing
   - Frequency: Every 10 minutes (60 iterations)
   - Data size: ~500 MB per rank
   - IMR Policy: Mode 5, group size 5

   ## Recovery
   - Recovery time: ~30 seconds typical
   - Supports up to 50 failures before spare depletion
   - After spare depletion: continues with reduced ranks

   ## Performance
   - Checkpoint overhead: ~3% of total runtime
   - Memory overhead: ~600 MB per rank for checkpoints

Start Conservatively
~~~~~~~~~~~~~~~~~~~~~

**For first production runs, err on the side of caution:**

- More spares than calculated minimum
- More frequent checkpoints
- Full checkpoints (not partial)
- Higher checkpoint depth
- Verbose logging enabled

**After gaining experience:**

- Tune based on observed failure rates
- Reduce checkpoint frequency if overhead too high
- Experiment with partial checkpoints
- Reduce checkpoint depth if memory constrained

Use Version Control for Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Track Fenix configuration in version control:**

.. code-block:: cpp

   // config.hpp
   namespace FenixConfig {
     constexpr int NUM_SPARES = 50;
     constexpr int CHECKPOINT_INTERVAL = 60;  // iterations
     constexpr int CHECKPOINT_DEPTH = 1;
     constexpr int IMR_MODE = 5;
     constexpr int IMR_LEVEL = 5;
   }

**Rationale:** Allows tracking changes, reverting problematic configurations, and documenting decisions.

Monitor Production Runs
~~~~~~~~~~~~~~~~~~~~~~~~

**Collect metrics from production runs:**

.. code-block:: cpp

   // Log key events
   void log_checkpoint(int timestamp, double time) {
     FILE* log = fopen("fenix_checkpoint.log", "a");
     fprintf(log, "CHECKPOINT timestamp=%d time=%.3f\n", timestamp, time);
     fclose(log);
   }

   void log_recovery(int err, int spares_remaining) {
     FILE* log = fopen("fenix_recovery.log", "a");
     fprintf(log, "RECOVERY error=%d spares=%d\n", err, spares_remaining);
     fclose(log);
   }

**Track:**

- Number of failures per job
- Recovery times
- Checkpoint times
- Spare rank usage
- Memory consumption

**Use to:**

- Tune spare counts
- Adjust checkpoint frequency
- Identify problematic nodes
- Plan for future runs

----

Performance Optimization
------------------------

Profile Before Optimizing
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Measure to find bottlenecks:**

.. code-block:: cpp

   double stage_time = 0, store_time = 0, commit_time = 0;

   auto t1 = MPI_Wtime();
   fenix::data::member_stage(GROUP_ID, MEMBER_ID);
   stage_time = MPI_Wtime() - t1;

   auto t2 = MPI_Wtime();
   fenix::data::member_store(GROUP_ID, MEMBER_ID,
                             fenix::data::SUBSET_PRESTAGED);
   store_time = MPI_Wtime() - t2;

   auto t3 = MPI_Wtime();
   fenix::data::commit_barrier(GROUP_ID);
   commit_time = MPI_Wtime() - t3;

   printf("Stage: %.3fs, Store: %.3fs, Commit: %.3fs\n",
          stage_time, store_time, commit_time);

**Focus optimization efforts on the slowest component.**

Build with Release Mode
~~~~~~~~~~~~~~~~~~~~~~~~

**Always use Release builds for production:**

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=Release ..
   make

**Rationale:** Debug builds include assertions, extra checks, and no optimization. Can be 2-10x slower.

Consider Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~

**For communication-heavy applications, message logging can reduce recovery time:**

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   // Setup
   mlog::create(LOG_ID, res_comm, window_size);
   mlog::activate(LOG_ID);

   // In main loop
   for (int iter = 0; iter < max_iter; iter++) {
     if (iter % 50 == 0) {
       fenix::data::checkpoint(GROUP_ID, fenix::data::SUBSET_FULL);
     }

     mlog::begin_region(LOG_ID, iter);
     // MPI communication here
   }

**Trade-off:** Message logging adds overhead during normal execution but speeds recovery.

**Best for:** Applications where communication is more expensive than computation.

See Also
--------

- :doc:`common-mistakes` - Avoid common pitfalls
- :doc:`troubleshooting` - Fix problems
- :doc:`howto/index` - Task-specific guides
- :doc:`examples/index` - Working examples
