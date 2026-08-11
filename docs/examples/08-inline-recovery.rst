Example 8: Modern Exception-Based Recovery with Message Logging (RECOMMENDED)
==============================================================================

.. contents:: In This Example
   :local:
   :depth: 2

.. important::
   **This is the recommended modern pattern for Fenix applications!**

   This example demonstrates the most complete and production-ready approach using:

   - Modern C++ API with ``fenix::init()``
   - Exception-based recovery (RESUME_THROW mode)
   - Recovery callbacks for seamless continuation
   - Automatic message logging and replay
   - No longjmp - cleaner control flow

Overview
--------

This example shows a realistic stencil computation with fault tolerance. It demonstrates:

✓ **Modern C++ API** - Clean initialization with designated initializers

✓ **Exception-Based Recovery** - Continue execution without longjmp, using THROW mode

✓ **Message Logging** - Automatic capture and replay of messages

✓ **Exception Handling** - Type-safe recovery with C++ exceptions

✓ **Multiple Failures** - Handles multiple injected failures seamlessly

✓ **Periodic Checkpointing** - Combines data recovery with message logs

**What You'll Learn:**

- The modern way to structure Fenix applications
- How exception-based recovery (THROW mode) works vs. longjmp (JUMP mode)
- Using recovery callbacks for seamless continuation
- Message logging for automatic recovery
- Exception-based error handling with fenix::CommException

**Time to Complete:** 30 minutes

**Difficulty:** Intermediate to Advanced

Location
--------

- **Source:** ``examples/08_inline_recovery/stencil_skeleton.cpp``
- **Language:** C++ (uses C++17 features)

Prerequisites
-------------

- Good understanding of MPI communication patterns
- C++ knowledge (lambdas, exceptions, namespaces)
- Basic Fenix concepts from simpler examples
- Understanding of stencil computations (helpful but not required)

The Application
---------------

This simulates a 1D stencil computation where each rank:

1. Exchanges "ghost cell" data with neighbors (left and right ranks)
2. Performs local computation
3. Checks for convergence via allreduce
4. Periodically checkpoints state
5. Handles failures and recovers inline

The application intentionally injects 3 failures at different iterations to demonstrate recovery.

Complete Code Walkthrough
--------------------------

Let's examine this example section by section.

1. Headers and Modern API
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 2-3

   #include <mpi.h>
   #include <fenix.hpp>
   #include <fenix_util.hpp>

**Key Point:** Use ``fenix.hpp`` for the modern C++ API. This gives you:

- Type-safe interfaces
- Exception-based error handling
- Namespaced functions (``fenix::data``, ``fenix::mlog``)
- RAII wrappers where appropriate

2. Application Constants
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   constexpr int group        = 0;
   constexpr int state_member = 0;
   constexpr int mlogs_member = 1;
   constexpr int mlogs = 2;

   constexpr int app_iterations               = 100;
   constexpr int convergence_check_iterations = 5;
   constexpr int checkpoint_iterations        = 10;
   constexpr int iteration_work_ms            = 10;

This sets up:

- Data group and member IDs
- Message log ID
- Application parameters (100 iterations, checkpoint every 10)

3. Application State
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   struct State {
     int rank = -1, iteration = -1;
   };

Simple state for this example. In real applications, this would be your simulation data (mesh, arrays, etc.).

4. Modern Fenix Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 3-4

   MPI_Init(&argc, &argv);

   MPI_Comm res_world;
   fenix::init({.out_comm = &res_world, .spares = 3});
   assert(fenix::error() == FENIX_SUCCESS);

**Modern Pattern:**

- Use ``fenix::init()`` with **designated initializers** (C++20 style)
- Much cleaner than old ``Fenix_Init()`` with many parameters
- Directly specifies spare count
- Returns resilient communicator via ``.out_comm``

5. Message Logging Setup
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   namespace mlog = fenix::mlog;
   mlog::create(mlogs, res_world, checkpoint_iterations + 1);

**Message Logging:** Fenix can automatically record and replay MPI messages.

- Create message log with ID ``mlogs``
- Hold ``checkpoint_iterations + 1`` regions (sliding window)
- Enables recovery without re-sending messages

