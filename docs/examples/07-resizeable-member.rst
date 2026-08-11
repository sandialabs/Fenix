Example 7: Resizable Data Members with Modern C++ API
=======================================================

.. contents:: In This Example
   :local:
   :depth: 2

Overview
--------

This example demonstrates how to checkpoint and restore dynamic data structures like ``std::vector`` that can change size during execution. It shows:

- Working with ``FENIX_RESIZEABLE`` data members
- Updating buffer pointers after resize operations
- Using null restore to query stored subset information
- Exception-based recovery with try/catch blocks
- Modern C++ API patterns

**What You'll Learn:**

✓ How to checkpoint ``std::vector`` and other resizable containers

✓ Why and when to update buffer pointers

✓ Using null restore to discover stored data dimensions

✓ Exception-based error handling in C++

✓ Handling multiple checkpoints with different sizes

**Time to Complete:** 20 minutes

**Difficulty:** Intermediate

Location
--------

- **Source:** ``examples/07_resizeable_member/resizeable.cpp``
- **Language:** C++ (uses modern C++ features and STL containers)

Prerequisites
-------------

- Basic understanding of MPI
- C++ knowledge (``std::vector``, exceptions, references)
- Familiarity with Fenix initialization (see :doc:`01-hello-world`)
- Understanding of basic data recovery concepts

The Problem: Resizable Data
----------------------------

Many applications use dynamic data structures whose size changes during execution:

- Growing ``std::vector`` arrays as data is added
- Adaptive mesh refinement with changing mesh sizes
- Dynamic load balancing that redistributes data

**Challenge:** When you restore from a checkpoint, you need to:

1. Know how much data was stored
2. Resize your container to fit the stored data
3. Restore the data into the resized container

This example shows the correct pattern for handling resizable data with Fenix.

Complete Code Walkthrough
--------------------------

Let's examine this example section by section.

1. Headers and Setup
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 1, 8-9

   #include <fenix.hpp>
   #include <mpi.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <signal.h>
   #include <vector>

   using fenix::DataSubset;
   using namespace fenix::data;

**Modern C++ API:**

- Use ``fenix.hpp`` for C++ interface
- Import ``fenix::DataSubset`` for subset operations
- Use ``fenix::data`` namespace for clean API calls

2. Constants and Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   constexpr int kKillID = 2;
   constexpr int my_group = 0;
   constexpr int my_member = 0;
   constexpr int start_time_stamp = 0;
   constexpr int group_depth = 1;
   int errflag;

**Configuration:**

- Kill rank 2 to simulate failure
- Use data group 0 with member 0
- Single checkpoint depth (only keep most recent)

3. Modern Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 4-5

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int num_ranks, rank;
     MPI_Comm_size(res_comm, &num_ranks);
     MPI_Comm_rank(res_comm, &rank);

**Modern Pattern:**

- Use ``fenix::init()`` with designated initializers
- Much cleaner than old ``Fenix_Init()`` with many parameters
- Allocate 1 spare rank for recovery

