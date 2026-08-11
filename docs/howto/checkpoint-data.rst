Checkpoint Application Data
===========================

This guide shows you how to checkpoint your application's state using Fenix's data recovery system, so you can restore it after a rank failure instead of restarting from scratch.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Here's the minimal code to checkpoint and restore data:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     // Application state
     double my_data[1000];
     const int GROUP_ID = 0;
     const int MEMBER_ID = 0;

     // Create group and member
     data::group_create(GROUP_ID);
     data::member_create(GROUP_ID, MEMBER_ID,
                        my_data, 1000, MPI_DOUBLE);

     // Checkpoint data
     data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
     data::commit_barrier(GROUP_ID);

     // Later, after recovery
     data::member_restore(GROUP_ID, MEMBER_ID);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

The rest of this guide explains each step in detail.

Understanding Fenix Data Recovery
----------------------------------

Fenix's data recovery system has three key concepts:

1. **Data Groups**: Containers that hold related data members and provide transaction semantics
2. **Data Members**: Individual pieces of application data (arrays, structures, etc.)
3. **Snapshots**: Point-in-time copies created by ``commit`` operations

When you checkpoint:

- Data is redundantly stored across multiple ranks (in-memory RAID)
- Multiple snapshots can be kept with timestamps
- Data survives rank failures and can be restored by replacement ranks

What to Checkpoint
------------------

Checkpoint data that is:

- **Expensive to recompute**: Large arrays, simulation state, accumulated results
- **Necessary for correctness**: Iteration counters, convergence state, random seeds
- **Stateful**: Data that changes over time and can't be regenerated

Don't checkpoint:

- **Temporary/scratch data**: Intermediate values that are recomputed each iteration
- **Derived data**: Values that can be quickly recalculated from checkpointed data
- **Constants**: Configuration or input data that never changes

Example: Iterative Solver State
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct SolverState {
     // Checkpoint these:
     int iteration;           // Current iteration counter
     double* solution;        // Current solution vector
     double* residual;        // Residual vector
     double convergence;      // Convergence metric
     uint64_t rng_state;      // Random number generator state

     // Don't checkpoint these:
     double* temp_vector;     // Temporary scratch space
     double current_error;    // Recomputed each iteration
   };

Step-by-Step Guide
------------------

Step 1: Initialize Fenix
~~~~~~~~~~~~~~~~~~~~~~~~~

First, initialize Fenix with spare ranks:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     // Initialize with 2 spare ranks
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

Step 2: Create a Data Group
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create a data group to hold your checkpointed data:

.. code-block:: cpp

     // Create group with In-Memory RAID policy
     const int GROUP_ID = 0;
     data::group_create(GROUP_ID);

The group ID is an integer you choose. You can have multiple groups for different data.

**Advanced: Custom Redundancy Policy**

By default, Fenix uses a sensible RAID configuration. For more control:

.. code-block:: cpp

   // C API with explicit policy configuration
   int error;
   Fenix_Data_group_create(
     GROUP_ID,                              // Group ID
     res_comm,                              // Communicator
     0,                                      // Starting timestamp
     1,                                      // Depth (# of historical snapshots to keep, 0=keep only latest)
     FENIX_DATA_POLICY_IN_MEMORY_RAID,      // Policy type
     (int[]){1, size/2},                    // Policy params [k, n]
     &error
   );

   // k=1: One parity block
   // n=size/2: Distribute across half the ranks

Step 3: Create Data Members
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Register each piece of data you want to checkpoint:

.. code-block:: cpp

     // Application data
     double solution[10000];
     double residual[10000];
     int iteration = 0;

     // Create members for each array/variable
     const int SOLUTION_MEMBER = 0;
     const int RESIDUAL_MEMBER = 1;
     const int ITERATION_MEMBER = 2;

     data::member_create(GROUP_ID, SOLUTION_MEMBER,
                        solution, 10000, MPI_DOUBLE);
     data::member_create(GROUP_ID, RESIDUAL_MEMBER,
                        residual, 10000, MPI_DOUBLE);
     data::member_create(GROUP_ID, ITERATION_MEMBER,
                        &iteration, 1, MPI_INT);

**Member IDs** are integers you choose. They must be unique within a group.

**Checkpointing Structures:**

.. code-block:: cpp

   struct State {
     int iteration;
     double data[100];
     uint64_t checksum;
   } state;

   // Checkpoint entire structure as bytes
   data::member_create(GROUP_ID, 0, &state, sizeof(state), MPI_BYTE);

Step 4: Store Data (Checkpoint)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Call ``member_store`` to copy data into redundant storage:

