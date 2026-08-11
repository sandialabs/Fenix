Tutorial 3: Inline Recovery and Callbacks
==========================================

**Time:** 30-40 minutes | **Difficulty:** Intermediate

**Prerequisites:** :doc:`01-first-program`, :doc:`02-data-recovery`

In the previous tutorials, you learned how Fenix detects failures and repairs communicators, and how to checkpoint/restore application data. However, the exception-based recovery pattern we've used so far has a limitation: when a failure occurs, execution jumps back to the beginning of your work loop, losing any partial progress since the last checkpoint.

**Inline recovery** solves this problem by allowing your application to continue execution exactly where it left off after a failure, without restarting loops or losing local state. This tutorial will teach you how to use inline recovery with callbacks for seamless, transparent fault tolerance.

.. contents:: In This Tutorial
   :local:
   :depth: 2

Learning Objectives
-------------------

By completing this tutorial, you will:

✓ Understand the difference between exception-based and inline recovery

✓ Configure Fenix for inline recovery with callbacks

✓ Register recovery callbacks that execute during failure handling

✓ Maintain application state across inline recoveries

✓ Build a complete stencil computation with transparent fault tolerance

✓ Know when to use inline recovery vs. exception-based recovery

✓ Test and verify inline recovery behavior

Why Inline Recovery?
--------------------

The Problem with Exception-Based Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In :doc:`01-first-program`, you learned about exception-based recovery:

.. code-block:: cpp

   while (keep_running) {
     try {
       for (int iter = 0; iter < 100; iter++) {
         // Do work...
         MPI_Barrier(res_comm);
       }
       keep_running = false;
     } catch (fenix::CommException& e) {
       // Failure occurred - loop restarts from iteration 0
       res_comm = e.repaired_comm;
     }
   }

**Problem:** When a failure occurs at iteration 50, the catch block is executed and the entire for loop restarts from iteration 0, even though survivors have already completed iterations 0-50.

The Inline Recovery Solution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

With inline recovery, Fenix handles failures transparently in the background:

.. code-block:: cpp

   // Register callback to restore state on failure
   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     // Recreate group and define member
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, sizeof(state), MPI_BYTE);

     // Only recovered ranks restore
     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, STATE_ID);
     }
   });

   // Application code continues naturally - no try/catch needed!
   for (int iter = 0; iter < 100; iter++) {
     // Do work...
     MPI_Barrier(res_comm);  // Failures handled transparently
   }

**Benefits:**

1. **No control flow changes**: Your loop continues naturally without restarting
2. **Local state preserved**: Loop variables, stack state, everything remains intact
3. **Cleaner code**: No try/catch blocks cluttering your application logic
4. **Better performance**: No unnecessary re-execution of work

When to Use Each Approach
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - Exception-Based Recovery
     - Inline Recovery
   * - **Use When**
     - Need explicit control flow
     - Want transparent recovery
   * - **Code Pattern**
     - try/catch blocks
     - Callbacks
   * - **Loop Restart**
     - Yes (entire loop restarts)
     - No (continues inline)
   * - **Local State**
     - Lost (unless saved)
     - Preserved
   * - **Best For**
     - Simple applications, learning
     - Production apps, complex state
   * - **Code Complexity**
     - Moderate
     - Lower

Part 1: Setting Up Inline Recovery (10 minutes)
------------------------------------------------

Let's start by converting an exception-based recovery program to use inline recovery.

Step 1.1: Configure Resume Mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The key difference is configuring Fenix to use ``RETURN`` mode instead of ``THROW`` mode:

.. code-block:: cpp
   :emphasize-lines: 8-9

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     // Configure inline recovery (no exceptions)
     fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);
     fenix::set_option(fenix::RECOVERY_MODE, fenix::REPAIR);

     // ... rest of application
   }

**What changed:**

- ``RESUME_MODE = RETURN``: When a failure occurs, MPI operations return normally (rather than throwing exceptions)
- The communicator is repaired in the background
- Your application continues execution from exactly where it was

.. important::
   **RETURN vs THROW vs JUMP**

   - ``THROW``: Throws C++ exception (:doc:`01-first-program` approach)
   - ``RETURN``: Returns from MPI call normally, continues inline (this tutorial)
   - ``JUMP``: Uses longjmp (old C API, not recommended)

Step 1.2: Understanding Callbacks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

With inline recovery, you use **callbacks** to handle recovery tasks. A callback is a function that Fenix calls automatically when a failure occurs:

.. code-block:: cpp

   // Register a callback function
   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     printf("Rank %d: Failure detected and repaired!\n", rank);
     // Add recovery logic here
   });

**Callback Parameters:**

- ``repaired_comm``: The new, repaired communicator
- ``mpi_err``: The MPI error code that triggered the failure

**When is it called?**

The callback executes automatically when:

1. An MPI operation detects a failure
2. Fenix repairs the communicator
3. Before the MPI operation returns to your code

Step 1.3: Basic Inline Recovery Example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here's a complete minimal example:

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <stdio.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Enable inline recovery
     fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);
     fenix::set_option(fenix::RECOVERY_MODE, fenix::REPAIR);

     // Register callback for failures
     int failure_count = 0;
     fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
       failure_count++;
       printf("Rank %d: Recovered from failure #%d\n", rank, failure_count);
     });

     // Application work - failures handled transparently
     for (int i = 0; i < 10; i++) {
       printf("Rank %d: iteration %d\n", rank, i);
       MPI_Barrier(res_comm);
     }

     printf("Rank %d: Completed with %d failures encountered\n",
            rank, failure_count);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Key Points:**

- No try/catch blocks needed
- The for loop continues naturally through failures
- The callback tracks how many failures occurred
- Lambda captures ``[&]`` allow accessing local variables

Part 2: Data Recovery with Callbacks (10 minutes)
--------------------------------------------------

Inline recovery really shines when combined with data recovery. Let's build an example that checkpoints state and restores it inline.

Step 2.1: Set Up Data Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

First, create data groups and members as in :doc:`02-data-recovery`:

.. code-block:: cpp
   :linenos:

   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;
   constexpr int DATA_ID = 1;

   struct AppState {
     int rank;
     int iteration;
     double sum;
   };

   int main(int argc, char** argv) {
     // ... initialization code ...

     AppState state{rank, 0, 0.0};
     std::vector<double> data(100, static_cast<double>(rank));

     // Create data group
     fenix::data::group_create(GROUP_ID);

     if (fenix::role() == fenix::INITIAL_RANK) {
       // Initial ranks create members
       fenix::data::member_create(GROUP_ID, STATE_ID, &state, 3, MPI_DOUBLE);
       fenix::data::member_create(GROUP_ID, DATA_ID,
                                  data.data(), data.size(), MPI_DOUBLE);
     } else {
       // Recovered ranks define members
       fenix::data::member_define(GROUP_ID, STATE_ID, &state, 3, MPI_DOUBLE);
       fenix::data::member_define(GROUP_ID, DATA_ID,
                                  data.data(), data.size(), MPI_DOUBLE);
       // Restore data immediately
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, DATA_ID);
     }

Step 2.2: Register Recovery Callback
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now register a callback that restores data when failures occur:

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 3-12

   // Register callback to restore data on inline recovery
   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     printf("Rank %d: Inline recovery callback triggered\n", rank);

     // Recreate data group (always needed after recovery)
     fenix::data::group_create(GROUP_ID);

     // Define members with buffer pointers
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, 3, MPI_DOUBLE);
     fenix::data::member_define(GROUP_ID, DATA_ID,
                                data.data(), data.size(), MPI_DOUBLE);

     // Only recovered ranks need data restoration
     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, DATA_ID);
       printf("Rank %d: Restored to iteration %d\n", rank, state.iteration);
     } else {
       printf("Rank %d: Survivor, continuing with current data\n", rank);
     }
   });

**Why recreate the group and define members?**

After a failure, internal Fenix state is reset. Always recreate data groups and redefine members in callbacks with their buffer pointers, even though you created them earlier.

**Why check for recovered rank?**

The callback executes on **all** ranks (survivors and recovered). Survivors already have valid data in their buffers - calling ``member_restore()`` would overwrite it with stale checkpoint data. Only recovered ranks need restoration since they're starting fresh.

Step 2.3: Checkpoint During Execution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add checkpointing during your application work:

.. code-block:: cpp
   :linenos:

   for (int iter = state.iteration; iter < 100; iter++) {
     state.iteration = iter;

     // Do computation
     for (size_t i = 0; i < data.size(); i++) {
       data[i] = data[i] * 1.01 + rank;
     }
     state.sum += data[0];  // Track running sum

     // Checkpoint every 10 iterations
     if ((iter + 1) % 10 == 0) {
       fenix::data::member_store(GROUP_ID);
       fenix::data::commit_barrier(GROUP_ID);
     }

     // Synchronize
     MPI_Barrier(res_comm);
   }

