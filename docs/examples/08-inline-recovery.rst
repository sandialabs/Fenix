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

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [headers]
   :end-before: // [headers]
   :emphasize-lines: 2-3

**Key Point:** Use ``fenix.hpp`` for the modern C++ API. This gives you:

- Type-safe interfaces
- Exception-based error handling
- Namespaced functions (``fenix::data``, ``fenix::mlog``)
- RAII wrappers where appropriate

2. Application Constants
^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [constants]
   :end-before: // [constants]

This sets up:

- Data group and member IDs
- Message log ID
- Application parameters (100 iterations, checkpoint every 10)

3. Application State
^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [state-struct]
   :end-before: // [state-struct]

Simple state for this example. In real applications, this would be your simulation data (mesh, arrays, etc.).

4. Modern Fenix Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [fenix-modern-init]
   :end-before: // [fenix-modern-init]
   :emphasize-lines: 3-4

**Modern Pattern:**

- Use ``fenix::init()`` with **designated initializers** (C++20 style)
- Much cleaner than old ``Fenix_Init()`` with many parameters
- Directly specifies spare count
- Returns resilient communicator via ``.out_comm``

5. Message Logging Setup
^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [mlog-setup]
   :end-before: // [mlog-setup]

**Message Logging:** Fenix can automatically record and replay MPI messages.

- Create message log with ID ``mlogs``
- Hold ``checkpoint_iterations + 1`` regions (sliding window)
- Enables recovery without re-sending messages

6. Initial State Setup
^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [initial-setup]
   :end-before: // [initial-setup]
   :emphasize-lines: 4, 7-8, 11-12

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

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [recovery-path]
   :end-before: // [recovery-path]
   :emphasize-lines: 4, 7-11, 13

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

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [mlog-activate]
   :end-before: // [mlog-activate]
   :emphasize-lines: 3-4

**Activation:** After this point, all MPI communication is:

- Automatically logged
- Automatically replayed after recovery
- Synchronized across ranks

**INLINE_AUTOSYNC mode:** Messages are replayed automatically inline after communicator repair. This allows survivor ranks to continue without reloading checkpoints - only recovered ranks need to restore from the last checkpoint.

9. Register Recovery Callback
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [callback-register]
   :end-before: // [callback-register]
   :emphasize-lines: 3-4, 6-8, 10-12

**Recovery callbacks work with all resume modes** and are required for MLOG_RECOVERY_INLINE modes.

**How It Works:**

1. When a failure occurs **during the main loop**, this callback fires on all ranks
2. Callback recreates data structures that may have been invalidated
3. Survivors continue with their current data; recovered ranks restore from checkpoint
4. The lambda captures ``[&]`` to access local variables

**Why Callbacks Are Used:**

- Recreate data members/groups after communicator repair
- Recovered ranks restore from checkpoint
- Survivor ranks continue with current state (no checkpoint reload needed)
- Required for inline message recovery modes

10. Main Application Loop
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [main-loop]
   :end-before: // [main-loop]
   :emphasize-lines: 5, 7-8, 19-21, 23-24

**Application Loop:**

- Start from ``state.iteration`` (allows continuation after recovery)
- Begin message log region for each iteration
- Standard MPI communication (Sendrecv, Allreduce)
- Update state
- Checkpoint every 10 iterations

**Inline Message Recovery:**

When a failure occurs:

1. MPI function detects failure, throws ``CommException`` (RESUME_THROW mode)
2. Exception propagates to catch block in main
3. Callback fires on all ranks during communicator repair
4. Recovered ranks restore from checkpoint; survivors keep their current state
5. Message log autosync replays only messages needed by recovered ranks
6. Loop continues from each rank's current iteration (survivors continue where they were)
7. Assertions verify correctness (messages from expected neighbors)

11. Failure Injection
^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/08_inline_recovery/stencil_skeleton.cpp
   :language: cpp
   :start-after: // [inject-failure]
   :end-before: // [inject-failure]

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