.. code-block:: cpp

     // Store all members
     data::member_store(GROUP_ID, SOLUTION_MEMBER, SUBSET_FULL);
     data::member_store(GROUP_ID, RESIDUAL_MEMBER, SUBSET_FULL);
     data::member_store(GROUP_ID, ITERATION_MEMBER, SUBSET_FULL);

``SUBSET_FULL`` means checkpoint all elements of the array. To checkpoint only specific element ranges (e.g., only elements that changed), see :doc:`partial-checkpoints`.

Step 5: Commit the Checkpoint
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Commit finalizes the checkpoint and makes it available for recovery:

.. code-block:: cpp

     // Commit and wait for all ranks to finish
     data::commit_barrier(GROUP_ID);

After this, the checkpoint is safe and can be restored even if ranks fail.

**Alternative: Non-blocking Commit**

.. code-block:: cpp

   // Start commit without waiting
   int time_stamp;
   Fenix_Data_commit(GROUP_ID, &time_stamp);

   // Continue computation...

   // Wait for commit to finish later
   Fenix_Data_wait(GROUP_ID);

Step 6: Restore After Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a rank is recovered (after a failure), restore the checkpointed data:

.. code-block:: cpp

     if (fenix::role() == fenix::RECOVERED_RANK) {
       // Recreate the group
       data::group_create(GROUP_ID);

       // Define members (use member_define, not member_create)
       data::member_define(GROUP_ID, SOLUTION_MEMBER,
                          solution, 10000, MPI_DOUBLE);
       data::member_define(GROUP_ID, RESIDUAL_MEMBER,
                          residual, 10000, MPI_DOUBLE);
       data::member_define(GROUP_ID, ITERATION_MEMBER,
                          &iteration, 1, MPI_INT);

       // Restore from latest checkpoint
       data::member_restore(GROUP_ID, SOLUTION_MEMBER);
       data::member_restore(GROUP_ID, RESIDUAL_MEMBER);
       data::member_restore(GROUP_ID, ITERATION_MEMBER);

       printf("Rank %d recovered to iteration %d\n", rank, iteration);
     }

**Note:** While ``member_restore`` can create members automatically, using ``member_define`` first is preferred in recovery contexts when you need to specify a **custom serializer** (via ``member_fdefine``) or want **explicit control over buffer pointers**. Both ``member_define`` and ``member_restore`` are idempotent and safe for retry loops.

Complete Example
----------------