**Important:** With inline recovery, failures during ``member_store()`` or ``commit_barrier()`` are handled transparently. The operations return normally, and your callback restores the last good checkpoint.

Part 3: Complete Stencil Computation Example (15 minutes)
----------------------------------------------------------

Now let's build a realistic stencil computation that demonstrates inline recovery in a real-world scenario. This pattern is common in scientific simulations.

The Application: 1D Stencil
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We'll simulate a 1D stencil computation where each rank:

1. Owns a section of a 1D array
2. Exchanges boundary values with neighbors
3. Updates interior points based on neighbors
4. Checks for convergence

This mimics heat diffusion, wave propagation, and many other scientific applications.

Complete Implementation
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <cmath>
   #include <stdio.h>
   #include <signal.h>

   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;
   constexpr int GRID_ID = 1;

   constexpr int GRID_SIZE = 1000;
   constexpr int MAX_ITERS = 100;
   constexpr int CHECKPOINT_FREQ = 10;

   struct State {
     int rank;
     int iteration;
     double max_diff;
   };

   // Inject failure for testing
   void check_failure(int rank, int iter) {
     if (rank == 1 && iter == 25) {
       printf("Rank %d: Injecting failure at iteration %d\n", rank, iter);
       raise(SIGKILL);
     }
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix with 2 spares
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Neighbor ranks (periodic boundary)
     int left = (rank - 1 + size) % size;
     int right = (rank + 1) % size;

     // Enable inline recovery
     fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);
     fenix::set_option(fenix::RECOVERY_MODE, fenix::REPAIR);

     // Application state and grid
     State state{rank, 0, 1.0};
     std::vector<double> grid(GRID_SIZE);
     std::vector<double> grid_new(GRID_SIZE);

     // Create data group
     fenix::data::group_create(GROUP_ID);

     if (fenix::role() == fenix::INITIAL_RANK) {
       printf("Rank %d: Initial rank starting\n", rank);

       // Initialize grid with rank-based values
       for (int i = 0; i < GRID_SIZE; i++) {
         grid[i] = static_cast<double>(rank * GRID_SIZE + i);
       }

       // Create data members
       fenix::data::member_create(GROUP_ID, STATE_ID, &state, 3, MPI_DOUBLE);
       fenix::data::member_create(GROUP_ID, GRID_ID,
                                  grid.data(), GRID_SIZE, MPI_DOUBLE);

       // Initial checkpoint
       fenix::data::member_store(GROUP_ID);
       fenix::data::commit_barrier(GROUP_ID);

     } else {
       printf("Rank %d: Recovered rank restoring data\n", rank);

       // Define members (idempotent)
       fenix::data::member_define(GROUP_ID, STATE_ID, &state, 3, MPI_DOUBLE);
       fenix::data::member_define(GROUP_ID, GRID_ID,
                                  grid.data(), GRID_SIZE, MPI_DOUBLE);

       // Restore from checkpoint
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, GRID_ID);

       printf("Rank %d: Restored to iteration %d\n", rank, state.iteration);
     }

     // Register inline recovery callback
     fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
       printf("Rank %d: Inline recovery at iteration %d\n",
              rank, state.iteration);

       // Recreate data group
       fenix::data::group_create(GROUP_ID);

       // Define members with buffer pointers
       fenix::data::member_define(GROUP_ID, STATE_ID, &state, 3, MPI_DOUBLE);
       fenix::data::member_define(GROUP_ID, GRID_ID,
                                  grid.data(), GRID_SIZE, MPI_DOUBLE);

       // Only recovered ranks need data restoration
       if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
         fenix::data::member_restore(GROUP_ID, STATE_ID);
         fenix::data::member_restore(GROUP_ID, GRID_ID);
         printf("Rank %d: Restored and continuing from iteration %d\n",
                rank, state.iteration);
       } else {
         printf("Rank %d: Survivor, continuing from iteration %d\n",
                rank, state.iteration);
       }
     });

     // Main stencil computation loop
     for (int iter = state.iteration; iter < MAX_ITERS; iter++) {
       check_failure(rank, iter);
       state.iteration = iter;

       // Exchange boundary values with neighbors
       double left_boundary = grid[0];
       double right_boundary = grid[GRID_SIZE - 1];
       double left_ghost, right_ghost;

       MPI_Sendrecv(&right_boundary, 1, MPI_DOUBLE, right, 0,
                    &left_ghost, 1, MPI_DOUBLE, left, 0,
                    res_comm, MPI_STATUS_IGNORE);

       MPI_Sendrecv(&left_boundary, 1, MPI_DOUBLE, left, 1,
                    &right_ghost, 1, MPI_DOUBLE, right, 1,
                    res_comm, MPI_STATUS_IGNORE);

       // Update interior points using 3-point stencil
       grid_new[0] = 0.25 * (left_ghost + 2.0 * grid[0] + grid[1]);
       for (int i = 1; i < GRID_SIZE - 1; i++) {
         grid_new[i] = 0.25 * (grid[i-1] + 2.0 * grid[i] + grid[i+1]);
       }
       grid_new[GRID_SIZE-1] = 0.25 * (grid[GRID_SIZE-2] +
                                       2.0 * grid[GRID_SIZE-1] + right_ghost);

       // Compute maximum difference (convergence check)
       state.max_diff = 0.0;
       for (int i = 0; i < GRID_SIZE; i++) {
         double diff = std::abs(grid_new[i] - grid[i]);
         if (diff > state.max_diff) state.max_diff = diff;
         grid[i] = grid_new[i];
       }

       // Global convergence check every 5 iterations
       if (iter % 5 == 0) {
         double global_max_diff;
         MPI_Allreduce(&state.max_diff, &global_max_diff, 1,
                       MPI_DOUBLE, MPI_MAX, res_comm);

         if (rank == 0) {
           printf("Iteration %d: max_diff = %e\n", iter, global_max_diff);
         }
       }

       // Checkpoint periodically
       if ((iter + 1) % CHECKPOINT_FREQ == 0) {
         fenix::data::member_store(GROUP_ID);
         fenix::data::commit_barrier(GROUP_ID);

         if (rank == 0) {
           printf("Checkpoint at iteration %d\n", iter + 1);
         }
       }
     }

     printf("Rank %d: Completed %d iterations\n", rank, MAX_ITERS);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Understanding the Example
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Key Components:**

