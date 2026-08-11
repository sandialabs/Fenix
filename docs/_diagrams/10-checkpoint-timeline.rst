Checkpoint and Recovery Timeline
=================================

This document illustrates the timing and sequencing of checkpoint and restore operations in Fenix.

.. _checkpoint-timeline:

Normal Operation with Checkpoints
----------------------------------

.. graphviz::
   :caption: Checkpoint Timeline During Normal Operation

   digraph checkpoint_timeline {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       t0 [label="t0: App starts\niter=0", fillcolor=lightblue];
       t1 [label="t1: Fenix_Init()\nrole=INITIAL", fillcolor=lightyellow];
       t2 [label="t2: Initialize data\niter=0", fillcolor=lightgreen];
       t3 [label="t3: Create group", fillcolor=lightgreen];
       t4 [label="t4: Register members", fillcolor=lightgreen];
       t5 [label="t5: Initial checkpoint\nT=0 ✓", fillcolor=lightcoral];
       t6 [label="t6-15: Work\niter=0-9", fillcolor=lightgreen];
       t16 [label="t16: Checkpoint\niter=10, T=1 ✓", fillcolor=lightcoral];
       t17 [label="t17-26: Work\niter=10-19", fillcolor=lightgreen];
       t27 [label="t27: Checkpoint\niter=20, T=2 ✓", fillcolor=lightcoral];
       t28 [label="t28-37: Work\niter=20-29", fillcolor=lightgreen];
       t38 [label="t38: Checkpoint\niter=30, T=3 ✓", fillcolor=lightcoral];
       t42 [label="t42: FAILURE! ✗", fillcolor=red, fontcolor=white];

       t0 -> t1 -> t2 -> t3 -> t4 -> t5 -> t6 -> t16 -> t17 -> t27 -> t28 -> t38 -> t42;
   }

Detailed Recovery Timeline
---------------------------

What happens during failure detection and recovery across all ranks:

.. graphviz::
   :caption: Multi-Rank Recovery Timeline

   digraph recovery_timeline {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       subgraph cluster_t38 {
           label="t38: Checkpoint Complete";
           style=filled;
           fillcolor=lightgreen;

           r0_38 [label="Rank 0\niter=30 ✓"];
           r1_38 [label="Rank 1\niter=30 ✓"];
           r2_38 [label="Rank 2\niter=30 ✓"];
           s_38 [label="Spare\n(waiting)"];
       }

       subgraph cluster_t42 {
           label="t42: Failure Occurs";
           style=filled;
           fillcolor=red;

           r0_42 [label="Rank 0\niter=34\nMPI_Allreduce"];
           r1_42 [label="Rank 1\niter=34\nMPI_Allreduce"];
           r2_42 [label="Rank 2\n✗ CRASH!", fillcolor=red, fontcolor=white];
           s_42 [label="Spare\n(waiting)"];
       }

       subgraph cluster_t43 {
           label="t43: Detection";
           style=filled;
           fillcolor=orange;

           r0_43 [label="Rank 0\n❌ PROC_FAILED"];
           r1_43 [label="Rank 1\n❌ PROC_FAILED"];
           s_43 [label="Spare\n(waiting)"];
       }

       subgraph cluster_t44_49 {
           label="t44-t49: Recovery Process (15-50ms)";
           style=filled;
           fillcolor=lightyellow;

           recovery [label="• Revoke comm\n• Agreement\n• Shrink comm\n• Spawn spare\n• Merge comms\n• New comm ✓", shape=note];
       }

       subgraph cluster_t50_53 {
           label="t50-t53: Data Recovery (10-100ms)";
           style=filled;
           fillcolor=lightcoral;

           r0_50 [label="Rank 0\nNo restore\n(survivor)"];
           r1_50 [label="Rank 1\nNo restore\n(survivor)"];
           r2_50 [label="Rank 2'\nRestore!\niter=30"];
       }

       subgraph cluster_t54 {
           label="t54: Resume";
           style=filled;
           fillcolor=lightgreen;

           r0_54 [label="Rank 0\niter=34"];
           r1_54 [label="Rank 1\niter=34"];
           r2_54 [label="Rank 2'\niter=30\nMust redo!"];
       }

       r0_38 -> r0_42 -> r0_43 -> recovery -> r0_50 -> r0_54;
       r1_38 -> r1_42 -> r1_43 -> recovery -> r1_50 -> r1_54;
       r2_38 -> r2_42;
       s_38 -> s_42 -> s_43 -> recovery -> r2_50 -> r2_54;
   }

.. note::
   Rank 2' (recovered) must re-execute iterations 30-34 to catch up with survivors.

Checkpoint Operation Breakdown
-------------------------------

Detailed timing of what happens during a checkpoint operation:

.. graphviz::
   :caption: Checkpoint Operation Timeline

   digraph checkpoint_ops {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       call [label="member_store() called", fillcolor=lightblue];
       validate [label="Validate group/member IDs\n(1-5 μs)", fillcolor=lightyellow];
       serialize [label="Serialize data to buffer\n(10-1000 μs)", fillcolor=lightyellow];
       redundancy [label="Compute parity\n(50-5000 μs)", fillcolor=lightyellow];
       distribute [label="Distribute redundancy\n(100-10000 μs)\nNetwork bound!", fillcolor=orange];
       local [label="Store local snapshot\n(10-100 μs)", fillcolor=lightyellow];
       ret [label="Return (non-blocking)", fillcolor=lightblue];

       app [label="App continues...", fillcolor=lightgreen, style=dashed];

       commit_call [label="commit_barrier() called", fillcolor=lightblue];
       barrier [label="MPI_Barrier\n(100-5000 μs)", fillcolor=orange];
       mark [label="Mark committed\n(1-10 μs)", fillcolor=lightyellow];
       advance [label="Advance timestamp\n(1-5 μs)", fillcolor=lightyellow];
       evict [label="Evict old snapshot\n(10-100 μs)", fillcolor=lightyellow];
       done [label="Return", fillcolor=lightblue];

       call -> validate -> serialize -> redundancy -> distribute -> local -> ret;
       ret -> app [style=dashed];
       app -> commit_call [style=dashed];
       commit_call -> barrier -> mark -> advance -> evict -> done;
   }

**Total time:** ~300 μs to 20 ms (typical: 1-5 ms for 1MB data on 4 ranks)

Restore Operation Breakdown
----------------------------

.. graphviz::
   :caption: Restore Operation Timeline

   digraph restore_ops {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       call [label="member_restore() called", fillcolor=lightblue];
       role [label="Determine rank role\n(1-10 μs)", fillcolor=lightyellow];
       find [label="Find latest snapshot\n(10-50 μs)", fillcolor=lightyellow];
       check [label="Check if local\n(1-5 μs)", fillcolor=lightyellow];
       decision [label="RECOVERED rank?\nNeed remote fetch", shape=diamond, fillcolor=orange];
       fetch [label="Fetch parity data\n(100-10000 μs)\nNetwork I/O", fillcolor=orange];
       reconstruct [label="Reconstruct data\nXOR computation\n(50-5000 μs)", fillcolor=lightyellow];
       deserialize [label="Deserialize to buffer\n(10-1000 μs)", fillcolor=lightyellow];
       validate [label="Validate checksum\n(10-100 μs)", fillcolor=lightyellow, style=dashed];
       done [label="Return", fillcolor=lightblue];

       call -> role -> find -> check -> decision;
       decision -> fetch [label="YES"];
       fetch -> reconstruct -> deserialize -> validate -> done;
   }

**Total time:** ~200 μs to 15 ms (typical: 2-8 ms for 1MB data)

Multiple Failures Timeline
---------------------------

.. graphviz::
   :caption: Timeline with Multiple Failures

   digraph multiple_failures {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       t0 [label="t0: Start\n4 active, 2 spares\n0,1,2,3 + S0,S1", fillcolor=lightblue];
       t10 [label="t10: Checkpoint\n✓", fillcolor=lightcoral];
       t20 [label="t20: Checkpoint\n✓", fillcolor=lightcoral];
       t25 [label="t25: Rank 2 fails ✗\n0,1,X,3 + S0,S1", fillcolor=red, fontcolor=white];
       t26 [label="t26: Recovery\nS0 → Rank 2'\n0,1,2',3 + S1", fillcolor=orange];
       t30 [label="t30: Checkpoint\n✓ All 4 ranks", fillcolor=lightcoral];
       t40 [label="t40: Checkpoint\n✓", fillcolor=lightcoral];
       t45 [label="t45: Rank 1 fails ✗\n0,X,2',3 + S1", fillcolor=red, fontcolor=white];
       t46 [label="t46: Recovery\nS1 → Rank 1'\n0,1',2',3 + (none!)", fillcolor=orange];
       warning [label="⚠️ No spares left!", shape=note, fillcolor=pink];
       t50 [label="t50: Checkpoint\n✓", fillcolor=lightcoral];
       t55 [label="t55: Rank 3 fails ✗\n0,1',2',X + (none!)", fillcolor=red, fontcolor=white];
       t56 [label="t56: Recovery\nNo spare!\nComm shrinks\n0,1',2'", fillcolor=red, fontcolor=white];

       t0 -> t10 -> t20 -> t25 -> t26 -> t30 -> t40 -> t45 -> t46;
       t46 -> warning [style=dashed];
       t46 -> t50 -> t55 -> t56;
   }

