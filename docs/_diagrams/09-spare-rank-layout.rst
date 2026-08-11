Spare Ranks and Active Ranks Layout
====================================

This document explains how Fenix manages spare ranks alongside active ranks for fault tolerance.

.. _spare-rank-layout:

Initial Configuration
---------------------

When you launch Fenix with N active ranks and S spares (e.g., ``mpiexec -n 6 ./app`` for 4 active + 2 spares):

.. graphviz::
   :caption: Initial Rank Distribution

   digraph initial_config {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       world [label="MPI_COMM_WORLD (6 total ranks)\nRank 0  Rank 1  Rank 2  Rank 3  Rank 4  Rank 5", fillcolor=lightblue];

       init [label="Fenix_Init(..., spares = 2)", shape=diamond, fillcolor=lightyellow];

       user [label="User Communicator (res_comm)\n4 active ranks", fillcolor=lightgreen];
       spare [label="Spare Ranks (internal)\n2 spares waiting", fillcolor=lightgray];

       world -> init [label="split"];
       init -> user;
       init -> spare;
   }

.. graphviz::
   :caption: Active vs Spare Ranks After Initialization

   digraph active_spare {
       rankdir=LR;

       subgraph cluster_active {
           label="User Communicator (res_comm)\nACTIVE - Application executes";
           style=filled;
           fillcolor=lightgreen;

           node [shape=circle, style=filled, fillcolor=green];
           r0 [label="0"];
           r1 [label="1"];
           r2 [label="2"];
           r3 [label="3"];

           {rank=same; r0; r1; r2; r3;}
       }

       subgraph cluster_spare {
           label="Spare Ranks (internal)\nWAITING - Blocked in Fenix_Init()";
           style=filled;
           fillcolor=lightgray;

           node [shape=circle, style=filled, fillcolor=yellow];
           s0 [label="S0"];
           s1 [label="S1"];

           {rank=same; s0; s1;}
       }
   }

Rank Distribution Details
--------------------------

.. graphviz::
   :caption: Characteristics of Active vs Spare Ranks

   digraph rank_details {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       subgraph cluster_active {
           label="Active Ranks (4)";
           style=filled;
           fillcolor=lightgreen;

           a0 [label="Rank 0\n\n• Part of res_comm\n• Has MPI rank ID\n• Executes app code\n• Can checkpoint\n• Registers callbacks", fillcolor=lightgreen];
           a1 [label="Rank 1\n\n• Part of res_comm\n• Has MPI rank ID\n• Executes app code\n• Can checkpoint\n• Registers callbacks", fillcolor=lightgreen];
           a2 [label="Rank 2\n\n• Part of res_comm\n• Has MPI rank ID\n• Executes app code\n• Can checkpoint\n• Registers callbacks", fillcolor=lightgreen];
           a3 [label="Rank 3\n\n• Part of res_comm\n• Has MPI rank ID\n• Executes app code\n• Can checkpoint\n• Registers callbacks", fillcolor=lightgreen];
       }

       subgraph cluster_spare {
           label="Spare Ranks (2)";
           style=filled;
           fillcolor=lightgray;

           s0 [label="Spare 0\n\n• Not in res_comm\n• No app rank ID\n• Waits in Init\n• No state\n• No callbacks", fillcolor=yellow];
           s1 [label="Spare 1\n\n• Not in res_comm\n• No app rank ID\n• Waits in Init\n• No state\n• No callbacks", fillcolor=yellow];

           internal [label="Internal:\nFenix tracks spares\nin internal list\nActivates on demand", shape=note, fillcolor=lightyellow];
       }
   }

Spare Activation Process
-------------------------

Step-by-step: What happens when a rank fails and a spare is activated