4. Declare Resizable Data
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 1

   std::vector<int> data;

   bool should_throw = Fenix_get_role() == FENIX_ROLE_RECOVERED_RANK;
   while (true) {
     try {

**Key Point:** Declare ``std::vector`` **outside** the try/catch block so it persists across recovery cycles.

The ``should_throw`` flag triggers exception-based recovery for recovered ranks.

5. Initial Rank Setup
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 5-7, 9-10, 13-16, 18

   if (Fenix_get_role() == FENIX_ROLE_INITIAL_RANK) {
     // Create data group and member
     Fenix_Data_group_create(
       my_group, res_comm, start_time_stamp, group_depth,
       FENIX_DATA_POLICY_IMR, NULL, &errflag
     );
     Fenix_Data_member_create(
       my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
     );

     // First resize: Store 100 elements initialized to -1
     data.resize(100);
     for (int& i : data) i = -1;

     // CRITICAL: Update buffer pointer after resize
     Fenix_Data_member_attr_set(
       my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
       &errflag
     );
     member_store(my_group, my_member, {{0, data.size() - 1}});
     Fenix_Data_commit_barrier(my_group, NULL);

**First Checkpoint (100 elements):**

1. Create data group with ``FENIX_DATA_POLICY_IMR`` (In-Memory Replication)
2. Register member with ``FENIX_RESIZEABLE`` flag—tells Fenix the size can change
3. Resize vector to 100 elements and initialize to -1
4. **Update buffer pointer** with ``Fenix_Data_member_attr_set``
5. Store subset ``[0, 99]`` to checkpoint
6. Commit with barrier (all ranks must call)

.. important::
   **Why update the buffer pointer?**

   When you call ``std::vector::resize()``, the vector may allocate new memory and invalidate the old pointer. Fenix needs to know the current buffer address before storing data.

6. Second Checkpoint with Different Size
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 2-4, 7-10

   // Second resize: Store 50 elements with different values
   data.resize(50);
   int val = 1;
   for (int& i : data) i = val++;

   // Update buffer pointer again
   Fenix_Data_member_attr_set(
     my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(),
     &errflag
   );
   member_store(my_group, my_member, {{0, data.size() - 1}});
   Fenix_Data_commit_barrier(my_group, NULL);

**Second Checkpoint (50 elements):**

1. Resize to 50 elements
2. Fill with values [1, 2, 3, ..., 50]
3. **Update buffer pointer again** (resize may have changed memory location)
4. Store new subset ``[0, 49]``
5. Commit (this overwrites previous checkpoint)

**Key Insight:** Each resize requires updating the buffer pointer. Fenix now knows this checkpoint contains 50 elements, not 100.

7. Inject Failure
^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   if (rank == kKillID) {
     fprintf(stderr, "Doing kill on node %d\n", rank);
     raise(SIGTERM);
   }

Rank 2 kills itself to trigger recovery.

8. Exception-Based Recovery Entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 3

   Fenix_Finalize();
   break;
   } catch (const fenix::CommException& e) {
     const fenix::CommException* err = &e;
     while (true) {
       try {

**Recovery Pattern:**

When failure is detected, Fenix throws ``fenix::CommException``. The outer try/catch catches this and begins recovery.

The inner ``while (true)`` loop with try/catch handles cascading failures during recovery itself.

9. Recreate Data Group
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   fprintf(stderr, "Starting data recovery on rank %d\n", rank);
   if (err->fenix_err != FENIX_SUCCESS) {
     fprintf(stderr, "FAILURE on Fenix Init (%d). Exiting.\n", err->fenix_err);
     exit(1);
   }

   Fenix_Data_group_create(
     my_group, res_comm, start_timestamp, group_depth,
     FENIX_DATA_POLICY_IMR, NULL, &errflag
   );

After recovery, recreate the data group. This is required before restoring members.

10. Null Restore: Query Stored Dimensions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 2-6

   // Do a null restore to get information about the stored subset
   DataSubset stored_subset;
   int ret = member_restore(
     my_group, my_member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST,
     stored_subset
   );
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Rank %d restore failure w/ code %d\n", rank, ret);
     MPI_Abort(MPI_COMM_WORLD, 1);
   }

**Null Restore Pattern:**

This is the key technique for resizable data!

**How It Works:**

- Pass ``nullptr`` as the buffer and ``0`` as size
- Fenix doesn't actually restore any data
- Instead, it returns the stored subset dimensions in ``stored_subset``
- ``stored_subset.max_count()`` tells you how many elements were stored

**Why This Matters:**

You need to know the stored size before you can resize your container to fit the data!

11. Resize to Fit and Restore
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 2, 8-11

   // Resize data to fit all stored data
   data.resize(stored_subset.max_count());

   // Set all data to -2 (for testing, this wasn't stored)
   for (int& i : data) i = -2;

   // Now do an lrestore to get the recovered data
   ret = member_lrestore(
     my_group, my_member, data.data(), data.size(),
     FENIX_DATA_SNAPSHOT_LATEST, stored_subset
   );

   break;

**Restore Process:**

1. Resize vector to match stored size (50 elements)
2. Initialize to -2 (sentinel value, proves restore works)
3. Call ``member_lrestore`` to actually restore data
4. ``lrestore`` uses local restore semantics
5. Break from recovery loop

After this, ``data`` contains [1, 2, 3, ..., 50] restored from checkpoint.

.. note::
   This example uses the **DEPRECATED** ``member_lrestore`` function, which
   only loads from the local snapshot without repairing from redundancy. It works
   in this example because no rank failure occurred, so local snapshots are available.

   **For new code:**

   - Use ``member_load`` instead of ``member_lrestore`` for local-only restore
   - Use ``member_restore`` for recovery after failures (collective repair + load)

   **Key distinctions:**

   - ``member_lrestore``: Local-only, no repair, **deprecated**
   - ``member_restore``: Collective repair from redundancy + load
   - ``member_load``: Modern local-only load (replacement for lrestore)

12. Nested Exception Handling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   } catch (const fenix::CommException& nested) {
     err = &nested;
   }
   }  // end inner while
   }  // end outer catch
   }  // end outer while