Here's a complete iterative solver with checkpointing:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <cmath>
   #include <cstdio>

   constexpr int N = 10000;           // Problem size
   constexpr int MAX_ITER = 1000;     // Max iterations
   constexpr int CHECKPOINT_FREQ = 10; // Checkpoint every 10 iterations

   constexpr int GROUP_ID = 0;
   constexpr int SOLUTION_MEMBER = 0;
   constexpr int RESIDUAL_MEMBER = 1;
   constexpr int STATE_MEMBER = 2;

   struct SolverState {
     int iteration;
     double convergence;
   };

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     // Initialize Fenix
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Application data
     double solution[N];
     double residual[N];
     SolverState state;

     // Initialize or recover
     if (fenix::role() == fenix::INITIAL_RANK) {
       // Initial ranks: initialize data
       state.iteration = 0;
       state.convergence = 1.0;

       for (int i = 0; i < N; i++) {
         solution[i] = 0.0;
         residual[i] = 1.0;
       }

       // Create data group and members
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, SOLUTION_MEMBER,
                          solution, N, MPI_DOUBLE);
       data::member_create(GROUP_ID, RESIDUAL_MEMBER,
                          residual, N, MPI_DOUBLE);
       data::member_create(GROUP_ID, STATE_MEMBER,
                          &state, sizeof(state), MPI_BYTE);

       // Initial checkpoint
       data::member_store(GROUP_ID, SOLUTION_MEMBER, SUBSET_FULL);
       data::member_store(GROUP_ID, RESIDUAL_MEMBER, SUBSET_FULL);
       data::member_store(GROUP_ID, STATE_MEMBER, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);

       printf("Rank %d: initialized\n", rank);

     } else {
       // Recovered ranks: restore from checkpoint
       printf("Rank %d: recovering...\n", rank);

       data::group_create(GROUP_ID);
       data::member_define(GROUP_ID, SOLUTION_MEMBER,
                          solution, N, MPI_DOUBLE);
       data::member_define(GROUP_ID, RESIDUAL_MEMBER,
                          residual, N, MPI_DOUBLE);
       data::member_define(GROUP_ID, STATE_MEMBER,
                          &state, sizeof(state), MPI_BYTE);

       data::member_restore(GROUP_ID, SOLUTION_MEMBER);
       data::member_restore(GROUP_ID, RESIDUAL_MEMBER);
       data::member_restore(GROUP_ID, STATE_MEMBER);

       printf("Rank %d: recovered to iteration %d\n",
              rank, state.iteration);
     }

     // Register callback for inline recovery
     fenix::callback_register([&](MPI_Comm comm, int err) {
       data::group_create(GROUP_ID);
       data::member_restore(GROUP_ID, SOLUTION_MEMBER, NULL, 0);
       data::member_restore(GROUP_ID, RESIDUAL_MEMBER, NULL, 0);
       data::member_restore(GROUP_ID, STATE_MEMBER, NULL, 0);
       printf("Rank %d: continuing inline at iteration %d\n",
              rank, state.iteration);
     });

     // Main solver loop
     for (int i = state.iteration; i < MAX_ITER; i++) {
       state.iteration = i;

       // Compute next iteration (simplified)
       for (int j = 0; j < N; j++) {
         double old = solution[j];
         solution[j] = solution[j] + 0.1 * residual[j];
         residual[j] = residual[j] - 0.1 * (solution[j] - old);
       }

       // Check convergence
       double local_norm = 0.0;
       for (int j = 0; j < N; j++) {
         local_norm += residual[j] * residual[j];
       }

       MPI_Allreduce(&local_norm, &state.convergence, 1,
                    MPI_DOUBLE, MPI_SUM, res_comm);
       state.convergence = std::sqrt(state.convergence);

       if (rank == 0 && i % 10 == 0) {
         printf("Iteration %d: convergence = %e\n",
                i, state.convergence);
       }

       // Checkpoint periodically
       if (i % CHECKPOINT_FREQ == 0) {
         data::member_store(GROUP_ID, SOLUTION_MEMBER, SUBSET_FULL);
         data::member_store(GROUP_ID, RESIDUAL_MEMBER, SUBSET_FULL);
         data::member_store(GROUP_ID, STATE_MEMBER, SUBSET_FULL);
         data::commit_barrier(GROUP_ID);
       }

       // Check convergence
       if (state.convergence < 1e-6) {
         if (rank == 0) {
           printf("Converged at iteration %d\n", i);
         }
         break;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

When to Checkpoint
------------------

Checkpoint Frequency Tradeoffs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Too frequent:**

- High overhead (copying and distributing data)
- Network congestion
- Slower overall execution

**Too infrequent:**

- More work lost on failure
- Longer recovery time
- Higher total execution time if failures occur

**Guidelines:**

- Start with every 10-100 iterations
- Checkpoint after expensive computations
- Checkpoint before risky operations (large communications)
- Profile to find optimal frequency

Example: Adaptive Checkpointing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Checkpoint after every expensive operation
   void expensive_computation() {
     // ... complex calculation ...

     // Checkpoint immediately after
     checkpoint_all();
   }

   // Or checkpoint based on time
   auto last_checkpoint = std::chrono::steady_clock::now();
   const int CHECKPOINT_INTERVAL_SEC = 60; // Every minute

   for (int i = 0; i < MAX_ITER; i++) {
     // Do work...

     auto now = std::chrono::steady_clock::now();
     auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
       now - last_checkpoint
     ).count();

     if (elapsed >= CHECKPOINT_INTERVAL_SEC) {
       checkpoint_all();
       last_checkpoint = now;
     }
   }

Checkpointing Multiple Data Types
----------------------------------

Example: Mixed Data Types
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Different data types in one group
   double* solution = new double[N];
   int* indices = new int[M];
   float convergence = 1.0f;
   char status[256];

   const int GROUP = 0;

   data::group_create(GROUP);

   data::member_create(GROUP, 0, solution, N, MPI_DOUBLE);
   data::member_create(GROUP, 1, indices, M, MPI_INT);
   data::member_create(GROUP, 2, &convergence, 1, MPI_FLOAT);
   data::member_create(GROUP, 3, status, 256, MPI_CHAR);

   // Store all at once
   data::member_store(GROUP, 0, SUBSET_FULL);
   data::member_store(GROUP, 1, SUBSET_FULL);
   data::member_store(GROUP, 2, SUBSET_FULL);
   data::member_store(GROUP, 3, SUBSET_FULL);
   data::commit_barrier(GROUP);

Example: Multiple Groups
~~~~~~~~~~~~~~~~~~~~~~~~~

Use different groups for data with different checkpoint frequencies:

.. code-block:: cpp

   const int FREQUENT_GROUP = 0;  // Checkpoint every iteration
   const int RARE_GROUP = 1;      // Checkpoint every 100 iterations

   // Frequently changing data
   data::group_create(FREQUENT_GROUP);
   data::member_create(FREQUENT_GROUP, 0, solution, N, MPI_DOUBLE);

   // Rarely changing data
   data::group_create(RARE_GROUP);
   data::member_create(RARE_GROUP, 0, config, sizeof(config), MPI_BYTE);

   for (int i = 0; i < MAX_ITER; i++) {
     // Checkpoint solution every iteration
     data::member_store(FREQUENT_GROUP, 0, SUBSET_FULL);
     data::commit_barrier(FREQUENT_GROUP);

     // Checkpoint config every 100 iterations
     if (i % 100 == 0) {
       data::member_store(RARE_GROUP, 0, SUBSET_FULL);
       data::commit_barrier(RARE_GROUP);
     }
   }

Advanced Topics
---------------

Resizable Members
~~~~~~~~~~~~~~~~~

If your data size changes over time:

.. code-block:: cpp

   int current_size = 1000;
   double* data = new double[current_size];

   // Create with initial size
   data::member_create(GROUP_ID, MEMBER_ID, data, current_size, MPI_DOUBLE);

   // Later, if size changes
   current_size = 2000;
   double* new_data = new double[current_size];
   delete[] data;
   data = new_data;

   // Update member attribute with new pointer and size
   int flag;
   Fenix_Data_member_attr_set(GROUP_ID, MEMBER_ID,
                              FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                              data, &flag);
   Fenix_Data_member_attr_set(GROUP_ID, MEMBER_ID,
                              FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
                              &current_size, &flag);

See :doc:`/examples/index` for a complete resizable member example.

Multiple Snapshots
~~~~~~~~~~~~~~~~~~

Keep multiple checkpoint versions:

.. code-block:: cpp

   // Create group with depth=3 (keep last 3 snapshots)
   int error;
   Fenix_Data_group_create(GROUP_ID, res_comm, 0, 3,
                          FENIX_DATA_POLICY_IN_MEMORY_RAID,
                          (int[]){1, size/2}, &error);

   // Each commit creates a new snapshot with timestamp
   int time_stamp1, time_stamp2, time_stamp3;

   // First checkpoint
   data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
   Fenix_Data_commit(GROUP_ID, &time_stamp1);

   // Second checkpoint
   data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
   Fenix_Data_commit(GROUP_ID, &time_stamp2);

   // Third checkpoint
   data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
   Fenix_Data_commit(GROUP_ID, &time_stamp3);

   // Restore from specific snapshot
   Fenix_Data_member_restore(GROUP_ID, MEMBER_ID, data, count,
                            time_stamp2,  // Restore 2nd checkpoint
                            NULL);

Troubleshooting
---------------

**Problem: Restore returns FENIX_ERROR_NODATA_FOUND**

- Check that you called ``commit_barrier`` before the failure
- Verify that the group and member IDs match
- Ensure enough ranks survived to recover the data

**Problem: Restored data is incorrect**

- Verify you're using ``member_define`` (not ``member_create``) on recovered ranks
- Check that the buffer pointer and size are correct
- Ensure data types match between store and restore

**Problem: Checkpoint is too slow**

- Reduce checkpoint frequency
- Use partial checkpoints (subsets) for large arrays
- Consider using multiple groups for different frequencies
- Profile to identify bottlenecks

**Problem: Out of memory**

- Reduce depth (number of snapshots to keep)
- Use partial checkpoints to store less data
- Increase number of ranks for redundancy (spreads data across more nodes)

**Problem: member_create fails after recovery**

- Use ``member_define`` instead of ``member_create`` for recovered ranks
- ``member_create`` only works once per member; ``member_define`` is idempotent

Verification Checklist
----------------------

Before deploying your checkpointing code:

- [ ] Checkpoint includes all necessary state
- [ ] Checkpoint frequency balances overhead vs. recovery time
- [ ] Initial ranks use ``member_create``
- [ ] Recovered ranks use ``member_define``
- [ ] Recovery callback restores all state
- [ ] Checksums verify data integrity
- [ ] Tests inject failures at various points
- [ ] Code handles cascading failures

See Also
--------

- :doc:`partial-checkpoints` - Checkpoint only part of large arrays
- :doc:`optimize-checkpoints` - Performance tuning guide
- :doc:`choose-recovery-pattern` - Choose inline vs. longjmp recovery
- :doc:`test-locally` - How to test checkpointing
- :doc:`/api/data-recovery` - Data recovery API reference
- :doc:`/guides/data-recovery` - Conceptual guide to data recovery
- :doc:`/guides/imr-policy` - Understanding In-Memory RAID policy
