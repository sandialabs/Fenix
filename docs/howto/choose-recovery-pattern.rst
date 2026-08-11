Choose a Recovery Pattern
=========================

When a rank fails, Fenix needs to return control to your application after repairing the communicator. This guide helps you choose the right recovery pattern for your application.

.. contents:: On this page
   :local:
   :depth: 2

Overview
--------

Fenix offers three recovery patterns, controlled by the ``FENIX_RESUME_MODE`` setting:

1. **Inline + Callbacks (RESUME_RETURN/THROW)** - Modern, flexible approach
2. **Longjmp (RESUME_JUMP)** - Legacy restart pattern, default behavior
3. **No Recovery** - Just use communicator repair, handle everything yourself

Decision Matrix
---------------

Use this table to quickly choose the right pattern:

.. list-table::
   :header-rows: 1
   :widths: 20 25 25 30

   * - Pattern
     - Best For
     - Pros
     - Cons
   * - **Inline + Callbacks**
     - C++ applications, modern codebases
     - Clean control flow, RAII-friendly, no UB
     - Requires callback setup
   * - **Longjmp**
     - C applications, simple restart logic
     - Simple setup, automatic restart
     - Undefined behavior risks, no RAII
   * - **No Recovery**
     - Custom recovery, research use
     - Maximum control
     - Must handle everything manually

Recovery Patterns Explained
----------------------------