**Cascading Failure Handling:**

If another failure occurs **during recovery**, the inner catch handles it and the ``while (true)`` loop retries recovery.

This ensures the application keeps trying to recover even if multiple failures occur close together.

13. Validation
^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   // Ensure data is correct after execution and recovery
   bool successful = data.size() == 50;
   if (!successful)
     printf("Rank %d expected data size 50, but got %ld\n", rank, data.size());

   for (int i = 0; i < data.size() && successful; i++) {
     successful &= data[i] == i + 1;
     if (!successful) {
       printf("Rank %d data[%d]=%d, but should be %d!\n", rank, i, data[i], i + 1);
     }
   }

**Verification:**

- Check that size is 50 (not 100 from first checkpoint)
- Verify data contains [1, 2, 3, ..., 50]
- This proves the second checkpoint was restored correctly

Building and Running
--------------------

Build the Example
^^^^^^^^^^^^^^^^^

From the Fenix build directory:

.. code-block:: bash

   cd examples/07_resizeable_member
   make

Or manually:

.. code-block:: bash

   mpicxx -std=c++17 resizeable.cpp \
     -I$HOME/fenix/include \
     -L$HOME/fenix/lib -lfenix \
     -o resizeable

Run the Example
^^^^^^^^^^^^^^^

.. code-block:: bash

   # Run with 5 total ranks: 4 active + 1 spare
   mpiexec --with-ft mpi -n 5 ./resizeable

**Expected Output:**

.. code-block:: text

   Doing kill on node 2
   Starting data recovery on rank 2
   Rank 0 successfully recovered
   Rank 1 successfully recovered
   Rank 2 successfully recovered
   Rank 3 successfully recovered

**What Happened:**

1. All ranks checkpoint 100 elements (value -1)
2. All ranks checkpoint 50 elements (values 1-50), overwriting previous
3. Rank 2 fails
4. Rank 2 recovers using spare
5. Null restore queries stored size (50 elements)
6. Vector resized to 50
7. Data restored successfully
8. Validation confirms data is correct

Understanding the Recovery Flow
--------------------------------

Let's trace what happens when rank 2 fails:

**Before Failure:**

.. code-block:: text

   Rank | data.size() | data contents | Buffer pointer
   -----+-------------+---------------+---------------
   0    | 50          | [1..50]       | 0x1234
   1    | 50          | [1..50]       | 0x2345
   2    | 50          | [1..50]       | 0x3456 (dies)
   3    | 50          | [1..50]       | 0x4567

**After Recovery (Rank 2):**

