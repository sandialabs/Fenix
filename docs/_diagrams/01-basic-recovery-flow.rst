Basic Recovery Flow
===================

This document illustrates the complete Fenix recovery flow from normal operation through failure detection to resumed execution.

.. _basic-recovery-flow:

Phase-by-Phase Recovery Process
--------------------------------

Phase 1: Normal Operation
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 1: Normal Operation

   digraph phase1 {
       rankdir=TB;
       node [shape=circle, style=filled];

       subgraph cluster_active {
           label="Active Ranks";
           style=filled;
           fillcolor=lightgreen;

           r0 [label="0", fillcolor=green];
           r1 [label="1", fillcolor=green];
           r2 [label="2", fillcolor=green];
           r3 [label="3", fillcolor=green];

           {rank=same; r0; r1; r2; r3;}
       }

       subgraph cluster_spare {
           label="Spare Rank";
           style=filled;
           fillcolor=lightyellow;

           s0 [label="S", fillcolor=yellow];
           wait [label="(waiting in\nFenix_Init)", shape=note, fillcolor=white];
       }

       op [label="MPI_Allreduce()\n✓", shape=box, fillcolor=lightblue];

       r0 -> op [style=invis];
       r1 -> op [style=invis];
       r2 -> op [style=invis];
       r3 -> op [style=invis];
   }

Phase 2: Failure Occurs
^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 2: Failure Occurs

   digraph phase2 {
       rankdir=TB;
       node [shape=circle, style=filled];

       r0 [label="0", fillcolor=green];
       r1 [label="1", fillcolor=green];
       r2 [label="2", fillcolor=red, fontcolor=white];
       r3 [label="3", fillcolor=green];
       s0 [label="S", fillcolor=yellow];

       {rank=same; r0; r1; r2; r3;}

       crash [label="✗ CRASH!\nRank 2 terminates\nunexpectedly", shape=box, fillcolor=red, fontcolor=white];

       r2 -> crash;

       warning [label="⚠️ Rank 2 terminates unexpectedly", shape=note, fillcolor=pink];
   }

Phase 3: Detection
^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 3: Failure Detection

   digraph phase3 {
       rankdir=TB;
       node [shape=circle, style=filled];

       r0 [label="0", fillcolor=green];
       r1 [label="1", fillcolor=green];
       r2 [label="2", fillcolor=red, fontcolor=white];
       r3 [label="3", fillcolor=green];
       s0 [label="S", fillcolor=yellow];

       {rank=same; r0; r1; r2; r3;}

       op [label="MPI_Allreduce()", shape=box, fillcolor=orange];
       error [label="❌ MPI_ERR_PROC_FAILED", shape=box, fillcolor=red, fontcolor=white];

       r0 -> op;
       r1 -> op;
       r3 -> op;
       r2 -> op [style=dashed, color=red, label="missing"];

       op -> error;

       notes [label="• ULFM detects missing rank\n• Returns error to survivors\n• Triggers Fenix error handler", shape=note, fillcolor=lightyellow];
   }

Phase 4: Revocation
^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 4: Communicator Revocation

   digraph phase4 {
       rankdir=TB;
       node [shape=circle, style=filled];

       r0 [label="0", fillcolor=green];
       r1 [label="1", fillcolor=green];
       r2 [label="2", fillcolor=red, fontcolor=white];
       r3 [label="3", fillcolor=green];
       s0 [label="S", fillcolor=yellow];

       {rank=same; r0; r1; r2; r3;}

       revoke [label="MPIX_Comm_revoke(comm)", shape=box, fillcolor=orange];

       r0 -> revoke;
       r1 -> revoke;
       r3 -> revoke;

       revoked [label="⚡ Communicator REVOKED\n(permanent, propagates\nto all future ops)", shape=box, fillcolor=pink];

       revoke -> revoked;

       notes [label="• Ensures all ranks know about failure\n• All future ops on comm will fail", shape=note, fillcolor=lightyellow];
   }

Phase 5: Communicator Repair
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 5: Communicator Repair

   digraph phase5 {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       survivors [label="Survivors:\nRank 0, 1, 3", fillcolor=lightgreen];
       spare [label="Spare:\nActivated", fillcolor=yellow];

       shrink [label="MPIX_Comm_shrink()\nCreate comm with\nsurvivors only", fillcolor=lightyellow];

       spawn [label="MPIX_Comm_spawn()\nActivate spare rank", fillcolor=lightyellow];

       merge [label="MPI_Intercomm_merge()\nMerge spare into comm", fillcolor=lightyellow];

       renumber [label="Rank Renumbering\n0→0, 1→1, S→2, 3→3", fillcolor=lightyellow];

       done [label="✅ New communicator\nwith spare replacing\nfailed rank", fillcolor=lightgreen];

       survivors -> shrink;
       spare -> shrink;
       shrink -> spawn;
       spawn -> merge;
       merge -> renumber;
       renumber -> done;
   }