6. Initial State Setup
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 5, 9-10, 13-14

   State state;

   if (fenix::role() == fenix::INITIAL_RANK) {
     // Initial ranks initialize state
     state.rank      = rank;
     state.iteration = 0;

     // Create data group and members
     data::group_create(group);
     data::member_create(group, state_member, &state, 2, MPI_INT);
     mlog::create_data_member(mlogs, group, mlogs_member);

     // Store initial checkpoint
     data::member_store(group, SUBSET_FULL);
     data::commit_barrier(group);
   }

**Initial Ranks:** First time through, initialize application state and take first checkpoint.

**Key Functions:**

- ``fenix::role()`` - Returns ``INITIAL_RANK``, ``RECOVERED_RANK``, or ``SURVIVOR_RANK``
- ``data::group_create()`` - Create group for organizing related data members
- ``data::member_create()`` - Register state for checkpointing
- ``mlog::create_data_member()`` - Register message log for checkpointing
- ``data::member_store()`` - Save data to checkpoint
- ``data::commit_barrier()`` - Finalize checkpoint (all ranks must call)

7. Recovery Path (Exception-Based)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 3, 6-10, 12

   } else {
     // Recovered ranks restore from checkpoint
     while (true) {
       try {
         data::group_create(group);

         // member_define is safely idempotent
         data::member_define(group, state_member, &state, 2, MPI_INT);
         mlog::define_data_member(mlogs, group, mlogs_member);

         // Restore from checkpoint
         data::member_restore(group, state_member);
         data::member_restore(group, mlogs_member);

         // Sync message logs to recovered iteration
         mlog::sync(mlogs, state.iteration);
       } catch (fenix::CommException& error) {
         // If recovery fails (another failure during recovery), retry
         continue;
       }
       break;
     }
     printf("Rank %d recovered to iteration %d\n", state.rank, state.iteration);
   }

**Recovery Pattern:**

1. Use ``member_define()`` (idempotent) instead of ``member_create()``
2. Restore data and message logs from checkpoint
3. Sync message log to correct iteration
4. **Wrap in try/catch** - handles cascading failures during recovery
5. Retry loop - keeps trying until recovery succeeds

**Why member_define vs member_create?**

- ``member_create()`` fails if member already exists
- ``member_define()`` is idempotent - safe to call multiple times
- Critical for retry loops during recovery

8. Activate Message Logging
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 2-3

   // Enable automatic message recovery
   fenix::mlog::activate(mlogs);
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

**Activation:** After this point, all MPI communication is:

- Automatically logged
- Automatically replayed after recovery
- Synchronized across ranks

**INLINE_AUTOSYNC mode:** Recovery happens inline (no longjmp), and message logs auto-sync.

9. Register Recovery Callback
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 2-3, 5-7, 9-11

   // Register callback for THROW or RETURN resume mode
   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     assert(fenix::error() == FENIX_SUCCESS);

     // Re-create data group and restore
     data::group_create(group);
     data::member_restore(group, state_member, NULL, 0);
     data::member_restore(group, mlogs_member, NULL, 0);

     printf(
       "Rank %d continuing inline at iteration %d\n", state.rank, state.iteration
     );
   });

**This is the key to THROW or RETURN resume mode!**

**How It Works:**

1. When a failure occurs **during the main loop**, this callback fires
2. Callback restores state from checkpoint
3. Execution continues right where it left off (no longjmp!)
4. The lambda captures ``[&]`` to access local variables

**Benefits:**

- No jumping back to initialization
- Cleaner control flow
- Can continue mid-iteration
- More predictable behavior

10. Main Application Loop
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:
   :emphasize-lines: 4, 6-7, 16-18, 20-21

   for (int i = state.iteration; i < app_iterations; i++) {
     check_inject_failure(state, n_ranks);  // Simulated failures

     // Start message log region for this iteration
     mlog::begin_region(mlogs, i);

     // Exchange with neighbors (standard MPI)
     State left_state, right_state;
     MPI_Sendrecv(
       &state,      2, MPI_INT, right_rank, 0,
       &left_state, 2, MPI_INT, left_rank,  0,
       res_world, MPI_STATUS_IGNORE
     );
     MPI_Sendrecv(/*...*/);

     // Do local computation
     state.iteration++;
     std::this_thread::sleep_for(std::chrono::milliseconds(iteration_work_ms));

     // Periodic convergence check
     if (state.iteration % convergence_check_iterations == 0) {
       double my_part = i, result = -1;
       MPI_Allreduce(&my_part, &result, 1, MPI_DOUBLE, MPI_SUM, res_world);
     }

     // Periodic checkpoint
     if (state.iteration % checkpoint_iterations == 0) {
       data::checkpoint(group, SUBSET_FULL, {mlogs_member});
     }
   }

