Inline Recovery Pattern
========================

The inline recovery pattern (FENIX_RESUME_RETURN) returns error codes from MPI operations, allowing clean continuation without stack unwinding.

.. _inline-recovery:

Call Stack Behavior
-------------------

.. graphviz::
   :caption: Inline Recovery - Stack Preserved

   digraph inline_stack {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       normal [label="Execution Stack:\nmain() → loop() → MPI_Allreduce()", fillcolor=lightgreen];

       error [label="Error handler:\n• Revoke comm\n• Repair comm\n• Call callbacks\n• Return error code", fillcolor=orange];

       return [label="Stack Preserved:\nReturn to caller\nwith MPI_ERR_PROC_FAILED", fillcolor=lightgreen];

       continue [label="Continue execution\nfrom same point", fillcolor=lightgreen];

       normal -> error [label="Failure"];
       error -> return [label="Return"];
       return -> continue;
   }

Key Characteristics
-------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Aspect
     - Behavior
   * - **Stack unwinding**
     - ✅ None - stack preserved
   * - **Destructors**
     - ✅ Called normally
   * - **Local variables**
     - ✅ Preserved
   * - **Resource cleanup**
     - ✅ Automatic (RAII works)
   * - **RAII compatibility**
     - ✅ Yes
   * - **Code changes**
     - ⚠️ Moderate (check return codes)
   * - **Performance**
     - ✅ Best (minimal work lost)

Code Example with Callbacks
----------------------------

.. code-block:: c

   typedef struct {
     int iteration;
     double data[1000];
   } AppState;

   AppState state;

   void recovery_callback(MPI_Comm repaired, int err, void* user_data) {
     printf("Callback invoked! Restoring state...\n");

     // Recreate data group
     Fenix_Data_group_create(GROUP_ID, repaired, ...);

     // Restore from checkpoint
     Fenix_Data_member_restore(GROUP_ID, STATE_MEMBER,
                              &state, sizeof(state),
                              FENIX_DATA_SNAPSHOT_LATEST, NULL);

     printf("Restored to iteration %d\n", state.iteration);
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Configure inline recovery
     Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_RETURN);

     int role, error;
     MPI_Comm comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &comm, &argc, &argv, 2, &error);

     // Register callback
     Fenix_Callback_register(recovery_callback, NULL);

     // Initial setup or recovery
     if (role == FENIX_ROLE_INITIAL_RANK) {
       state.iteration = 0;
       // Create data group and checkpoint...
     } else if (role == FENIX_ROLE_RECOVERED_RANK) {
       // Restore handled by callback
     }

     // Application loop
     for (int i = state.iteration; i < 100; i++) {
       state.iteration = i;

       // Work...

       // MPI operation - check return code
       int ret = MPI_Allreduce(MPI_IN_PLACE, state.data, 1000,
                              MPI_DOUBLE, MPI_SUM, comm);

       if (ret == MPI_ERR_PROC_FAILED) {
         // Recovery happened! Callback already ran
         printf("Recovered at iteration %d\n", state.iteration);
         // Continue from current iteration
         continue;
       } else if (ret != MPI_SUCCESS) {
         fprintf(stderr, "MPI error: %d\n", ret);
         break;
       }

       // Checkpoint periodically
       if (i % 10 == 0) {
         Fenix_Data_member_store(GROUP_ID, STATE_MEMBER, 
                                FENIX_DATA_SUBSET_FULL);
         Fenix_Data_commit_barrier(GROUP_ID, NULL);
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Advantages
----------

✅ **Benefits:**

- Stack and local variables preserved
- RAII cleanup works correctly
- Minimal work lost (continue from checkpoint)
- Fine-grained error handling
- Works with both C and C++
- Best performance

When to Use
-----------

✅ **Recommended for:**

- Modern C applications
- C++ applications (if not using exceptions)
- Applications with complex state
- Production systems
- When checkpoint cost is high

.. seealso::

   * :doc:`04-exception-recovery` - C++ exception alternative
   * :doc:`02-longjmp-recovery` - Legacy alternative
   * :doc:`12-decision-recovery-pattern` - Pattern comparison