.. graphviz::
   :caption: Spare Activation Timeline

   digraph activation_timeline {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       t0 [label="t0: Normal operation\nActive: 0,1,2,3\nSpares: S0,S1", fillcolor=lightgreen];

       t1 [label="t1: Rank 2 crashes ✗", fillcolor=red, fontcolor=white];

       t2 [label="t2: Survivors detect failure\nvia MPI", fillcolor=orange];

       t3 [label="t3: Spares woken up\nNotification sent ⚡", fillcolor=lightyellow];

       t4 [label="t4: All ranks agree\nMPIX_Comm_agree()\nvote 'rank 2 failed'", fillcolor=lightyellow];

       t5 [label="t5: Shrink communicator\nMPIX_Comm_shrink()\nSurvivors only", fillcolor=lightyellow];

       t6 [label="t6: Select spare\nS0: 'I volunteer!'\nS1: 'Keep waiting'", fillcolor=lightyellow];

       t7 [label="t7: Activate spare\nMPIX_Comm_spawn()", fillcolor=lightyellow];

       t8 [label="t8: Merge into comm\nMPI_Intercomm_merge()", fillcolor=lightyellow];

       t9 [label="t9: Renumber ranks\n0→0, 1→1, S0→2, 3→3", fillcolor=lightyellow];

       t10 [label="t10: Recovery complete!\nActive: 0,1,2',3\nSpares: S1", fillcolor=lightgreen];

       t0 -> t1 -> t2 -> t3 -> t4 -> t5 -> t6 -> t7 -> t8 -> t9 -> t10;
   }

.. graphviz::
   :caption: Before and After Spare Activation

   digraph before_after {
       rankdir=LR;

       subgraph cluster_before {
           label="Before Failure";
           style=filled;
           fillcolor=lightgreen;

           node [shape=circle, style=filled, fillcolor=green];
           a0 [label="0"];
           a1 [label="1"];
           a2 [label="2"];
           a3 [label="3"];

           node [shape=circle, style=filled, fillcolor=yellow];
           s0 [label="S0"];
           s1 [label="S1"];

           {rank=same; a0; a1; a2; a3;}
           {rank=same; s0; s1;}
       }

       subgraph cluster_after {
           label="After Rank 2 Fails";
           style=filled;
           fillcolor=lightcoral;

           node [shape=circle, style=filled, fillcolor=green];
           b0 [label="0"];
           b1 [label="1"];
           b2 [label="2'"];
           b3 [label="3"];

           node [shape=circle, style=filled, fillcolor=yellow];
           s1b [label="S1"];

           {rank=same; b0; b1; b2; b3;}

           note [label="S0 promoted\nto replace\nfailed rank 2", shape=note, fillcolor=lightyellow];
           b2 -> note [style=dashed];
       }
   }

Memory Layout
-------------

.. graphviz::
   :caption: Memory Distribution Across Nodes

   digraph memory_layout {
       rankdir=TB;

       subgraph cluster_node1 {
           label="Node 1";
           style=filled;
           fillcolor=lightblue;

           r0_mem [label="Rank 0 (Active)\n• App State\n• Checkpoint\n• Data", shape=box, fillcolor=lightgreen];
           r1_mem [label="Rank 1 (Active)\n• App State\n• Checkpoint\n• Data", shape=box, fillcolor=lightgreen];
           s0_mem [label="Spare 0 (Waiting)\n• Minimal memory\n• No app state", shape=box, fillcolor=yellow];
       }

       subgraph cluster_node2 {
           label="Node 2";
           style=filled;
           fillcolor=lightblue;

           r2_mem [label="Rank 2 (Active)\n• App State\n• Checkpoint\n• Data", shape=box, fillcolor=lightgreen];
           r3_mem [label="Rank 3 (Active)\n• App State\n• Checkpoint\n• Data", shape=box, fillcolor=lightgreen];
           s1_mem [label="Spare 1 (Waiting)\n• Minimal memory\n• No app state", shape=box, fillcolor=yellow];
       }

       note [label="Note: Spares consume\nminimal memory\n(~10% per spare)", shape=note, fillcolor=white];
   }

Communicator Structure
-----------------------

.. graphviz::
   :caption: Communicator Hierarchy

   digraph comm_structure {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       world [label="MPI_COMM_WORLD (all 6 ranks)\n0  1  2  3  4  5", fillcolor=lightblue];

       split [label="Fenix_Init splits", shape=diamond, fillcolor=lightyellow];

       res_comm [label="res_comm (user visible)\nRank 0 (was 0)\nRank 1 (was 1)\nRank 2 (was 2)\nRank 3 (was 3)", fillcolor=lightgreen];

       spare_comm [label="spare_comm (internal)\nRank 0 (was 4)\nRank 1 (was 5)", fillcolor=yellow];

       world -> split;
       split -> res_comm;
       split -> spare_comm;

       after_label [label="After Rank 2 fails:", shape=plaintext];

       res_comm_new [label="res_comm (new)\nRank 0 (was 0)\nRank 1 (was 1)\nRank 2 (was 4) ← Promoted!\nRank 3 (was 3)", fillcolor=lightgreen];

       spare_comm_new [label="spare_comm (remaining)\nRank 0 (was 5)", fillcolor=yellow];

       after_label -> res_comm_new [style=invis];
       after_label -> spare_comm_new [style=invis];
   }