**Application Loop:**

- Start from ``state.iteration`` (allows continuation after recovery)
- Begin message log region for each iteration
- Standard MPI communication (Sendrecv, Allreduce)
- Update state
- Checkpoint every 10 iterations

**Automatic Recovery:**

When a failure occurs:

1. Message log stops at ``begin_region(i)``
2. Callback fires, restores to last checkpoint
3. Message log syncs and replays messages
4. Loop continues from correct iteration
5. Assertions verify correctness (messages from expected neighbors)

11. Failure Injection
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   void check_inject_failure(State& state, int app_ranks) {
     int rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Use global rank

     bool kill = false;
     kill |= rank == app_ranks / 2 && state.iteration == 18;
     kill |= rank == app_ranks - 1 && state.iteration == 21;
     kill |= rank == 0 && state.iteration == 78;
     if (kill) {
       printf("Rank %d failing at iteration %d\n", rank, state.iteration);
       raise(SIGKILL);
     }
   }

**Three Failures Injected:**

- Middle rank at iteration 18
- Last rank at iteration 21
- Rank 0 at iteration 78

**Why Use Global Rank:** So failures don't repeat after recovery (recovered ranks get new positions in resilient communicator).

Building and Running
--------------------

Build the Example
^^^^^^^^^^^^^^^^^

From the Fenix build directory:

.. code-block:: bash

   cd examples/08_inline_recovery
   make

Or manually:

.. code-block:: bash

   mpicxx -std=c++17 stencil_skeleton.cpp \
     -I$HOME/fenix/include \
     -L$HOME/fenix/lib -lfenix -lmlog \
     -o stencil_skeleton

Run the Example
^^^^^^^^^^^^^^^

.. code-block:: bash

   # Run with 7 ranks total: 4 active + 3 spares
   mpiexec --with-ft mpi -n 7 ./stencil_skeleton

**Expected Output:**

.. code-block:: text

   Rank 2 failing at iteration 18
   Rank 2 recovered to iteration 10
   Rank 2 continuing inline at iteration 10

   Rank 3 failing at iteration 21
   Rank 3 recovered to iteration 20
   Rank 3 continuing inline at iteration 20

   Rank 0 failing at iteration 78
   Rank 0 recovered to iteration 70
   Rank 0 continuing inline at iteration 70

   [All ranks complete iteration 100]

**What Happened:**

1. Each failure triggers recovery from last checkpoint
2. Callback restores state
3. Message logs replay missing messages
4. Computation continues seamlessly
5. All ranks reach iteration 100 despite 3 failures

Understanding the Recovery Flow
--------------------------------

Let's trace what happens when rank 2 fails at iteration 18:

.. code-block:: text

   Time | Rank 0       | Rank 1       | Rank 2       | Rank 3
   -----+--------------+--------------+--------------+--------------
   T0   | Iter 18      | Iter 18      | Iter 18      | Iter 18
   T1   | Iter 18      | Iter 18      | KILLED       | Iter 18
   T2   | Detect fail  | Detect fail  | [dead]       | Detect fail
   T3   | Callback     | Callback     | [dead]       | Callback
        | fires        | fires        |              | fires
   T4   | Restore to   | Restore to   | Spare        | Restore to
        | iter 10      | iter 10      | activated    | iter 10
   T5   | Sync mlogs   | Sync mlogs   | Restore to   | Sync mlogs
        |              |              | iter 10      |
   T6   | Continue     | Continue     | Continue     | Continue
        | at iter 10   | at iter 10   | as new       | at iter 10
        |              |              | rank 2       |
   T7   | Messages     | Messages     | Messages     | Messages
        | replayed     | replayed     | replayed     | replayed
        | 10-17        | 10-17        | 10-17        | 10-17
   T8   | Resume       | Resume       | Resume       | Resume
        | at iter 18   | at iter 18   | at iter 18   | at iter 18

**Key Points:**

1. **No longjmp** - Each rank continues from where it was
2. **Callback handles recovery** - Restores state inline
3. **Message replay** - Iterations 10-17 replayed automatically
4. **All ranks synchronized** - Resume together at iteration 18