.. graphviz::
   :align: center
   :caption: Timeline of failure and recovery for rank 2 at iteration 18

   digraph recovery_flow {
       rankdir=TB;
       node [shape=box, style=filled, fontname="monospace", fontsize=13];

       // Time labels
       node [shape=plaintext, fillcolor=none];
       t0 [label="T0"];
       t1 [label="T1"];
       t2 [label="T2"];
       t3 [label="T3"];
       t4 [label="T4"];
       t5 [label="T5"];
       t6 [label="T6"];
       t7 [label="T7"];
       t8 [label="T8"];
       t9 [label="T9"];

       // Rank 0 column
       node [shape=box, style=filled, fillcolor=lightblue];
       r0_t0 [label="Rank 0, Iter 17\nHalo Exchange"];
       r0_t1 [label="Rank 0, Iter 18\nHalo Exchange"];
       r0_t2 [label="Rank 0, Iter 18\nDetect fail\n(REVOKED)"];
       r0_t3 [label="Rank 0, Iter 18\nCallback fires"];
       r0_t4 [label="Rank 0, Iter 18\nRepair without\nrestore"];
       r0_t5 [label="Rank 0, Iter 18\nAutosync mlogs\n@CONTINUE"];
       r0_t6 [label="Rank 0, Iter 18\nNo replay\nneeded"];
       r0_t7 [label="Rank 0, Iter 18\nHalo Exchange\n(auto-retry)"];
       r0_t8 [label="Rank 0, Iter 19\nHalo Exchange\n(delayed)", fillcolor=lightyellow];
       r0_t9 [label="Rank 0, Iter 19\nHalo Exchange\n(cont.)"];

       // Rank 1 column
       r1_t0 [label="Rank 1, Iter 17\nHalo Exchange"];
       r1_t1 [label="Rank 1, Iter 18\nHalo Exchange"];
       r1_t2 [label="Rank 1, Iter 18\nDetect fail\n(PROC_FAILED)"];
       r1_t3 [label="Rank 1, Iter 18\nCallback fires"];
       r1_t4 [label="Rank 1, Iter 18\nRepair without\nrestore"];
       r1_t5 [label="Rank 1, Iter 18\nAutosync mlogs\n@CONTINUE"];
       r1_t6 [label="Rank 1, Iter 18\nReplay msgs\niters 10-17"];
       r1_t7 [label="Rank 1, Iter 18\nHalo Exchange\n(auto-retry)\n(delayed)", fillcolor=lightyellow];
       r1_t8 [label="Rank 1, Iter 18\nHalo Exchange\n(cont.)"];
       r1_t9 [label="Rank 1, Iter 19\nHalo Exchange"];

       // Rank 2 column (with failure)
       node [fillcolor=lightcoral];
       r2_t0 [label="Rank 2, Iter 17\nHalo Exchange"];
       r2_t1 [label="Rank 2, Iter 18\nKILLED", fillcolor=red, penwidth=3];
       r2_t2 [label="[dead]", fillcolor=gray];
       node [fillcolor=lightgreen];
       r2_t3 [label="Rank 2'\nSpare activated"];
       r2_t4 [label="Rank 2', Iter 0\nRestore from\niter 10 ckpt"];
       r2_t5 [label="Rank 2', Iter 10\nSync mlogs\n@Region 10"];
       r2_t6 [label="Rank 2', Iter 10\nRepeat iters\n10-17"];
       // No r2_t7
       r2_t8 [label="Rank 2', Iter 18\nHalo Exchange"];
       r2_t9 [label="Rank 2', Iter 19\nHalo Exchange"];

       // Rank 3 column
       node [fillcolor=lightblue];
       r3_t0 [label="Rank 3, Iter 17\nHalo Exchange"];
       r3_t1 [label="Rank 3, Iter 18\nHalo Exchange"];
       r3_t2 [label="Rank 3, Iter 18\nDetect fail\n(PROC_FAILED)"];
       r3_t3 [label="Rank 3, Iter 18\nCallback fires"];
       r3_t4 [label="Rank 3, Iter 18\nRepair without\nrestore"];
       r3_t5 [label="Rank 3, Iter 18\nAutosync mlogs\n@CONTINUE"];
       r3_t6 [label="Rank 3, Iter 18\nReplay msgs\niters 10-17"];
       r3_t7 [label="Rank 3, Iter 18\nHalo Exchange\n(auto-retry)\n(delayed)", fillcolor=lightyellow];
       r3_t8 [label="Rank 3, Iter 18\nHalo Exchange\n(cont.)"];
       r3_t9 [label="Rank 3, Iter 19\nHalo Exchange"];

       // Vertical flow and horizontal ordering
       {rank=same; t0; r0_t0; r1_t0; r2_t0; r3_t0;}
       {rank=same; t1; r0_t1; r1_t1; r2_t1; r3_t1;}
       {rank=same; t2; r0_t2; r1_t2; r2_t2; r3_t2;}
       {rank=same; t3; r0_t3; r1_t3; r2_t3; r3_t3;}
       {rank=same; t4; r0_t4; r1_t4; r2_t4; r3_t4;}
       {rank=same; t5; r0_t5; r1_t5; r2_t5; r3_t5;}
       {rank=same; t6; r0_t6; r1_t6; r2_t6; r3_t6;}
       {rank=same; t7; r0_t7; r1_t7; r3_t7;}
       {rank=same; t8; r0_t8; r1_t8; r2_t8; r3_t8;}
       {rank=same; t9; r0_t9; r1_t9; r2_t9; r3_t9;}

       // Edges for time progression
       t0 -> t1 -> t2 -> t3 -> t4 -> t5 -> t6 -> t7 -> t8 -> t9 [style=invis];

       // Force left-to-right ordering of ranks
       t0 -> r0_t0 -> r1_t0 -> r2_t0 -> r3_t0 [style=invis];
       t1 -> r0_t1 -> r1_t1 -> r2_t1 -> r3_t1 [style=invis];
       t2 -> r0_t2 -> r1_t2 -> r2_t2 -> r3_t2 [style=invis];
       t3 -> r0_t3 -> r1_t3 -> r2_t3 -> r3_t3 [style=invis];
       t4 -> r0_t4 -> r1_t4 -> r2_t4 -> r3_t4 [style=invis];
       t5 -> r0_t5 -> r1_t5 -> r2_t5 -> r3_t5 [style=invis];
       t6 -> r0_t6 -> r1_t6 -> r2_t6 -> r3_t6 [style=invis];
       t7 -> r0_t7 -> r1_t7 -> r3_t7 [style=invis];
       t8 -> r0_t8 -> r1_t8 -> r2_t8 -> r3_t8 [style=invis];
       t9 -> r0_t9 -> r1_t9 -> r2_t9 -> r3_t9 [style=invis];

       r0_t0 -> r0_t1 -> r0_t2 -> r0_t3 -> r0_t4 -> r0_t5 -> r0_t6 -> r0_t7 [arrowhead=vee];
       r0_t7 -> r0_t8 [arrowhead=vee, style=dashed];
       r0_t8 -> r0_t9 [arrowhead=vee];
       r1_t0 -> r1_t1 -> r1_t2 -> r1_t3 -> r1_t4 -> r1_t5 -> r1_t6 -> r1_t7 [arrowhead=vee];
       r1_t7 -> r1_t8 [arrowhead=vee, style=dashed];
       r1_t8 -> r1_t9 [arrowhead=vee];
       r2_t0 -> r2_t1 -> r2_t2 [arrowhead=vee];
       r2_t3 -> r2_t4 -> r2_t5 -> r2_t6 [arrowhead=vee, color=green, penwidth=2]
       r2_t6 -> r2_t8 [arrowhead=vee, color=green, penwidth=2, style=dashed, overlap=true];
       r2_t8 -> r2_t9 [arrowhead=vee, color=green, penwidth=2];
       r3_t0 -> r3_t1 -> r3_t2 -> r3_t3 -> r3_t4 -> r3_t5 -> r3_t6 -> r3_t7 [arrowhead=vee];
       r3_t7 -> r3_t8 [arrowhead=vee, style=dashed];
       r3_t8 -> r3_t9 [arrowhead=vee];

       // Normal message exchanges at T0 (stencil pattern: each rank exchanges with neighbors)
       r0_t0 -> r1_t0 [color=blue, style=solid, constraint=false];
       r1_t0 -> r0_t0 [color=blue, style=solid, constraint=false];
       r1_t0 -> r2_t0 [color=blue, style=solid, constraint=false];
       r2_t0 -> r1_t0 [color=blue, style=solid, constraint=false];
       r2_t0 -> r3_t0 [color=blue, style=solid, constraint=false];
       r3_t0 -> r2_t0 [color=blue, style=solid, constraint=false];

       // Failed message exchanges at T1 (stencil pattern: each rank exchanges with neighbors)
       r0_t1 -> r1_t1 [color=red, style=dashed, constraint=false];
       r1_t1 -> r0_t1 [color=red, style=dashed, constraint=false];
       r1_t1 -> r2_t1 [color=red, style=dashed, constraint=false];
       r3_t1 -> r2_t1 [color=red, style=dashed, constraint=false];

       // Message replay at T6 (survivors replay to recovered rank)
       r1_t6 -> r2_t6 [color=purple, style=bold, penwidth=2, constraint=false];
       r3_t6 -> r2_t6 [color=purple, style=bold, penwidth=2, constraint=false];

       // Partial message exchanges at T7 (stencil pattern: each rank exchanges with neighbors)
       r0_t7 -> r1_t7 [color=blue, style=solid, constraint=false];
       r1_t7 -> r0_t7 [color=blue, style=solid, constraint=false];

       // Partial message exchanges at T8 (stencil pattern: each rank exchanges with neighbors)
       r1_t8 -> r2_t8 [color=blue, style=solid, constraint=false];
       r2_t8 -> r1_t8 [color=blue, style=solid, constraint=false];
       r2_t8 -> r3_t8 [color=blue, style=solid, constraint=false];
       r3_t8 -> r2_t8 [color=blue, style=solid, constraint=false];

       // Normal message exchanges at T9 (stencil pattern: each rank exchanges with neighbors)
       r0_t9 -> r1_t9 [color=blue, style=solid, constraint=false];
       r1_t9 -> r0_t9 [color=blue, style=solid, constraint=false];
       r1_t9 -> r2_t9 [color=blue, style=solid, constraint=false];
       r2_t9 -> r1_t9 [color=blue, style=solid, constraint=false];
       r2_t9 -> r3_t9 [color=blue, style=solid, constraint=false];
       r3_t9 -> r2_t9 [color=blue, style=solid, constraint=false];

       // Spare activation
       r2_t2 -> r2_t3 [label="spare\nactivates", color=green, penwidth=2, style=dashed];
   }