Spare Exhaustion
----------------

What happens when all spares are used?

.. graphviz::
   :caption: Spare Exhaustion Decision Tree

   digraph spare_exhaustion {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       failure [label="Failure detected", fillcolor=red, fontcolor=white];

       check [label="Spares left?", shape=diamond, fillcolor=lightyellow];

       yes_promote [label="Promote spare\nto replace\nfailed rank", fillcolor=lightgreen];

       no_shrink [label="Shrink communicator\n• Remove failed rank\n• Renumber ranks\n• Set warning flag\n• Continue with N-1", fillcolor=orange];

       continue_same [label="Continue with\nsame comm size", fillcolor=lightgreen];

       failure -> check;
       check -> yes_promote [label="YES"];
       check -> no_shrink [label="NO"];
       yes_promote -> continue_same;
   }

**Example progression:**

Configuration: 4 active, 2 spares

.. list-table::
   :widths: 20 40 20 20

   * - **Failure**
     - **Before → After**
     - **Status**
     - **Spares Left**
   * - Failure 1
     - 0,1,2,3 + S0,S1 → 0,1,2',3 + S1
     - ✓ Recovered
     - 1
   * - Failure 2
     - 0,1,2',3 + S1 → 0'',1,2',3 + (none)
     - ✓ Recovered, ⚠️ DEPLETED
     - 0
   * - Failure 3
     - 0'',1,2',3 + (none) → 0'',1,2' (shrunk!)
     - ⚠️ Comm shrinks to 3
     - 0

Optimal Spare Count
-------------------

**Rule of thumb:** 10-20% of active ranks

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Active Ranks
     - Recommended Spares
     - Notes
   * - 10
     - 1-2
     - Small jobs
   * - 100
     - 10-20
     - Medium jobs
   * - 1000
     - 100-200
     - Large jobs

**Considerations:**

- MTBF (mean time between failures)
- Cost of shrinking communicator
- Memory overhead (~10% per spare)
- Likelihood of simultaneous failures

**Example calculation:**

  - Job runs for 24 hours
  - MTBF = 1000 node-hours
  - 100 nodes → expect 2.4 failures
  - **Provision 3-4 spares for safety**

Spare Release
-------------

Spares can be released early if no longer needed:

.. code-block:: c

   Fenix_Spare_release(n_spares_to_release);

.. graphviz::
   :caption: Spare Release Effect

   digraph spare_release {
       rankdir=LR;

       subgraph cluster_before {
           label="Before Release";
           style=filled;
           fillcolor=lightgreen;

           node [shape=circle, style=filled, fillcolor=green];
           a0 [label="0"];
           a1 [label="1"];
           a2 [label="2"];
           a3 [label="3"];

           node [shape=circle, style=filled, fillcolor=yellow];
           s0 [label="S0"];
           s1 [label="S1"];

           {rank=same; a0; a1; a2; a3;}
           {rank=same; s0; s1;}
       }

       subgraph cluster_after {
           label="After Spare Release";
           style=filled;
           fillcolor=lightgreen;

           node [shape=circle, style=filled, fillcolor=green];
           b0 [label="0"];
           b1 [label="1"];
           b2 [label="2"];
           b3 [label="3"];

           {rank=same; b0; b1; b2; b3;}

           note [label="Spares return from\nFenix_Init() and exit", shape=note, fillcolor=lightyellow];
       }
   }

**Use cases:**

- Finishing computation, no more failures expected
- Adaptive spare management
- Resource optimization

Symbol Legend
-------------

.. list-table::
   :widths: 20 80

   * - ◉
     - Active rank
   * - ●
     - Spare rank (waiting)
   * - ◎
     - Recovered rank (was spare)
   * - ✗
     - Failed rank
   * - ⚡
     - Event/notification

.. seealso::

   * :doc:`01-basic-recovery-flow` - Recovery process
   * :doc:`07-rank-roles` - Rank role details
   * :doc:`05-architecture` - Overall architecture