.. code-block:: text

   Step | Action                        | data.size() | data contents
   -----+-------------------------------+-------------+--------------
   1    | Exception caught              | 50          | [1..50] (stale)
   2    | Recreate data group           | 50          | [1..50] (stale)
   3    | Null restore                  | 50          | [1..50] (stale)
   4    | stored_subset.max_count() = 50| 50          | [1..50] (stale)
   5    | data.resize(50)               | 50          | [1..50] (may reallocate)
   6    | Fill with -2                  | 50          | [-2..-2] (sentinel)
   7    | member_lrestore()             | 50          | [1..50] (RESTORED!)

**Key Observations:**

- Null restore didn't change data contents
- Resizing ensured container has correct capacity
- Actual restore wrote checkpoint data into resized container

Key Concepts
------------

FENIX_RESIZEABLE Flag
^^^^^^^^^^^^^^^^^^^^^^

When creating a data member:

.. code-block:: cpp

   Fenix_Data_member_create(
     my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT
   );

The ``FENIX_RESIZEABLE`` flag tells Fenix:

- The size of this member can change between checkpoints
- Don't assume the buffer pointer stays constant
- Store size information with each checkpoint

Without this flag, Fenix assumes fixed size and may produce incorrect results or crashes.

Updating Buffer Pointers
^^^^^^^^^^^^^^^^^^^^^^^^^

**Rule:** Call ``Fenix_Data_member_attr_set()`` after **every** ``resize()`` and **before** ``member_store()``.

.. code-block:: cpp

   data.resize(new_size);  // May allocate new memory

   // Update Fenix with new buffer location
   Fenix_Data_member_attr_set(
     my_group, my_member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, data.data(), &errflag
   );

   member_store(my_group, my_member, {{0, data.size() - 1}});

**Why:**

``std::vector::resize()`` may:

- Allocate new memory if capacity exceeded
- Free old memory
- Invalidate pointers to the old buffer

Fenix must know the current buffer address to store data correctly.

Null Restore Pattern
^^^^^^^^^^^^^^^^^^^^^

**Purpose:** Query metadata about stored checkpoint without restoring data.

**Syntax:**

.. code-block:: cpp

   DataSubset stored_subset;
   member_restore(
     group, member,
     nullptr,  // No buffer
     0,        // No size
     FENIX_DATA_SNAPSHOT_LATEST,
     stored_subset  // Output: stored dimensions
   );

**Use Cases:**

- Determine how much memory to allocate before restoring
- Check if stored data matches expected size
- Validate checkpoint before committing to restore
- Implement custom allocation strategies

**Useful Methods:**

- ``stored_subset.max_count()`` - Total number of elements stored
- ``stored_subset.count()`` - Number of ranges in subset
- Access individual ranges if needed

Exception-Based Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^

**Modern Pattern:**

.. code-block:: cpp

   while (true) {
     try {
       // Normal execution
       if (should_throw) {
         should_throw = false;
         fenix::throw_exception();  // Recovered ranks enter recovery
       }

       // Application code
       Fenix_Finalize();
       break;
     } catch (const fenix::CommException& e) {
       // Outer recovery: handle initial failure
       while (true) {
         try {
           // Recovery code
           break;
         } catch (const fenix::CommException& nested) {
           // Inner recovery: handle cascading failures
           continue;
         }
       }
     }
   }

**Benefits:**

- Type-safe error handling (C++ exceptions)
- RAII-compatible (destructors run correctly)
- Clear separation of normal vs. recovery paths
- Handles nested failures gracefully

Best Practices
--------------