Checkpoint Frequency Tradeoffs
-------------------------------

Scenario A: Checkpoint Every Iteration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Frequent Checkpointing (Every Iteration)

   digraph freq_checkpoint {
       rankdir=LR;
       node [shape=box, style=filled];

       i0 [label="0\n✓", fillcolor=lightcoral];
       i1 [label="1\n✓", fillcolor=lightcoral];
       i2 [label="2\n✓", fillcolor=lightcoral];
       i3 [label="3\n✓", fillcolor=lightcoral];
       i4 [label="4\n✓", fillcolor=lightcoral];
       i5 [label="5\n✓", fillcolor=lightcoral];
       i6 [label="6\n✓", fillcolor=lightcoral];
       i7 [label="7\n✗", fillcolor=red, fontcolor=white];
       restore [label="Restore\nfrom 7\nLost: 0", shape=note, fillcolor=lightgreen];

       i0 -> i1 -> i2 -> i3 -> i4 -> i5 -> i6 -> i7 -> restore;
   }

- **Overhead:** HIGH (10-20% of runtime)
- **Lost work:** MINIMAL (0-1 iterations)

Scenario B: Checkpoint Every 5 Iterations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Moderate Checkpointing (Every 5 Iterations)

   digraph mod_checkpoint {
       rankdir=LR;
       node [shape=box, style=filled];

       i0 [label="0\n✓", fillcolor=lightcoral];
       i1_4 [label="1-4", fillcolor=lightgreen];
       i5 [label="5\n✓", fillcolor=lightcoral];
       i6 [label="6", fillcolor=lightgreen];
       i7 [label="7\n✗", fillcolor=red, fontcolor=white];
       restore [label="Restore\nfrom 5\nLost: 2", shape=note, fillcolor=orange];

       i0 -> i1_4 -> i5 -> i6 -> i7 -> restore;
   }

- **Overhead:** MEDIUM (2-5% of runtime)
- **Lost work:** MODERATE (0-4 iterations)

Scenario C: Checkpoint Every 10 Iterations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Infrequent Checkpointing (Every 10 Iterations)

   digraph infreq_checkpoint {
       rankdir=LR;
       node [shape=box, style=filled];

       i0 [label="0\n✓", fillcolor=lightcoral];
       i1_6 [label="1-6", fillcolor=lightgreen];
       i7 [label="7\n✗", fillcolor=red, fontcolor=white];
       restore [label="Restore\nfrom 0\nLost: 7", shape=note, fillcolor=red, fontcolor=white];

       i0 -> i1_6 -> i7 -> restore;
   }

- **Overhead:** LOW (< 1% of runtime)
- **Lost work:** HIGH (0-9 iterations)

Optimal Checkpoint Frequency
-----------------------------

**Formula:**

.. math::

   T_{checkpoint} = \sqrt{2 \times T_{iteration} \times MTBF}

Where:
  - :math:`T_{checkpoint}` = Time between checkpoints
  - :math:`T_{iteration}` = Time per iteration
  - :math:`MTBF` = Mean time between failures

**Example:**

Given:
  - :math:`T_{iteration} = 100` ms
  - :math:`MTBF = 1` hour :math:`= 3,600,000` ms

Calculate:

.. math::

   T_{checkpoint} &= \sqrt{2 \times 100 \times 3,600,000} \\
   &= \sqrt{720,000,000} \\
   &\approx 26,833 \text{ ms} \\
   &\approx 26.8 \text{ seconds} \\
   &\approx 268 \text{ iterations}

**Recommendation:** Checkpoint every 250-300 iterations

Recovery Time Breakdown
------------------------

.. list-table::
   :header-rows: 1
   :widths: 40 20 40

   * - Operation
     - Duration
     - Notes
   * - Failure detection
     - 1-10 ms
     - Next MPI operation
   * - Communicator revocation
     - 5-20 ms
     - Propagate to all ranks
   * - Shrink communicator
     - 5-15 ms
     - Remove failed ranks
   * - Spawn spare
     - 10-30 ms
     - Activate replacement
   * - Merge communicators
     - 5-10 ms
     - Integrate spare
   * - Data restore
     - 10-100 ms
     - Depends on data size
   * - **Total**
     - **30-180 ms**
     - **Typical for small-medium jobs**

Symbol Legend
-------------

.. list-table::
   :widths: 20 80

   * - ✓
     - Checkpoint completed
   * - ✗
     - Failure occurred
   * - ◄─
     - Restored from
   * - ⚠️
     - Warning/Note
   * - →
     - Becomes/Transitions to

.. seealso::

   * :doc:`06-data-group-structure` - Data structure details
   * :doc:`01-basic-recovery-flow` - Recovery process overview
   * :doc:`12-decision-recovery-pattern` - Choosing checkpoint interval
