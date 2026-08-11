Fenix Architecture Overview
===========================

This document provides a high-level architectural view of the Fenix library's three-component design.

.. _architecture-overview:

Three-Component Architecture
-----------------------------

Fenix provides three complementary fault tolerance mechanisms that work together:

.. graphviz::
   :caption: Fenix Three-Component Architecture

   digraph architecture {
       rankdir=TB;
       node [shape=box, style="rounded,filled", fillcolor=lightblue];

       subgraph cluster_fenix {
           label="Fenix Library";
           style=filled;
           fillcolor=lightyellow;

           process [label="1. Process Recovery\n\n• Detect failures\n• Repair communicators\n• Activate spares\n• Resume application", fillcolor=lightgreen];
           data [label="2. Data Recovery\n\n• Checkpoint state\n• Redundant storage\n• Restore on failure", fillcolor=lightcoral];
           message [label="3. Message Recovery\n\n• Log sends/receives\n• Replay messages\n• Local repair", fillcolor=lightblue];
       }

       ulfm [label="ULFM MPI\n\n• Failure detection\n• Communicator ops\n• Error propagation", shape=box, style="rounded,filled", fillcolor=wheat];

       process -> ulfm;
       data -> ulfm;
       message -> ulfm;
   }

Component Details
-----------------

Process Recovery
^^^^^^^^^^^^^^^^

**Purpose:** Handles rank failures by repairing communicators and resuming execution.

**Files:**
  - ``src/fenix_process_recovery.cpp``
  - ``src/fenix.cpp`` (main initialization)

**API:**
  - ``Fenix_Init()`` - Initialize with spares
  - ``Fenix_Finalize()`` - Cleanup

.. graphviz::
   :caption: Process Recovery: Before and After Failure

   digraph process_recovery {
       rankdir=LR;

       subgraph cluster_before {
           label="Normal Operation";
           style=filled;
           fillcolor=lightgreen;

           node [shape=circle, style=filled, fillcolor=green];
           r0a [label="0"];
           r1a [label="1"];
           r2a [label="2"];
           r3a [label="3"];

           node [shape=circle, style=filled, fillcolor=yellow];
           s0a [label="S"];
           s1a [label="S"];

           {rank=same; r0a; r1a; r2a; r3a;}
           {rank=same; s0a; s1a;}
       }

       subgraph cluster_after {
           label="After Rank 2 Fails";
           style=filled;
           fillcolor=lightcoral;

           node [shape=circle, style=filled, fillcolor=green];
           r0b [label="0"];
           r1b [label="1"];
           r2b [label="2'"];
           r3b [label="3"];

           node [shape=circle, style=filled, fillcolor=yellow];
           s0b [label="S"];

           {rank=same; r0b; r1b; r2b; r3b;}

           note [label="Spare promoted\nto replace failed\nrank 2", shape=note, fillcolor=lightyellow];
       }
   }

Data Recovery
^^^^^^^^^^^^^

**Purpose:** In-memory checkpoint/restart with redundant storage.

**Files:**
  - ``src/fenix_data_group.cpp``
  - ``src/fenix_data_member.cpp``
  - ``src/fenix_data_subset.cpp``
  - ``src/fenix_data_buffer.cpp``
  - ``src/fenix_data_policy*.cpp``
  - ``src/data/util/`` (serialization utilities)

**API:**
  - ``Fenix_Data_group_create()``
  - ``Fenix_Data_member_create()``
  - ``Fenix_Data_member_store()``
  - ``Fenix_Data_member_restore()``
  - ``Fenix_Data_commit()``

.. graphviz::
   :caption: Data Recovery Layers

   digraph data_recovery {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       app [label="Application Data\nstruct AppState { int iter; double data[1000]; }", fillcolor=lightblue];

       members [label="Data Members\n(Member 0, Member 1, Member 2)", fillcolor=lightgreen];

       group [label="Data Group\n• Group ID: 0\n• Depth: 3 snapshots\n• Policy: IN_MEMORY_RAID", fillcolor=lightyellow];

       snapshots [label="Snapshots\n(T=0, T=1, T=2)\nRotating buffer", fillcolor=lightcoral];

       storage [label="Redundant Storage\nRAID-style across ranks\n• Each rank stores own data\n• Plus parity for others", fillcolor=wheat];

       app -> members [label="Register"];
       members -> group [label="Group together"];
       group -> snapshots [label="Store"];
       snapshots -> storage [label="Distributed"];

       recovery [label="If rank fails:\nReconstruct from\nsurvivors' parity data", shape=note, fillcolor=pink];
       storage -> recovery [style=dashed];
   }