1. Always Use FENIX_RESIZEABLE for Dynamic Containers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // GOOD: Explicitly mark as resizable
   std::vector<double> data;
   Fenix_Data_member_create(group, member, data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // BAD: Will fail if size changes
   Fenix_Data_member_create(group, member, data.data(), data.size(), MPI_DOUBLE);

2. Update Buffer Pointer After Every Resize
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // GOOD: Update pointer each time
   data.resize(100);
   Fenix_Data_member_attr_set(group, member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                               data.data(), &errflag);
   member_store(group, member, {{0, 99}});

   data.resize(50);
   Fenix_Data_member_attr_set(group, member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                               data.data(), &errflag);  // May have moved!
   member_store(group, member, {{0, 49}});

   // BAD: Forgot to update pointer
   data.resize(50);
   member_store(group, member, {{0, 49}});  // May store garbage or crash!

3. Use Null Restore Before Resizing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // GOOD: Query size, then resize, then restore
   DataSubset stored_subset;
   member_restore(group, member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST, stored_subset);
   data.resize(stored_subset.max_count());
   member_lrestore(group, member, data.data(), data.size(),
                   FENIX_DATA_SNAPSHOT_LATEST, stored_subset);

   // BAD: Restore into wrong-sized container
   data.resize(100);  // Guessing size
   member_restore(group, member, data.data(), data.size(), FENIX_DATA_SNAPSHOT_LATEST);
   // May overflow or underflow!

4. Handle Cascading Failures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // GOOD: Nested try/catch for cascading failures
   catch (const fenix::CommException& e) {
     while (true) {
       try {
         // Recovery code
         break;
       } catch (const fenix::CommException& nested) {
         continue;  // Retry if recovery fails
       }
     }
   }

   // BAD: Single try/catch
   catch (const fenix::CommException& e) {
     // Recovery code - fails if another failure occurs
   }

5. Validate After Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // GOOD: Verify restored data
   bool valid = (data.size() == expected_size);
   for (int i = 0; i < data.size() && valid; i++) {
     valid &= (data[i] == expected_value(i));
   }
   assert(valid);

Common Patterns
---------------

Pattern: Checkpoint Multiple Vectors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   std::vector<double> x, y, z;

   // Create resizable members
   Fenix_Data_member_create(group, 0, x.data(), FENIX_RESIZEABLE, MPI_DOUBLE);
   Fenix_Data_member_create(group, 1, y.data(), FENIX_RESIZEABLE, MPI_DOUBLE);
   Fenix_Data_member_create(group, 2, z.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // All vectors change to same size
   x.resize(new_size);
   y.resize(new_size);
   z.resize(new_size);

   // Update all buffer pointers
   Fenix_Data_member_attr_set(group, 0, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, x.data(), &err);
   Fenix_Data_member_attr_set(group, 1, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, y.data(), &err);
   Fenix_Data_member_attr_set(group, 2, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, z.data(), &err);

   // Store all
   member_store(group, 0, {{0, x.size() - 1}});
   member_store(group, 1, {{0, y.size() - 1}});
   member_store(group, 2, {{0, z.size() - 1}});
   Fenix_Data_commit_barrier(group, NULL);

Pattern: Struct with Vector
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   struct AppState {
     std::vector<double> data;
     int iteration;
     double residual;
   };

   AppState state;

   // Member 0: The vector (resizable)
   Fenix_Data_member_create(group, 0, state.data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // Member 1: The scalars (fixed size)
   int metadata[2] = {state.iteration, *reinterpret_cast<int*>(&state.residual)};
   Fenix_Data_member_create(group, 1, metadata, 2, MPI_INT);

   // After resize
   state.data.resize(new_size);
   Fenix_Data_member_attr_set(group, 0, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                               state.data.data(), &err);

Pattern: Adaptive Mesh
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   std::vector<Cell> mesh;

   // Mesh refines/coarsens during simulation
   refine_mesh(mesh);  // Changes mesh.size()

   // Update and checkpoint
   Fenix_Data_member_attr_set(group, member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                               mesh.data(), &err);
   member_store(group, member, {{0, mesh.size() - 1}});
   Fenix_Data_commit_barrier(group, NULL);

   // Recovery
   DataSubset stored_subset;
   member_restore(group, member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST, stored_subset);
   mesh.resize(stored_subset.max_count());
   member_lrestore(group, member, mesh.data(), mesh.size(),
                   FENIX_DATA_SNAPSHOT_LATEST, stored_subset);

Exercises
---------

1. **Checkpoint at Different Sizes**

   Modify to checkpoint at three different sizes (100, 75, 50). Verify that recovery uses the most recent (50).

2. **Multiple Failures**

   Add code to kill rank 1 at a different iteration. Does recovery still work correctly?

3. **Add a Second Vector**

   Add ``std::vector<double> other_data`` and checkpoint it alongside ``data``. Restore both during recovery.

4. **Partial Subsets**

   Instead of storing ``{{0, data.size() - 1}}``, store only ``{{10, 30}}`` (a subset). How does null restore behave?

5. **Validation Failure**

   Intentionally break the buffer pointer update. What error messages do you see?

6. **Growing Over Time**

   Make ``data`` grow by 10 elements each iteration. Checkpoint every 5 iterations. Verify recovery gets the right size.

Troubleshooting
---------------

Problem: Segfault During Store
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Symptom:** Crash when calling ``member_store()``

**Cause:** Forgot to update buffer pointer after ``resize()``

**Fix:**

.. code-block:: cpp

   data.resize(new_size);
   Fenix_Data_member_attr_set(group, member, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                               data.data(), &err);
   member_store(group, member, {{0, data.size() - 1}});

Problem: Wrong Data After Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Symptom:** Restored data is garbage or from old checkpoint

**Cause:** Restored into wrong-sized container

**Fix:** Use null restore pattern:

.. code-block:: cpp

   DataSubset stored_subset;
   member_restore(group, member, nullptr, 0, FENIX_DATA_SNAPSHOT_LATEST, stored_subset);
   data.resize(stored_subset.max_count());  // Critical!
   member_lrestore(group, member, data.data(), data.size(),
                   FENIX_DATA_SNAPSHOT_LATEST, stored_subset);

Problem: Forgot FENIX_RESIZEABLE Flag
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Symptom:** Crashes or wrong sizes when restoring

**Fix:**

.. code-block:: cpp

   // Wrong:
   Fenix_Data_member_create(group, member, data.data(), data.size(), MPI_INT);

   // Correct:
   Fenix_Data_member_create(group, member, data.data(), FENIX_RESIZEABLE, MPI_INT);

Next Steps
----------

Now that you understand resizable data members:

📚 **Learn More:**

- :doc:`08-inline-recovery` - Modern THROW or RETURN resume mode (RECOMMENDED)
- :doc:`/guides/data-recovery` - Deep dive into data recovery
- :doc:`/api/data-recovery` - Complete data recovery API

🔨 **Apply It:**

- Checkpoint your application's ``std::vector`` arrays
- Use null restore for dynamic mesh applications
- Combine with THROW or RETURN resume mode from Example 8

📖 **Reference:**

- :c:func:`Fenix_Data_member_create` - Creating resizable members
- :c:func:`Fenix_Data_member_attr_set` - Updating buffer pointers
- :cpp:func:`fenix::data::member_restore` - Null restore pattern
- :cpp:class:`fenix::DataSubset` - Subset query methods

Summary
-------

**You've learned:**

✓ How to checkpoint ``std::vector`` and other dynamic containers

✓ When and why to update buffer pointers

✓ Using null restore to query stored dimensions

✓ Exception-based recovery patterns

✓ Handling multiple checkpoints with different sizes

**Key Takeaways:**

1. **Always use** ``FENIX_RESIZEABLE`` for dynamic data structures
2. **Update buffer pointer** after every ``resize()`` before ``member_store()``
3. **Use null restore** to query stored size before restoring
4. **Nest try/catch** to handle cascading failures
5. **Validate** restored data to catch bugs early

**This pattern extends to:**

- ``std::vector`` of any type
- Custom dynamic data structures
- Adaptive mesh refinement
- Dynamic load balancing
- Any scenario where data size changes during execution