.. graphviz::
   :caption: Rank Layout After Repair

   digraph phase5_ranks {
       rankdir=LR;
       node [shape=circle, style=filled];

       r0 [label="0", fillcolor=green];
       r1 [label="1", fillcolor=green];
       r2 [label="2'", fillcolor=lightcoral];
       r3 [label="3", fillcolor=green];

       {rank=same; r0; r1; r2; r3;}

       note [label="Rank 2 (new!)\nwas spare", shape=note, fillcolor=lightyellow];
       r2 -> note [style=dashed];

       success [label="✅ Ranks keep same IDs (0,1,2,3)", shape=note, fillcolor=lightgreen];
   }

Phase 6: Application Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 6: Application Recovery Handoff

   digraph phase6 {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       control [label="Control returns to\napplication via:", fillcolor=lightblue];

       longjmp [label="Longjmp:\nJump back to Fenix_Init", fillcolor=lightyellow];
       inline [label="Inline:\nReturn error code from MPI", fillcolor=lightyellow];
       exception [label="Exception:\nThrow fenix::CommException", fillcolor=lightyellow];

       callbacks [label="Callbacks execute\n(if registered)", fillcolor=lightgreen];

       data_recovery [label="Data recovery restores\ncheckpoint (if configured)", fillcolor=lightcoral];

       continue [label="Application continues\nwith repaired communicator", fillcolor=lightgreen];

       control -> longjmp;
       control -> inline;
       control -> exception;

       longjmp -> callbacks;
       inline -> callbacks;
       exception -> callbacks;

       callbacks -> data_recovery;
       data_recovery -> continue;
   }

Phase 7: Resumed Operation
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Phase 7: Normal Operation Resumed

   digraph phase7 {
       rankdir=TB;
       node [shape=circle, style=filled];

       r0 [label="0", fillcolor=green];
       r1 [label="1", fillcolor=green];
       r2 [label="2'", fillcolor=lightcoral];
       r3 [label="3", fillcolor=green];

       {rank=same; r0; r1; r2; r3;}

       op [label="MPI operations continue\non repaired communicator", shape=box, fillcolor=lightblue];
       success [label="✓", shape=circle, fillcolor=lightgreen];

       r0 -> op [style=invis];
       r1 -> op [style=invis];
       r2 -> op [style=invis];
       r3 -> op [style=invis];

       op -> success;

       note [label="✅ Application running normally again", shape=note, fillcolor=lightgreen];
   }

Complete Recovery Flow
-----------------------

.. graphviz::
   :caption: End-to-End Recovery Flow

   digraph complete_flow {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       normal [label="Phase 1:\nNormal Operation\nAll ranks working", fillcolor=lightgreen];

       failure [label="Phase 2:\nFailure Occurs\nRank crashes", fillcolor=red, fontcolor=white];

       detection [label="Phase 3:\nDetection\nMPI_ERR_PROC_FAILED", fillcolor=orange];

       revocation [label="Phase 4:\nRevocation\nMPIX_Comm_revoke()", fillcolor=orange];

       repair [label="Phase 5:\nCommunicator Repair\nShrink, Spawn, Merge", fillcolor=lightyellow];

       app_recovery [label="Phase 6:\nApplication Recovery\nCallbacks, Data restore", fillcolor=lightcoral];

       resumed [label="Phase 7:\nResumed Operation\nBack to normal", fillcolor=lightgreen];

       normal -> failure;
       failure -> detection;
       detection -> revocation;
       revocation -> repair;
       repair -> app_recovery;
       app_recovery -> resumed;

       resumed -> failure [label="Another\nfailure", style=dashed, color=gray];
   }

Recovery Timeline
-----------------

Typical timing for each phase:

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Phase
     - Duration
     - Notes
   * - 1. Normal Operation
     - N/A
     - Until failure occurs
   * - 2. Failure
     - Instant
     - Process terminates
   * - 3. Detection
     - 1-10 ms
     - Next MPI operation detects
   * - 4. Revocation
     - 5-20 ms
     - Propagate to all ranks
   * - 5. Comm Repair
     - 15-50 ms
     - Shrink, spawn, merge
   * - 6. App Recovery
     - 10-100 ms
     - Depends on checkpoint size
   * - 7. Resume
     - Instant
     - Back to normal

**Total recovery time:** typically 30-180 ms for small to medium jobs

Symbol Legend
-------------

.. list-table::
   :widths: 20 80

   * - ◉
     - Active rank
   * - ●
     - Spare rank (waiting)
   * - ✗
     - Failed rank
   * - ⚠️
     - Warning/Error
   * - ✅
     - Success
   * - ❌
     - Failure detected
   * - ⚡
     - System event

.. seealso::

   * :doc:`02-longjmp-recovery` - Longjmp mode details
   * :doc:`03-inline-recovery` - Inline recovery details
   * :doc:`04-exception-recovery` - Exception recovery details
   * :doc:`07-rank-roles` - Rank role state machine
