detect_failures
===============

.. operation:: collective

Explicitly check for process failures and optionally trigger recovery.

This function proactively checks for failed ranks and can initiate the recovery protocol.
It's essential for applications with long compute phases that don't make frequent MPI calls,
as Fenix normally detects failures only when MPI operations fail.

.. c:function:: int Fenix_Process_detect_failures(int do_recovery)

   :param int do_recovery: [in] If non-zero, perform recovery if failures are detected. If zero, only detect without recovering.
   :returns: Return code indicating whether failures were found and/or recovered

.. cpp:function:: int fenix::detect_failures(bool recover = true)

   :param bool recover: [in] If true, perform recovery if failures are detected (default: true)
   :returns: Return code indicating status

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - No failures detected
- :c:enumerator:`FENIX_WARNING_SPARE_RANKS_DEPLETED` - Failures detected and recovered, but spares were depleted
- :c:enumerator:`FENIX_ERROR_PROCESS_FAILURE` - Failures detected but not recovered (when do_recovery=0)

**When to Use:**

This function is critical in scenarios where ranks may fail but your application code
doesn't call MPI frequently enough to detect it promptly:

1. **Long compute phases**: If you have computation that runs for seconds/minutes without MPI calls
2. **Independent computation**: When ranks work independently between synchronization points
3. **Periodic health checks**: To ensure failed ranks are detected and replaced promptly
4. **Controlled recovery timing**: To trigger recovery at specific safe points in your algorithm

**Usage Examples:**

.. code-block:: c

   // C example - Periodic failure detection in compute loop
   int main(int argc, char** argv) {
       Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

       for (int iter = 0; iter < max_iterations; iter++) {
           // Long computation phase (no MPI calls)
           for (int i = 0; i < 1000000; i++) {
               local_data[i] = expensive_computation(i);
           }

           // Periodically check for failures every 10 iterations
           if (iter % 10 == 0) {
               int ret = Fenix_Process_detect_failures(1);
               if (ret == FENIX_WARNING_SPARE_RANKS_DEPLETED) {
                   fprintf(stderr, "Iter %d: Recovered from failure, spares depleted\n", iter);
                   // May want to adjust algorithm or checkpoint more frequently
               } else if (ret != FENIX_SUCCESS) {
                   fprintf(stderr, "Iter %d: Failure detection failed: %d\n", iter, ret);
               }
           }

           // Synchronization point - would detect failures anyway
           MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, fenix_comm);
       }

       Fenix_Finalize();
       return 0;
   }

.. code-block:: cpp

   // C++ example - Check without recovery
   for (int iter = 0; iter < max_iterations; iter++) {
       // Heavy computation
       compute(data);

       // Check if failures exist (don't recover yet)
       int status = fenix::detect_failures(false);
       if (status == FENIX_ERROR_PROCESS_FAILURE) {
           std::cout << "Failure detected at iter " << iter << "\n";

           // Finish current computation phase, checkpoint, then recover
           checkpoint_data();

           // Now trigger recovery
           fenix::detect_failures(true);
       }
   }

**Detection Without Recovery:**

You can detect failures without immediately recovering by passing ``do_recovery=0``:

.. code-block:: c

   // Detect but don't recover yet
   int ret = Fenix_Process_detect_failures(0);
   if (ret == FENIX_ERROR_PROCESS_FAILURE) {
       printf("Failures exist but not yet recovered\n");

       // Do any necessary preparation
       save_intermediate_state();

       // Now recover
       Fenix_Process_detect_failures(1);
   }

This can be useful to:

- Complete in-flight operations before recovery disrupts the communicator
- Create a checkpoint at a safe point before recovering
- Log or report the failure before recovery changes rank roles

**Integration with Callbacks:**

Detection triggers the same recovery callbacks as automatic failure detection:

.. code-block:: c

   void pre_recovery_callback(MPI_Comm comm, int error, void* data) {
       printf("About to recover from failure\n");
       // Prepare for recovery
   }

   void post_recovery_callback(MPI_Comm comm, int error, void* data) {
       printf("Recovery complete\n");
       // Restore state, etc.
   }

   // Register callbacks
   Fenix_Callback_register(pre_recovery_callback, NULL);
   Fenix_Callback_register(post_recovery_callback, NULL);

   // Detect failures - callbacks will be invoked if failures found
   Fenix_Process_detect_failures(1);

**Performance Considerations:**

- Detection is lightweight but not free - it checks communicator health
- For applications with frequent MPI calls, automatic detection is sufficient
- Balance detection frequency against overhead (e.g., every N iterations)
- More frequent detection reduces time between failure and recovery
- Less frequent detection reduces overhead but increases recovery latency

**Common Pitfalls:**

- **Not calling collectively**: All ranks must call this function together.
- **Over-checking**: Calling on every iteration when you have frequent MPI calls is wasteful.
- **Under-checking**: Not calling often enough in compute-heavy phases can delay recovery for minutes.
- **Ignoring return codes**: Always check the return value to know if recovery occurred.
- **Checking after MPI calls**: If you call MPI operations frequently, they already detect failures automatically.

**Comparison: Automatic vs Manual Detection:**

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - Automatic Detection
     - Manual Detection (this function)
   * - Trigger
     - MPI operation fails
     - Explicit function call
   * - Latency
     - Depends on MPI call frequency
     - Controlled by application
   * - Use case
     - Frequent MPI calls
     - Long compute phases
   * - Overhead
     - No extra overhead
     - Cost of periodic checks
   * - Control
     - Automatic, no control
     - App controls when to check

**Best Practices:**

1. Use in compute-heavy loops: Call periodically during long computation phases
2. Balance frequency: Check often enough to bound recovery latency, but not so often you add overhead
3. Check before checkpoints: Detect failures before creating checkpoints for cleaner state
4. Log detection: When failures are detected, log the iteration/time for debugging
5. Monitor spare depletion: If spares are depleted, consider more aggressive checkpointing

.. seealso::
   :c:func:`Fenix_Init`, :c:func:`Fenix_Callback_register`, :c:func:`Fenix_get_role`, :doc:`/guides/process-recovery`