Message Recovery
^^^^^^^^^^^^^^^^

**Purpose:** Logs sent/received messages for replay after failures (localized recovery).

**Files:**
  - ``src/logging/message_logging.cpp``
  - ``src/logging/comm_log.cpp``
  - ``src/logging/rank_log.cpp``
  - ``src/logging/msg_log.cpp``
  - ``src/logging/mpi_overload.cpp``

**API:**
  - ``Fenix_Message_logging_enable()``
  - ``Fenix_Message_logging_disable()``

.. graphviz::
   :caption: Message Logging System

   digraph message_recovery {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       app [label="Application MPI Calls", fillcolor=lightblue];

       intercept [label="MPI Function\nInterception Layer\n(mpi_overload.cpp)", fillcolor=lightyellow];

       log [label="Log Message", shape=parallelogram, fillcolor=lightgreen];
       mpi [label="Execute Real\nMPI Call", shape=parallelogram, fillcolor=lightgreen];

       storage [label="Message Log\nper rank:\n• Send/Recv history\n• Message data\n• Sequence numbers", fillcolor=lightcoral];

       app -> intercept;
       intercept -> log;
       intercept -> mpi;
       log -> storage;

       recovery [label="On recovery:\n1. Restore checkpoint\n2. Replay messages\n3. Resend lost sends", shape=note, fillcolor=pink];
       storage -> recovery [style=dashed];
   }

Component Interaction
---------------------

.. graphviz::
   :caption: Recovery Process Flow

   digraph recovery_flow {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       failure [label="1. FAILURE\nRank crashes", fillcolor=red, fontcolor=white];

       process_rec [label="2. PROCESS RECOVERY\n• Detect failure (ULFM)\n• Revoke communicator\n• Activate spare rank\n• Rebuild communicator", fillcolor=lightgreen];

       data_rec [label="3. DATA RECOVERY\n(if enabled)\n• Identify new rank\n• Restore checkpoint\n• Reconstruct using\n  redundancy", fillcolor=lightcoral];

       msg_rec [label="4. MESSAGE RECOVERY\n(if enabled)\n• Access message logs\n• Replay messages\n• Resend/rerecv", fillcolor=lightblue];

       resume [label="5. RESUME APPLICATION\n• Return control via\n  longjmp/inline/exception\n• Continue from checkpoint", fillcolor=lightyellow];

       failure -> process_rec;
       process_rec -> data_rec;
       data_rec -> msg_rec;
       msg_rec -> resume;
   }

Component Dependencies
^^^^^^^^^^^^^^^^^^^^^^

.. graphviz::
   :caption: Dependency Hierarchy

   digraph dependencies {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       app [label="Application", fillcolor=lightblue];
       api [label="Fenix C/C++ API", fillcolor=lightyellow];

       process [label="Process Recovery\nRequired", fillcolor=lightgreen];
       data [label="Data Recovery\nOptional", fillcolor=lightcoral];
       message [label="Message Recovery\nOptional", fillcolor=lightblue];

       ulfm [label="ULFM MPI\n(Open MPI 5+)", fillcolor=wheat];

       app -> api;
       api -> process;
       api -> data;
       api -> message;

       process -> ulfm;
       data -> ulfm;
       message -> ulfm;

       note [label="• Process Recovery: ALWAYS active\n• Data Recovery: OPTIONAL\n• Message Recovery: OPTIONAL\n• Can combine Data + Message", shape=note, fillcolor=white];
   }

.. seealso::

   * :doc:`06-data-group-structure` - Detailed data group internals
   * :doc:`08-message-log-structure` - Message logging details
   * :doc:`09-spare-rank-layout` - Spare rank management