Pattern 1: Inline + Callbacks (Recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**When to use:**

- Modern C++ applications
- Applications with complex state
- When you want clean, maintainable code
- When using RAII (smart pointers, locks, etc.)

**How it works:**

When a failure is detected, Fenix repairs the communicator, calls your registered callbacks to restore state, and then continues execution inline without jumping. No undefined behavior, no broken RAII.

**Example - C++ with Exceptions:**

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     // Configure inline recovery with exceptions
     MPI_Comm res_comm;
     fenix::init({
       .out_comm = &res_comm,
       .spares = 2,
       .resume_mode = fenix::RESUME_THROW  // Throw on failures
     });

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     // Application state
     struct AppState {
       int iteration = 0;
       double data[1000];
     } state;

     const int GROUP_ID = 0;
     const int STATE_MEMBER = 0;

     // Initialize or recover state
     if (fenix::role() == fenix::INITIAL_RANK) {
       // Initial ranks: create and checkpoint
       state.iteration = 0;
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, STATE_MEMBER,
                          &state, sizeof(state), MPI_BYTE);
       data::member_store(GROUP_ID, STATE_MEMBER, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     } else {
       // Recovered ranks: restore from checkpoint
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, STATE_MEMBER,
                          &state, sizeof(state), MPI_BYTE);
       data::member_restore(GROUP_ID, STATE_MEMBER);
       printf("Rank %d recovered to iteration %d\n",
              rank, state.iteration);
     }

     // Register callback for inline recovery during execution
     fenix::callback_register([&](MPI_Comm repaired, int err) {
       // This callback runs after a failure
       // Restore state from checkpoint
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, STATE_MEMBER,
                          &state, sizeof(state), MPI_BYTE);
       data::member_restore(GROUP_ID, STATE_MEMBER);
       printf("Rank %d continuing inline at iteration %d\n",
              rank, state.iteration);
     });

     // Main application loop
     for (int i = state.iteration; i < 100; i++) {
       try {
         state.iteration = i;

         // Your MPI communication here
         MPI_Allreduce(MPI_IN_PLACE, state.data, 1000,
                      MPI_DOUBLE, MPI_SUM, res_comm);

         // Checkpoint periodically
         if (i % 10 == 0) {
           data::member_store(GROUP_ID, STATE_MEMBER, SUBSET_FULL);
           data::commit_barrier(GROUP_ID);
         }

       } catch (fenix::CommException& e) {
         // A failure occurred and was recovered
         // State is already restored by callback
         // Continue from current iteration
         printf("Recovered from failure, continuing at iteration %d\n", i);
         continue;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Example - C with Return Codes:**

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   typedef struct {
     int iteration;
     double data[1000];
   } AppState;

   AppState state;
   const int GROUP_ID = 0;
   const int STATE_MEMBER = 0;

   void recovery_callback(MPI_Comm repaired, int err, void* data) {
     // Restore state from checkpoint
     Fenix_Data_group_create(GROUP_ID, repaired, 0, 1,
                            FENIX_DATA_POLICY_IN_MEMORY_RAID,
                            (int[]){1, 2}, NULL);
     Fenix_Data_member_restore(GROUP_ID, STATE_MEMBER,
                              &state, sizeof(state),
                              FENIX_DATA_SNAPSHOT_LATEST, NULL);
     printf("Continuing inline at iteration %d\n", state.iteration);
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Set inline recovery mode (return error codes)
     Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_RETURN);

     int role, error;
     MPI_Comm res_comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
               &argc, &argv, 2, &error);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     // Initialize or recover
     if (role == FENIX_ROLE_INITIAL_RANK) {
       state.iteration = 0;
       Fenix_Data_group_create(GROUP_ID, res_comm, 0, 1,
                              FENIX_DATA_POLICY_IN_MEMORY_RAID,
                              (int[]){1, 2}, NULL);
       Fenix_Data_member_create(GROUP_ID, STATE_MEMBER,
                               &state, sizeof(state), MPI_BYTE);
       Fenix_Data_member_store(GROUP_ID, STATE_MEMBER,
                              FENIX_DATA_SUBSET_FULL);
       Fenix_Data_commit_barrier(GROUP_ID, NULL);
     } else {
       // Recovered rank
       Fenix_Data_group_create(GROUP_ID, res_comm, 0, 1,
                              FENIX_DATA_POLICY_IN_MEMORY_RAID,
                              (int[]){1, 2}, NULL);
       Fenix_Data_member_create(GROUP_ID, STATE_MEMBER,
                               &state, sizeof(state), MPI_BYTE);
       Fenix_Data_member_restore(GROUP_ID, STATE_MEMBER,
                                &state, sizeof(state),
                                FENIX_DATA_SNAPSHOT_LATEST, NULL);
     }

     // Register callback
     Fenix_Callback_register(recovery_callback, NULL);

     // Main loop
     for (int i = state.iteration; i < 100; i++) {
       state.iteration = i;

       // MPI operations may return MPI_ERR_PROC_FAILED
       int mpi_ret = MPI_Allreduce(MPI_IN_PLACE, state.data, 1000,
                                   MPI_DOUBLE, MPI_SUM, res_comm);

       if (mpi_ret == MPI_ERR_PROC_FAILED) {
         // Failure detected and recovered, callback already ran
         // Continue from current iteration
         continue;
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

Pattern 2: Longjmp (Legacy)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**When to use:**

- Simple C applications
- Legacy code migration
- When you want automatic restart to initialization point
- When application state is minimal or easily reset

**How it works:**

When a failure occurs, Fenix uses ``setjmp/longjmp`` to jump back to ``Fenix_Init``. All local variables return to their values at initialization (subject to compiler optimizations). Use ``volatile`` for variables you need to preserve across the jump.

**Warnings:**

- Variables may have undefined values after longjmp unless declared ``volatile``
- C++ destructors may not be called when leaving scope via longjmp (undefined behavior)
- RAII objects (smart pointers, locks) will leak
- Not recommended for modern C++ code

**Example:**

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   int main(int argc, char** argv) {
     // Variables that must survive longjmp should be volatile
     volatile int num_failures = 0;

     MPI_Init(&argc, &argv);

     // Default mode is FENIX_RESUME_JUMP
     int role, error;
     MPI_Comm res_comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
               &argc, &argv, 2, &error);

     // Execution resumes here after recovery via longjmp
     int rank;
     MPI_Comm_rank(res_comm, &rank);

     if (role == FENIX_ROLE_INITIAL_RANK) {
       printf("Rank %d starting fresh\n", rank);
     } else {
       num_failures++;
       printf("Rank %d recovered (failure #%d)\n", rank, num_failures);
     }

     // Simple application loop
     // If a failure occurs, we jump back to Fenix_Init
     for (int i = 0; i < 100; i++) {
       double value = i * rank;
       MPI_Allreduce(MPI_IN_PLACE, &value, 1, MPI_DOUBLE,
                    MPI_SUM, res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Key limitations:**

1. All work since initialization is lost on failure
2. Must re-initialize all state after longjmp
3. Cannot use with C++ RAII patterns
4. Variables need ``volatile`` qualifier to be reliable

Pattern 3: No Recovery (Manual)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**When to use:**

- Research or experimental use cases
- Custom recovery mechanisms
- When you need complete control
- Testing or debugging scenarios

**How it works:**

Set recovery mode to ``FENIX_RECOVERY_IGNORE`` or ``FENIX_RECOVERY_NOOP``. Fenix will not repair communicators automatically. You must handle all recovery yourself.

**Example:**

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Disable automatic recovery
     Fenix_set_option(FENIX_RECOVERY_MODE, FENIX_RECOVERY_IGNORE);
     Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_RETURN);

     int role, error;
     MPI_Comm comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &comm,
               &argc, &argv, 0, &error);

     // You must handle all MPI errors yourself
     int ret = MPI_Allreduce(/* ... */, comm);
     if (ret == MPI_ERR_PROC_FAILED) {
       // Handle failure manually
       // Maybe shrink communicator, revoke, etc.
       MPIX_Comm_shrink(comm, &comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Common Scenarios
----------------

Scenario: Iterative Solver with Checkpoints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best choice:** Inline + Callbacks

**Why:** You need to resume from the last checkpoint, not restart from scratch. Inline recovery lets you continue from the checkpoint without re-running completed iterations.

Scenario: Simple Embarrassingly Parallel Computation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best choice:** Longjmp

**Why:** If each rank can just restart its work independently, longjmp is simpler. No complex state to restore.

Scenario: Complex C++ Application with Smart Pointers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best choice:** Inline + Exceptions (RESUME_THROW)

**Why:** Longjmp will leak your RAII objects. Use exceptions to get proper cleanup and recovery.

Scenario: Legacy Fortran/C Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best choice:** Longjmp initially, then migrate to Inline

**Why:** Longjmp requires minimal code changes for initial integration. Add callbacks later for better performance.

Migration Path
--------------

If you're currently using longjmp and want to migrate to inline recovery:

**Step 1: Add resume mode setting**

.. code-block:: cpp

   fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

**Step 2: Wrap MPI operations in try-catch**

.. code-block:: cpp

   try {
     MPI_Allreduce(/* ... */, res_comm);
   } catch (fenix::CommException& e) {
     // Handle recovery
   }

**Step 3: Register recovery callback**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     // Restore state here
   });

**Step 4: Remove volatile qualifiers**

No longer needed since we're not using longjmp.

Troubleshooting
---------------

**Problem: Variables have wrong values after recovery**

- If using longjmp: Add ``volatile`` qualifier
- If using inline: Check that your callback restores all state

**Problem: Memory leaks after recovery**

- Longjmp doesn't call destructors - switch to inline recovery
- Or manually free resources before ``Fenix_Init``

**Problem: Segfault after recovery**

- Check that pointers in checkpointed data are still valid
- Ensure callbacks recreate all necessary state
- Verify that you're not accessing freed memory

**Problem: Recovery is too slow**

- Reduce checkpoint frequency
- Use partial checkpoints (subsets)
- Consider message logging for localized recovery

See Also
--------

- :doc:`test-locally` - How to test your recovery pattern
- :doc:`checkpoint-data` - How to checkpoint application state
- :doc:`/api/process-recovery` - API reference for recovery functions
- :doc:`/guides/process-recovery` - Conceptual guide to process recovery
- :doc:`/troubleshooting` - Common problems and solutions
