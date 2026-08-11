Message Logging and Replay Structure
=====================================

Message logging records MPI communication for replay after failures, enabling localized recovery.

.. _message-log-structure:

Overview
--------

.. graphviz::
   :caption: Message Logging Concept

   digraph message_concept {
       rankdir=LR;

       subgraph cluster_without {
           label="Without Message Logging";
           style=filled;
           fillcolor=lightcoral;

           w1 [label="Failure\n→\nGlobal Rollback\n\nLost work: O(N ranks)", shape=box];
       }

       subgraph cluster_with {
           label="With Message Logging";
           style=filled;
           fillcolor=lightgreen;

           w2 [label="Failure\n→\nLocal Replay\n\nLost work: O(1 rank)", shape=box];
       }
   }

Message Log Structure
---------------------

.. graphviz::
   :caption: Per-Rank Message Log

   digraph message_log {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       log [label="Rank 0 Message Log\n(Ring buffer in MPI window)", fillcolor=lightyellow];

       entry0 [label="Entry 0: SEND\n• Seq: 0\n• Dest: Rank 2\n• Tag: 42\n• Count: 100\n• Type: MPI_DOUBLE\n• Data: [copy]", fillcolor=lightgreen];

       entry1 [label="Entry 1: RECV\n• Seq: 1\n• Source: Rank 1\n• Tag: 99\n• Count: 50\n• Type: MPI_INT\n• Data: (not stored)", fillcolor=lightblue];

       log -> entry0;
       log -> entry1;
   }

MPI Interception
----------------

.. graphviz::
   :caption: MPI Function Interception Flow

   digraph interception {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       app [label="Application:\nMPI_Send()", fillcolor=lightblue];

       wrapper [label="Fenix Wrapper:\nCheck if logging enabled", fillcolor=lightyellow];

       log_check [label="Logging\nenabled?", shape=diamond, fillcolor=orange];

       log_it [label="Log message:\n• Record metadata\n• Copy data\n• Store in window", fillcolor=lightgreen];

       real_mpi [label="Execute real\nMPI_Send()", fillcolor=lightcoral];

       ret [label="Return to\napplication", fillcolor=lightblue];

       app -> wrapper;
       wrapper -> log_check;
       log_check -> log_it [label="YES"];
       log_check -> real_mpi [label="NO"];
       log_it -> real_mpi;
       real_mpi -> ret;
   }

Distributed Storage
-------------------

.. graphviz::
   :caption: Message Logs Stored in MPI Windows

   digraph distributed_storage {
       rankdir=TB;

       subgraph cluster_windows {
           label="MPI Windows for Peer Access";
           style=filled;
           fillcolor=lightblue;

           w0 [label="Rank 0 Window\n(log entries)", fillcolor=lightgreen];
           w1 [label="Rank 1 Window\n(log entries)", fillcolor=lightgreen];
           w2 [label="Rank 2 Window\n(log entries)", fillcolor=lightgreen];
           w3 [label="Rank 3 Window\n(log entries)", fillcolor=lightgreen];

           note [label="Each rank can\nread others' logs\nvia MPI_Get", shape=note, fillcolor=lightyellow];
       }
   }

Replay Algorithm
----------------

.. graphviz::
   :caption: Message Replay After Failure

   digraph replay {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       failure [label="Rank 2 fails", fillcolor=red, fontcolor=white];
       recover [label="Rank 2' (recovered)\nRestores checkpoint", fillcolor=orange];
       access [label="Access message logs\nfrom survivors", fillcolor=lightyellow];
       replay [label="Replay logged messages:\n• Re-receive sends from others\n• Resend messages to others", fillcolor=lightgreen];
       catchup [label="Caught up with\nother ranks", fillcolor=lightgreen];

       failure -> recover -> access -> replay -> catchup;
   }

Performance Overhead
--------------------

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Operation
     - Without Logging
     - With Logging
   * - MPI_Send
     - 1.0x
     - 1.1-1.2x
   * - MPI_Recv
     - 1.0x
     - 1.05-1.1x
   * - Overall application
     - Baseline
     - 10-20% overhead
   * - Memory per rank
     - Baseline
     - +10-50 MB

API Usage
---------

.. code-block:: c

   // Enable message logging
   int max_messages = 1000;
   Fenix_Message_logging_enable(res_comm, max_messages);

   // Application runs normally
   for (int i = 0; i < N; i++) {
     // MPI operations are automatically logged
     MPI_Send(buffer, count, MPI_DOUBLE, dest, tag, res_comm);
     MPI_Recv(buffer, count, MPI_DOUBLE, src, tag, res_comm, &status);
   }

   // Disable when no longer needed
   Fenix_Message_logging_disable(res_comm);

When to Use Message Logging
----------------------------

✅ **Good for:**

- Message-heavy applications
- Applications where data checkpointing is expensive
- Localized fault tolerance (one rank fails)
- Communication-intensive codes

⚠️ **Consider tradeoffs:**

- 10-20% runtime overhead
- 10-50 MB memory per rank
- Most beneficial when failures are rare
- Less useful with frequent global operations

.. seealso::

   * :doc:`05-architecture` - Component overview
   * :doc:`10-checkpoint-timeline` - Recovery timing
   * :doc:`00-quick-reference` - API quick reference