**Key Points:**

1. **Minimal lost work** - Each survivor continues from its current state without loading a checkpoint - only recovered ranks have to start from the old data.
2. **Callback handles recovery** - Repairs data members inline
3. **Message replay** - Iterations 10-17 replayed automatically
4. **Minimally synchronized** - Global synchronization during repair to decide what messages to replay, then ranks continue without waiting until data patterns demand it (e.g. collective, receives from recovering ranks, ...)

Why This Pattern is Superior
-----------------------------

Compared to Manual Message Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - MLOG_RECOVERY_MANUAL
     - MLOG_RECOVERY_INLINE_AUTOSYNC
   * - Survivor State
     - Must reload checkpoint
     - Continue with current state
   * - Message Replay
     - Manual sync required
     - Automatic replay to recovered ranks
   * - Synchronization
     - All ranks reload together
     - Minimal sync during repair
   * - Performance
     - All ranks pay checkpoint overhead
     - Only recovered ranks reload
   * - Code Complexity
     - Simpler (all ranks same path)
     - More complex (survivors/recovered diverge)
   * - Lost Work
     - All ranks lose work since checkpoint
     - Only recovered ranks lose work

**When to Use Each:**

- **Inline Message Recovery (MLOG_RECOVERY_INLINE_AUTOSYNC):** Performance-critical applications where minimizing lost work is important
- **Manual Message Recovery (MLOG_RECOVERY_MANUAL):** Simpler recovery logic, all ranks follow same path

**Note:** These message recovery modes work with any resume mode (JUMP, RETURN, or THROW).

Key Takeaways
-------------

✓ **Modern C++ API** - ``fenix::init()``, namespaces, exceptions

✓ **Inline Message Recovery** - Survivors continue without reloading checkpoints

✓ **Exception-Based Resume Mode** - Type-safe error handling with try/catch (RESUME_THROW)

✓ **Recovery Callbacks** - Required for inline message recovery, work with all resume modes

✓ **Automatic Message Replay** - Messages to recovered ranks replayed automatically

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

✅ Exception-based resume mode (RESUME_THROW)

✅ Inline message recovery (MLOG_RECOVERY_INLINE_AUTOSYNC) for minimal lost work

✅ Recovery callbacks to handle data member recreation

✅ Automatic message replay to recovered ranks

✅ Production-ready: handles multiple and cascading failures

**Use this pattern as a template for your own applications!**