Why This Pattern is Superior
-----------------------------

Compared to Traditional Longjmp Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - Longjmp (Old)
     - Inline + Callback (Modern)
   * - Control Flow
     - Jump back to init
     - Continue where you are
   * - Code Structure
     - Single recovery path
     - Recovery callbacks + main loop
   * - Predictability
     - Can be unpredictable
     - Predictable, local recovery
   * - C++ Safety
     - May skip destructors
     - Safe with RAII/exceptions
   * - Performance
     - Restart all work
     - Resume from checkpoint
   * - Debugging
     - Harder to debug
     - Easier to trace

**When to Use Each:**

- **Inline + Callback (Recommended):** Most applications, especially C++
- **Longjmp:** Legacy code, simple restart patterns

Key Takeaways
-------------

✓ **Modern C++ API** - ``fenix::init()``, namespaces, exceptions

✓ **Inline Recovery** - No longjmp, continue execution naturally

✓ **Recovery Callbacks** - Lambda functions for THROW or RETURN resume mode logic

✓ **Message Logging** - Automatic capture and replay

✓ **Exception Safety** - Type-safe error handling with try/catch

✓ **Production Ready** - Handles multiple failures, cascading failures

Best Practices Demonstrated
----------------------------

1. **Use member_define() in Recovery**

   Idempotent, safe for retry loops

2. **Wrap Recovery in try/catch**

   Handle cascading failures during recovery

3. **Checkpoint Periodically**

   Balance overhead vs. recovery time

4. **Use Global Rank for Failure Logic**

   Avoid repeating same failures after recovery

5. **Capture Context in Callback**

   Lambda ``[&]`` gives access to local state

6. **Sync Message Logs After Recovery**

   Ensures consistent message replay

Common Patterns
---------------

State Structure
^^^^^^^^^^^^^^^

.. code-block:: cpp

   struct ApplicationState {
     // Core data
     std::vector<double> mesh_data;
     int current_iteration;
     double residual;

     // Checkpoint this
     void checkpoint(int group, int member_id) {
       data::member_store(group, member_id, SUBSET_FULL);
     }
   };

Recovery Callback Template
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     // 1. Recreate data structures
     data::group_create(MY_GROUP);

     // 2. Restore application state
     data::member_restore(MY_GROUP, STATE_MEMBER, NULL, 0);
     data::member_restore(MY_GROUP, MLOGS_MEMBER, NULL, 0);

     // 3. Application-specific recovery
     recalculate_derived_quantities();

     // 4. Log recovery
     printf("Rank %d recovered\n", my_rank);
   });

Exercises
---------

1. **Modify Checkpoint Frequency**

   Change ``checkpoint_iterations`` to 5 or 20. How does it affect:

   - Recovery time?
   - Memory usage?
   - Total runtime?

2. **Add More Failures**

   Inject additional failures. What happens when:

   - Two ranks fail simultaneously?
   - Failures happen during checkpoint?
   - You run out of spares?

3. **Extend Application State**

   Add a ``std::vector<double>`` to State and checkpoint it.

4. **Try Without Message Logging**

   Comment out message logging. What fails?

5. **Convert to Longjmp**

   Remove callback and use default longjmp recovery. Compare.

Next Steps
----------

Now that you understand modern Fenix patterns:

📚 **Learn More:**

- :doc:`/howto/choose-recovery-pattern` - Deep dive into recovery patterns
- :doc:`/api/message-recovery` - Message logging details
- :doc:`/api/data-recovery` - Complete data recovery API

🔨 **Apply It:**

- :doc:`/migration-checklist` - Migrate your application
- :doc:`/tutorials/03-resume-modes` - Resume modes tutorial
- :doc:`/tutorials/04-message-logging` - Advanced message logging

📖 **Reference:**

- :cpp:func:`fenix::init` - Modern initialization
- :cpp:func:`fenix::callback_register` - Recovery callbacks
- :cpp:func:`fenix::mlog::create` - Message logging
- :cpp:func:`fenix::data::checkpoint` - Simplified checkpointing

Summary
-------

**This example demonstrates the recommended modern pattern for Fenix:**

✅ Clean C++ API with designated initializers

✅ Exception-based error handling

✅ Inline recovery with callbacks (no longjmp)

✅ Automatic message logging and replay

✅ Production-ready: handles multiple and cascading failures

**Use this pattern as a template for your own applications!**