1. **Lines 47-48**: Configure inline recovery mode
2. **Lines 88-101**: Recovery callback that restores state transparently
3. **Lines 104-157**: Main computation loop - no try/catch needed!
4. **Lines 112-122**: MPI communication continues normally through failures
5. **Lines 148-154**: Checkpointing happens inline, failures handled automatically

**What Happens During a Failure:**

1. Rank 1 dies at iteration 25 (line 24)
2. Surviving ranks detect failure at next MPI call (line 112)
3. Fenix repairs communicator automatically
4. Callback (line 88) restores data to iteration 20 (last checkpoint)
5. Execution continues from iteration 20 without loop restart
6. All ranks remain synchronized

Building and Running
^^^^^^^^^^^^^^^^^^^^

Compile the example:

.. code-block:: bash

   mpicxx -std=c++17 stencil_inline.cpp -o stencil_inline \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix

Run with 5 total ranks (3 active + 2 spares):

.. code-block:: bash

   mpiexec --with-ft mpi -n 5 ./stencil_inline

**Expected Output:**

.. code-block:: text

   Rank 0: Initial rank starting
   Rank 1: Initial rank starting
   Rank 2: Initial rank starting
   Iteration 0: max_diff = 2.500000e+02
   Checkpoint at iteration 10
   Checkpoint at iteration 20
   Rank 1: Injecting failure at iteration 25
   Rank 0: Inline recovery at iteration 25
   Rank 0: Continuing from iteration 20
   Rank 2: Inline recovery at iteration 25
   Rank 2: Continuing from iteration 20
   Rank 1: Recovered rank restoring data
   Rank 1: Restored to iteration 20
   Checkpoint at iteration 30
   Iteration 30: max_diff = 1.234567e+01
   ...
   Rank 0: Completed 100 iterations
   Rank 1: Completed 100 iterations
   Rank 2: Completed 100 iterations

Notice that:

- Survivors continue from iteration 25 (where they were)
- Recovered rank restores to iteration 20 (last checkpoint)
- The loop doesn't restart - execution continues inline

Part 4: Advanced Callback Patterns (5 minutes)
-----------------------------------------------

Multiple Callbacks
^^^^^^^^^^^^^^^^^^

You can register multiple callbacks that execute in order:

.. code-block:: cpp

   // First callback: restore core state
   fenix::callback_register([&](MPI_Comm comm, int err) {
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, sizeof(state), MPI_BYTE);

     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, STATE_ID);
     }
   });

   // Second callback: rebuild derived data
   fenix::callback_register([&](MPI_Comm comm, int err) {
     rebuild_lookup_tables();
     recompute_statistics();
   });

