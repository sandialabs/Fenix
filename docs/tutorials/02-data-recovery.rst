Tutorial 2: Adding Data Recovery to Your Application
=====================================================

.. contents:: In This Tutorial
   :local:
   :depth: 2

Introduction
------------

In the previous tutorial, you learned how Fenix automatically recovers from rank failures by rebuilding communicators using spare ranks. However, process recovery alone isn't enough—when a rank fails, its application data is lost. **Data recovery** allows you to checkpoint and restore application state so your program can continue from where it left off rather than starting over.

**Why Checkpoint Data?**

Without data recovery:

- Failed ranks restart with uninitialized data
- You must re-compute all lost work from the beginning
- Long-running computations can lose hours of progress

With data recovery:

- Failed ranks restore their state from the last checkpoint
- Only work since the last checkpoint needs recomputation
- Minimal performance impact and fast recovery

**What You'll Learn:**

✓ Creating data groups to organize checkpoint data

✓ Registering data members for checkpointing

✓ Storing snapshots of application state

✓ Restoring data after failures

✓ Using data subsets for partial checkpoints

✓ Best practices for checkpoint frequency

**Time:** 30 minutes | **Difficulty:** Intermediate

Prerequisites
-------------

Before starting this tutorial, you should:

✓ Understand basic Fenix process recovery (see :doc:`/quickstart`)

✓ Be comfortable with MPI programming

✓ Have completed or read :doc:`01-first-program` (understanding ``fenix::init()``, spare ranks, and recovery)