Callbacks execute in registration order on all surviving ranks.

Callback Error Handling
^^^^^^^^^^^^^^^^^^^^^^^^

By default, exceptions in callbacks are caught and logged. You can change this:

.. code-block:: cpp

   // Let callback exceptions propagate
   fenix::set_option(fenix::CALLBACK_EXCEPTION_MODE, fenix::PROPAGATE);

   fenix::callback_register([&](MPI_Comm comm, int err) {
     if (!restore_succeeded()) {
       throw std::runtime_error("Recovery failed!");
     }
   });

Conditional Recovery Logic
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use the callback parameters to make decisions:

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     // Recreate data group
     fenix::data::group_create(GROUP_ID);

     // Define members
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, sizeof(state), MPI_BYTE);
     fenix::data::member_define(GROUP_ID, DATA_ID, data.data(), data.size(), MPI_DOUBLE);

     // Only recovered ranks need restoration
     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       // Check which ranks failed
       auto failed_ranks = fenix::fail_list();

       if (failed_ranks.size() > 1) {
         // Multiple failures - full restore
         restore_all_data();
       } else {
         // Single failure - minimal restore
         restore_critical_data_only();
       }
     }
   });

Testing Recovery Paths (5 minutes)
-----------------------------------

Systematic Testing
^^^^^^^^^^^^^^^^^^

Test different failure scenarios:

.. code-block:: cpp

   // Test 1: Early failure
   if (rank == 1 && iter == 5) raise(SIGKILL);

   // Test 2: Mid-computation failure
   if (rank == 2 && iter == 50) raise(SIGKILL);

   // Test 3: Just before checkpoint
   if (rank == 0 && iter == 29) raise(SIGKILL);

   // Test 4: During checkpoint
   if (rank == 1 && iter == 30 && inside_checkpoint) raise(SIGKILL);

Verification
^^^^^^^^^^^^

Add validation to ensure recovery correctness:

.. code-block:: cpp

   // Compute checksum before checkpoint
   double checksum = 0.0;
   for (double val : data) checksum += val;

   // After recovery, verify
   fenix::callback_register([&](MPI_Comm comm, int err) {
     // Recreate data group (always needed after recovery)
     fenix::data::group_create(GROUP_ID);

     // Define member with buffer pointer
     fenix::data::member_define(GROUP_ID, DATA_ID, data.data(), data.size(), MPI_DOUBLE);

     // Only recovered ranks need data restoration
     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, DATA_ID);

       // Recompute checksum
       double new_checksum = 0.0;
       for (double val : data) new_checksum += val;

       // Should match (within floating point tolerance)
       assert(std::abs(checksum - new_checksum) < 1e-9);
     }
   });

Best Practices
--------------

When to Use Inline Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

✓ **Use inline recovery when:**

- Application has complex nested loops or state machines
- You want minimal code changes to existing applications
- Performance is critical (no loop restarts)
- You have good checkpointing infrastructure

✗ **Don't use inline recovery when:**

- Your application is simple and exception-based recovery is clearer
- You need explicit control flow after failures
- You're just learning Fenix (start with exceptions)

Callback Design Guidelines
^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Keep callbacks fast**: They run on the critical path
2. **Always recreate data groups**: Fenix resets internal state
3. **Always redefine members**: Bind buffer pointers after recreating groups
4. **Check rank role**: Only restored ranks should call member_restore()
5. **Survivors have valid data**: Don't overwrite survivor data with stale checkpoints
6. **Test callback code thoroughly**: Hard to debug during failures
7. **Avoid complex logic**: Simple restore operations are best

Common Pitfalls
^^^^^^^^^^^^^^^

**Pitfall 1: Forgetting to recreate groups**

.. code-block:: cpp

   // WRONG: Don't reuse existing group
   fenix::callback_register([&](MPI_Comm comm, int err) {
     fenix::data::member_restore(GROUP_ID, STATE_ID);  // May fail!
   });

   // RIGHT: Always recreate and define, only restore on recovered ranks
   fenix::callback_register([&](MPI_Comm comm, int err) {
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, sizeof(state), MPI_BYTE);

     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, STATE_ID);
     }
   });

**Pitfall 2: Restoring on all ranks unconditionally**

.. code-block:: cpp

   // WRONG: Overwrites valid survivor data!
   fenix::callback_register([&](MPI_Comm comm, int err) {
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_restore(GROUP_ID, STATE_ID);  // All ranks restore!
   });

   // RIGHT: Only recovered ranks restore
   fenix::callback_register([&](MPI_Comm comm, int err) {
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, sizeof(state), MPI_BYTE);

     if (fenix::get_role() == FENIX_ROLE_RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, STATE_ID);  // Only recovered!
     }
   });

**Critical:** Callbacks execute on **all** ranks. Survivors have valid current data; restoring would overwrite it with stale checkpoint data.

**Pitfall 3: Not handling communicator updates**

.. code-block:: cpp

   // WRONG: Using old communicator
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 1});

   fenix::callback_register([&](MPI_Comm repaired_comm, int err) {
     MPI_Barrier(res_comm);  // Old comm may be invalid!
   });

   // RIGHT: Use repaired communicator or get new one
   fenix::callback_register([&](MPI_Comm repaired_comm, int err) {
     res_comm = repaired_comm;  // Update reference
     MPI_Barrier(res_comm);
   });

**Pitfall 4: Callback exceptions**

Unhandled exceptions in callbacks are silently caught by default. Enable propagation for debugging:

.. code-block:: cpp

   fenix::set_option(fenix::CALLBACK_EXCEPTION_MODE, fenix::PROPAGATE);

Exercises
---------

Exercise 1: Add Multiple Callbacks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Modify the stencil example to use two callbacks:

1. First callback restores state
2. Second callback rebuilds a derived data structure (e.g., checksum)

Verify they execute in order.

Exercise 2: Measure Overhead
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Compare execution time with and without failures:

1. Run stencil without failures, measure time
2. Inject 1 failure, measure time
3. Inject 3 failures, measure time
4. Calculate overhead percentage

Exercise 3: Implement Adaptive Checkpointing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Create a callback that tracks failure frequency and adjusts checkpoint frequency:

- If failures are frequent, checkpoint more often
- If failures are rare, checkpoint less often

Exercise 4: Mixed Recovery Modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Combine inline recovery for most of the application with exception-based recovery for specific critical sections. Hint: You can change ``RESUME_MODE`` dynamically.

Exercise 5: Production-Ready Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add comprehensive validation to the stencil example:

1. Verify grid data after recovery
2. Check convergence metrics are consistent
3. Validate neighbor communication produces correct ghost cells
4. Log all recovery events to a file

Next Steps
----------

Congratulations! You now understand inline recovery and callbacks. Continue your learning:

📚 **Next Tutorial:**

- :doc:`04-message-logging` - Add automatic message replay for seamless recovery

🔗 **Related How-To Guides:**

- :doc:`/howto/choose-recovery-pattern` - Choosing between recovery modes
- :doc:`/howto/debug-fenix-app` - Debugging recovery callbacks
- :doc:`/howto/checkpoint-data` - Advanced checkpointing strategies

📖 **API Reference:**

- :cpp:func:`fenix::callback_register` - Register recovery callbacks
- :cpp:func:`fenix::set_option` - Configure recovery modes
- :cpp:func:`fenix::fail_list` - Query failed ranks

🔬 **Examples:**

- ``examples/08_inline_recovery/`` - Complete inline recovery examples

Summary
-------

**You've Learned:**

✅ The difference between exception-based and inline recovery

✅ How to configure Fenix for inline recovery with ``RESUME_MODE = RETURN``

✅ How to register callbacks that execute during failures

✅ How to restore data transparently without restarting loops

✅ How to build production-ready applications with inline recovery

✅ When to use inline vs. exception-based recovery

**Key Concepts:**

- **Inline recovery**: Execution continues from exactly where it was interrupted
- **Callbacks**: Functions that Fenix calls automatically during failure handling
- **Transparent recovery**: Application code has no try/catch blocks, failures handled in background
- **State preservation**: Local variables, loop counters, stack state all preserved

**Comparison: Exception-Based vs Inline Recovery:**

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Feature
     - Exception-Based
     - Inline Recovery
   * - Control Flow
     - try/catch blocks
     - Normal flow, callbacks
   * - Loop Behavior
     - Restarts from beginning
     - Continues inline
   * - Local State
     - Lost (unless saved)
     - Preserved
   * - Code Changes
     - Moderate
     - Minimal
   * - Complexity
     - Moderate
     - Lower for complex apps
   * - Best For
     - Simple apps, learning
     - Production, complex state

You're now ready to tackle the most advanced recovery pattern: message logging with automatic replay!