✓ Be familiar with C++17 or later (we'll use the modern C++ API)

Understanding Data Recovery Concepts
-------------------------------------

Fenix's data recovery system has three main components:

Data Groups
^^^^^^^^^^^

A **data group** is a container that holds related data members that should be checkpointed together. Think of it as a transaction boundary—when you commit a group, all its members are saved atomically.

.. code-block:: cpp

   // Create a data group with ID 0
   fenix::data::group_create(group_id);

Key properties:

- Each group has a unique integer ID
- Groups should be recreated after every failure
- Groups can contain multiple data members
- Committing a group creates a consistent snapshot

Data Members
^^^^^^^^^^^^

A **data member** represents a piece of application data to checkpoint—typically an array, vector, or struct. Each member has:

- A unique ID within its group
- A pointer to the data buffer
- A size (element count)
- An MPI datatype

.. code-block:: cpp

   // Register a vector as a data member
   std::vector<double> my_data(1000);
   fenix::data::member_create(
     group_id, member_id,
     my_data.data(), my_data.size(), MPI_DOUBLE
   );

Snapshots
^^^^^^^^^

A **snapshot** is a consistent checkpoint of all members in a group at a specific point in time. Each snapshot gets a unique timestamp.

.. code-block:: cpp

   // Store data and create a snapshot
   fenix::data::member_store(group_id);
   int time_stamp;
   fenix::data::commit(group_id, &time_stamp);

You can keep multiple snapshots (controlled by group depth) and restore from any of them.

Creating Data Groups and Members
---------------------------------

Let's build a simple example that checkpoints a computational state. We'll simulate a scientific application that computes values iteratively.

Basic Setup
^^^^^^^^^^^

First, include the necessary headers and set up constants:

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <stdio.h>

   // Data group and member IDs
   constexpr int GROUP_ID = 0;
   constexpr int DATA_MEMBER_ID = 0;
   constexpr int STATE_MEMBER_ID = 1;

The modern API provides a cleaner namespace:

.. code-block:: cpp

   using namespace fenix::data;  // For data recovery functions

Creating a Data Group
^^^^^^^^^^^^^^^^^^^^^

After initializing Fenix, create a data group. **Important:** Always recreate groups after initialization, even after recovery:

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 10-11

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Create data group (do this every time, including after recovery)
     group_create(GROUP_ID);

Why always recreate? After a failure, the group structure is lost. Recreating it is idempotent and ensures the group exists for both initial and recovered ranks.

**Advanced Options:**

.. code-block:: cpp

   // Create group with custom settings
   group_create(GROUP_ID, {
     .comm = res_comm,           // Communicator (default: Fenix comm)
     .start_time_stamp = 0,      // Starting timestamp
     .depth = 3,                 // Keep up to 3 snapshots
     .policy_name = FENIX_DATA_POLICY_IMR  // In-Memory Redundancy
   });

Registering Data Members
^^^^^^^^^^^^^^^^^^^^^^^^^

Now register the application data you want to checkpoint:

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 5-9, 14-15

   struct AppState {
     int iteration;
     int completed_work;
   };

   AppState state = {0, 0};
   std::vector<double> data(1000);

   // Register state struct
   member_create(
     GROUP_ID, STATE_MEMBER_ID,
     &state, 2, MPI_INT  // 2 integers in the struct
   );

   // Register computational data vector
   member_create(
     GROUP_ID, DATA_MEMBER_ID,
     data.data(), data.size(), MPI_DOUBLE
   );

**Key Points:**

- Call ``member_create()`` only for initial ranks (first time through)
- Use ``member_define()`` for recovered ranks (explained in restoration section)
- You can checkpoint any MPI datatype: arrays, vectors, structs, etc.
- Keep member IDs unique within a group

Storing Data (Checkpointing)
-----------------------------

Once data members are registered, you can checkpoint them at any time. The typical pattern is:

1. Do some computation
2. Store updated data members
3. Commit to create a snapshot

Basic Store and Commit
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 10-14

   // Application work loop
   for (int iter = 0; iter < 100; iter++) {
     // Do computation
     for (size_t i = 0; i < data.size(); i++) {
       data[i] = data[i] * 2.0 + rank;
     }
     state.iteration = iter;
     state.completed_work++;

     // Checkpoint every 10 iterations
     if (iter % 10 == 0) {
       member_store(GROUP_ID);  // Store all members
       commit(GROUP_ID);        // Create snapshot
     }
   }

The ``member_store()`` function copies data into Fenix's redundant storage. The ``commit()`` finalizes the snapshot, making it recoverable.

**Performance Tip:** Checkpoint frequently enough to limit re-computation, but not so often that checkpoint overhead dominates. Typical frequencies: every 5-50 iterations depending on iteration cost.

Storing Specific Members
^^^^^^^^^^^^^^^^^^^^^^^^^

You can checkpoint individual members instead of all members:

.. code-block:: cpp

   // Store only the data vector
   member_store(GROUP_ID, DATA_MEMBER_ID);

   // Store only the state struct
   member_store(GROUP_ID, STATE_MEMBER_ID);

   // Store all members (shorthand)
   member_store(GROUP_ID);

   // Commit after storing
   commit(GROUP_ID);

Collective vs. Non-Collective
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``member_store()`` is a **collective operation** within the group's communicator, but it's not globally synchronizing. Some ranks may finish before others.

Use ``commit_barrier()`` if you need a synchronization point:

.. code-block:: cpp

   member_store(GROUP_ID);
   commit_barrier(GROUP_ID);  // Synchronizes all ranks in group

This ensures all ranks complete the checkpoint before any rank proceeds.

Using Data Subsets
^^^^^^^^^^^^^^^^^^^

For large arrays, you may want to checkpoint only a portion of the data. A **data subset** specifies which element ranges to checkpoint, rather than checkpointing the entire array. This reduces checkpoint time and storage when only some elements have changed:

.. code-block:: cpp
   :linenos:

   // Create a subset: elements 0-99 and 200-299
   fenix::DataSubset my_subset{
     {0, 99},      // First range
     {200, 299}    // Second range
   };

   // Store only the subset
   member_store(GROUP_ID, DATA_MEMBER_ID, my_subset);
   commit(GROUP_ID);

This is useful when:

- Only part of the data changes between checkpoints
- You want to reduce checkpoint overhead
- You're implementing incremental checkpointing

**Full vs. Partial Checkpoints:**

.. code-block:: cpp

   // SUBSET_FULL: Checkpoint all elements (most common)
   member_store(GROUP_ID, DATA_MEMBER_ID, SUBSET_FULL);

   // SUBSET_EMPTY: Checkpoint no elements (placeholder)
   member_store(GROUP_ID, DATA_MEMBER_ID, SUBSET_EMPTY);

   // Custom subset: Checkpoint only specific element ranges
   member_store(GROUP_ID, DATA_MEMBER_ID, my_subset);

Restoring Data (Recovery)
--------------------------

When a rank fails and recovers, it needs to restore its state from the last checkpoint. The restoration process differs for initial vs. recovered ranks.

Detecting Recovery
^^^^^^^^^^^^^^^^^^

Use ``fenix::role()`` to determine if restoration is needed:

.. code-block:: cpp

   bool need_recovery = (fenix::role() != fenix::INITIAL_RANK);

   if (need_recovery) {
     // This rank recovered from a failure
     // Need to restore data
   }

Basic Restoration
^^^^^^^^^^^^^^^^^

For recovered ranks, use ``member_define()`` instead of ``member_create()``, then restore:

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 6-11, 13-15

   if (fenix::role() == fenix::INITIAL_RANK) {
     // Initial ranks: create members and initialize
     member_create(GROUP_ID, STATE_MEMBER_ID, &state, 2, MPI_INT);
     member_create(GROUP_ID, DATA_MEMBER_ID, data.data(), data.size(), MPI_DOUBLE);
   } else {
     // Recovered ranks: define members and restore
     member_define(GROUP_ID, STATE_MEMBER_ID, &state, 2, MPI_INT);
     member_define(GROUP_ID, DATA_MEMBER_ID, data.data(), data.size(), MPI_DOUBLE);

     // Restore from latest snapshot
     member_restore(GROUP_ID, STATE_MEMBER_ID);
     member_restore(GROUP_ID, DATA_MEMBER_ID);

     printf("Rank %d recovered to iteration %d\n",
            rank, state.iteration);
   }

**Why member_define() vs member_create()?**

- ``member_create()``: First-time registration, fails if member already exists
- ``member_define()``: Idempotent registration, safe to call multiple times

Restoration Options
^^^^^^^^^^^^^^^^^^^

You can control what snapshot to restore from:

.. code-block:: cpp

   // Restore from latest snapshot (default)
   member_restore(GROUP_ID, DATA_MEMBER_ID,
                  nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST);

   // Restore each element from its most recent available snapshot
   // (iterates backward through snapshots until all elements recovered)
   member_restore(GROUP_ID, DATA_MEMBER_ID,
                  nullptr, 0, FENIX_DATA_SNAPSHOT_ALL);

``FENIX_DATA_SNAPSHOT_ALL`` is useful for partial checkpoints. If you've stored
different subsets across multiple snapshots, this option reconstructs the complete
member by taking each element from its most recent available snapshot.

Restoring with Subsets
^^^^^^^^^^^^^^^^^^^^^^^

You can query which data was stored and restore accordingly:

.. code-block:: cpp
   :linenos:

   // Query what data was stored
   fenix::DataSubset stored_data;
   member_restore(
     GROUP_ID, DATA_MEMBER_ID,
     nullptr, 0,  // Don't actually restore yet
     FENIX_DATA_SNAPSHOT_LATEST,
     stored_data  // Output: what was stored
   );

   // Now restore the actual data
   member_restore(
     GROUP_ID, DATA_MEMBER_ID,
     data.data(), data.size(),
     FENIX_DATA_SNAPSHOT_LATEST
   );

Complete Working Example
-------------------------

Here's a complete program that demonstrates data recovery with checkpointing and restoration:

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <stdio.h>
   #include <signal.h>

   constexpr int GROUP_ID = 0;
   constexpr int DATA_ID = 0;
   constexpr int STATE_ID = 1;

   struct State {
     int iteration;
     int work_done;
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix with 1 spare rank
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Application data
     State state = {0, 0};
     std::vector<double> data(100);

     // Always create data group (initial and recovered ranks)
     fenix::data::group_create(GROUP_ID);

     // Setup based on role
     if (fenix::role() == fenix::INITIAL_RANK) {
       printf("Rank %d: Initial rank starting\n", rank);

       // Initialize data
       for (size_t i = 0; i < data.size(); i++) {
         data[i] = static_cast<double>(rank * 1000 + i);
       }

       // Create data members
       fenix::data::member_create(
         GROUP_ID, STATE_ID, &state, 2, MPI_INT
       );
       fenix::data::member_create(
         GROUP_ID, DATA_ID, data.data(), data.size(), MPI_DOUBLE
       );

       // Initial checkpoint
       fenix::data::member_store(GROUP_ID);
       fenix::data::commit_barrier(GROUP_ID);

     } else {
       printf("Rank %d: Recovered rank, restoring data\n", rank);

       // Define members (idempotent)
       fenix::data::member_define(
         GROUP_ID, STATE_ID, &state, 2, MPI_INT
       );
       fenix::data::member_define(
         GROUP_ID, DATA_ID, data.data(), data.size(), MPI_DOUBLE
       );

       // Restore from checkpoint
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, DATA_ID);

       printf("Rank %d: Restored to iteration %d\n",
              rank, state.iteration);
     }

     // Main computation loop
     int max_iterations = 50;
     for (int iter = state.iteration; iter < max_iterations; iter++) {
       // Simulate computation
       for (size_t i = 0; i < data.size(); i++) {
         data[i] = data[i] * 1.01 + rank;
       }

       state.iteration = iter + 1;
       state.work_done++;

       // Checkpoint every 10 iterations
       if ((iter + 1) % 10 == 0) {
         fenix::data::member_store(GROUP_ID);
         fenix::data::commit_barrier(GROUP_ID);

         if (rank == 0) {
           printf("Checkpoint at iteration %d\n", iter + 1);
         }
       }

       // Inject failure at iteration 25 on rank 1
       if (rank == 1 && iter == 25 &&
           fenix::role() == fenix::INITIAL_RANK) {
         printf("Rank 1: Simulating failure at iteration %d\n", iter);
         raise(SIGKILL);
       }

       MPI_Barrier(res_comm);
     }

     // Final results
     printf("Rank %d completed: %d iterations, %d work units\n",
            rank, state.iteration, state.work_done);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Building the Example
^^^^^^^^^^^^^^^^^^^^^

Save the code as ``data_recovery_tutorial.cpp`` and compile:

.. code-block:: bash

   mpicxx -std=c++17 data_recovery_tutorial.cpp -o data_recovery_tutorial \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix

Running the Example
^^^^^^^^^^^^^^^^^^^

Run with 4 total ranks (3 active + 1 spare):

.. code-block:: bash

   mpiexec --with-ft mpi -n 4 ./data_recovery_tutorial

**Expected Output:**

.. code-block:: text

   Rank 0: Initial rank starting
   Rank 1: Initial rank starting
   Rank 2: Initial rank starting
   Checkpoint at iteration 10
   Checkpoint at iteration 20
   Rank 1: Simulating failure at iteration 25
   Checkpoint at iteration 30
   Rank 1: Recovered rank, restoring data
   Rank 1: Restored to iteration 30
   Checkpoint at iteration 40
   Checkpoint at iteration 50
   Rank 0 completed: 50 iterations, 50 work units
   Rank 1 completed: 50 iterations, 20 work units  ← Only did 20 iterations after recovery
   Rank 2 completed: 50 iterations, 50 work units

Notice that rank 1:

- Failed at iteration 25
- Recovered and restored to iteration 30 (last checkpoint)
- Only had to redo iterations 30-50 (20 iterations instead of all 50!)

Running with Failures
----------------------

Understanding Failure Scenarios
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let's examine what happens during different failure scenarios:

**Scenario 1: Failure Between Checkpoints**

If a rank fails at iteration 37 and the last checkpoint was at iteration 30:

1. Spare rank replaces failed rank
2. Data restores to iteration 30 state
3. Iterations 30-37 must be recomputed
4. Only 7 iterations of work lost

**Scenario 2: Failure During Checkpoint**

If a rank fails while ``member_store()`` is executing:

1. The partial checkpoint is discarded
2. Recovery uses the previous complete checkpoint
3. More work may need recomputation

This is why checkpoint frequency is a tradeoff:

- **More frequent**: Less recomputation, more overhead
- **Less frequent**: More recomputation, less overhead

Testing Recovery Manually
^^^^^^^^^^^^^^^^^^^^^^^^^^

You can test recovery by manually killing ranks:

.. code-block:: bash

   # Terminal 1: Run the program
   mpiexec --with-ft mpi -n 4 ./data_recovery_tutorial

   # Terminal 2: Kill a specific rank (find its PID first)
   ps aux | grep data_recovery_tutorial
   kill -9 <PID>

The program should detect the failure and continue.

Multiple Failures
^^^^^^^^^^^^^^^^^

You can handle multiple failures if you have enough spares:

.. code-block:: cpp

   // Initialize with 2 spares to handle 2 failures
   fenix::init({.out_comm = &res_comm, .spares = 2});

Modify the example to kill multiple ranks at different iterations and observe the recovery behavior.

Best Practices
--------------

When to Checkpoint
^^^^^^^^^^^^^^^^^^

**Checkpoint Frequency Guidelines:**

.. list-table::
   :header-rows: 1
   :widths: 30 40 30

   * - Iteration Cost
     - Checkpoint Frequency
     - Reasoning
   * - < 1 ms
     - Every 50-100 iterations
     - Overhead dominates
   * - 1-10 ms
     - Every 20-50 iterations
     - Balance overhead/recomputation
   * - 10-100 ms
     - Every 5-20 iterations
     - Minimize recomputation
   * - > 100 ms
     - Every 1-5 iterations
     - Checkpoint cost is small

**Rule of Thumb:** Checkpoint often enough that you can tolerate recomputing lost work, but not so often that checkpoint overhead slows your application significantly.

What to Checkpoint
^^^^^^^^^^^^^^^^^^

**Always Checkpoint:**

- Loop iteration counters
- Computational state (convergence criteria, etc.)
- Random number generator state
- Any data needed to resume computation

**Consider Checkpointing:**

- Large working arrays (if they're expensive to recompute)
- Communication buffers (if needed for correctness)
- Derived data that's expensive to regenerate

**Don't Checkpoint:**

- Temporary scratch space
- Data that can be quickly recomputed
- MPI communicators (Fenix handles those)
- Read-only input data (keep in memory, don't checkpoint)

Memory Management
^^^^^^^^^^^^^^^^^

**For Dynamic Data:**

If your data size changes, update the member buffer pointer:

.. code-block:: cpp

   std::vector<double> data(100);
   member_create(GROUP_ID, DATA_ID, data.data(), data.size(), MPI_DOUBLE);

   // Later: data size changes
   data.resize(200);

   // Update the buffer pointer
   int flag;
   Fenix_Data_member_attr_set(
     GROUP_ID, DATA_ID,
     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     data.data(), &flag
   );

See :doc:`/examples/07-resizeable-member` for a complete demonstration.

Error Handling
^^^^^^^^^^^^^^

Always check return codes:

.. code-block:: cpp

   int err = fenix::data::member_restore(GROUP_ID, DATA_ID);
   if (err != FENIX_SUCCESS) {
     fprintf(stderr, "Rank %d: restore failed with code %d\n", rank, err);
     MPI_Abort(res_comm, 1);
   }

In production code, handle errors gracefully rather than aborting.

Checkpoint Depth
^^^^^^^^^^^^^^^^

The ``depth`` parameter controls how many snapshots to keep:

.. code-block:: cpp

   // Keep last 3 snapshots
   group_create(GROUP_ID, {.depth = 3});

**Tradeoffs:**

- **Depth = 1**: Minimal memory, but failure during checkpoint = data loss
- **Depth = 2-3**: Good balance, can recover from checkpoint failures
- **Depth > 3**: More memory usage, rarely needed

Exercises
---------

Test your understanding with these exercises:

Exercise 1: Adjust Checkpoint Frequency
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Modify the example to checkpoint every 5 iterations instead of every 10. Observe how this affects:

- Recovery time (how much work is lost)
- Total execution time (checkpoint overhead)

**Hint:** Change line 84 in the complete example.

Exercise 2: Add More Data Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add a third data member: a status string that tracks the application phase:

.. code-block:: cpp

   char status[50] = "initializing";

Checkpoint and restore this string along with the other data.

**Hint:** Use ``MPI_CHAR`` datatype and appropriate count.

Exercise 3: Use Data Subsets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Modify the example to checkpoint only the first 50 elements of the data array:

.. code-block:: cpp

   fenix::DataSubset first_half{{0, 49}};
   member_store(GROUP_ID, DATA_ID, first_half);

After recovery, verify that only the first 50 elements are restored.

Exercise 4: Handle Multiple Failures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Extend the example to:

1. Increase spares to 2
2. Inject failures on ranks 1 and 2 at different iterations
3. Verify both ranks recover correctly

Exercise 5: Implement Adaptive Checkpointing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Implement a simple adaptive checkpointing strategy:

- If iterations are fast (< 10ms), checkpoint every 20 iterations
- If iterations are slow (> 10ms), checkpoint every 5 iterations

Measure iteration time and adjust checkpoint frequency accordingly.

Next Steps
----------

Now that you understand data recovery, explore more advanced topics:

📚 **Continue Learning:**

- :doc:`03-inline-recovery` - Learn about exception-based recovery and callbacks
- :doc:`04-message-logging` - Add message logging for automatic replay
- :doc:`/guides/data-recovery` - Deep dive into data recovery internals

🔨 **Try Advanced Features:**

- :doc:`/howto/partial-checkpoints` - Implement delta checkpointing
- :doc:`/guides/imr-policy` - Understand In-Memory Redundancy policies

📖 **API Reference:**

- :cpp:func:`fenix::data::group_create` - Data group creation
- :cpp:func:`fenix::data::member_create` - Register data members
- :cpp:func:`fenix::data::member_store` - Checkpoint data
- :cpp:func:`fenix::data::member_restore` - Restore data
- :cpp:func:`fenix::data::commit` - Finalize snapshots

Summary
-------

**You've Learned:**

✅ How to create data groups and register data members

✅ When and how to checkpoint application state

✅ How to restore data after rank failures

✅ Using data subsets for partial checkpointing

✅ Best practices for checkpoint frequency and data selection

**Key Takeaways:**

- Data recovery complements process recovery
- Checkpoint frequently enough to limit recomputation
- Always recreate groups after initialization
- Use ``member_create`` for initial ranks, ``member_define`` for recovered ranks
- Data subsets reduce checkpoint overhead for large arrays
- Balance checkpoint frequency vs. overhead vs. recomputation cost

**Comparison: With vs. Without Data Recovery:**

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - Process Recovery Only
     - Process + Data Recovery
   * - Failed Rank State
     - Uninitialized
     - Restored from checkpoint
   * - Work Lost
     - All work since start
     - Work since last checkpoint
   * - Recovery Time
     - Full recomputation
     - Minimal recomputation
   * - Memory Overhead
     - None
     - Checkpoint storage (2-3x data size)
   * - Performance Impact
     - None (until failure)
     - Checkpoint overhead (typically < 5%)

You now have the tools to build fault-tolerant applications that preserve their computational state across failures. In the next tutorial, you'll learn about inline recovery patterns for even more control over fault handling. 🎉
