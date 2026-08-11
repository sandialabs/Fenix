Data Group and Member Structure
================================

Fenix organizes checkpoint data into a hierarchical structure of groups, members, and snapshots.

.. _data-group-structure:

Conceptual Hierarchy
--------------------

.. graphviz::
   :caption: Data Recovery Hierarchy

   digraph data_hierarchy {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       app [label="Application Data", fillcolor=lightblue];

       group [label="Data Group 0\n• ID: 0\n• Depth: 3 snapshots\n• Policy: IN_MEMORY_RAID\n• Communicator: res_comm", fillcolor=lightyellow];

       member0 [label="Member 0\n(counter)", fillcolor=lightgreen];
       member1 [label="Member 1\n(array)", fillcolor=lightgreen];
       member2 [label="Member 2\n(matrix)", fillcolor=lightgreen];

       data0 [label="User Data\n(int counter)", fillcolor=lightcoral];
       data1 [label="User Data\n(double array[1000])", fillcolor=lightcoral];
       data2 [label="User Data\n(matrix)", fillcolor=lightcoral];

       app -> group [label="Creates"];
       group -> member0 [label="Contains"];
       group -> member1;
       group -> member2;

       member0 -> data0 [label="Points to"];
       member1 -> data1;
       member2 -> data2;
   }

Data Group Structure
--------------------

.. graphviz::
   :caption: Data Group Internal Structure

   digraph group_structure {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       metadata [label="Group Metadata:\n• Group ID: 0\n• Depth: 3\n• Current Timestamp: 42\n• Policy: IN_MEMORY_RAID\n• Member Count: 3\n• Total Size: 12.5 MB", fillcolor=lightyellow];

       snapshots [label="Snapshot Ring Buffer (depth=3):\n┌─────────────────────────────┐\n│ Snap 40 │ Snap 41 │ Snap 42 │\n│ (oldest)│ (middle)│ (newest)│\n└─────────────────────────────┘", fillcolor=lightcoral, shape=box];

       registry [label="Member Registry:\n┌────────┬────────┬──────────┐\n│ ID | Size | Type      │ Buffer  │\n├────────┼────────┼──────────┤─────────┤\n│  0 |  8B  | MPI_INT  │ 0x7ffe..│\n│  1 |  8KB | MPI_DOUBLE│ 0x7fff..│\n│  2 | 12MB | MPI_DOUBLE│ 0x7ff0..│\n└────────┴────────┴──────────┴─────────┘", fillcolor=lightgreen, shape=box];

       metadata -> snapshots;
       metadata -> registry;
   }

Snapshot Ring Buffer
--------------------

.. graphviz::
   :caption: Ring Buffer with Depth=3

   digraph ring_buffer {
       rankdir=LR;
       node [shape=box, style="rounded,filled"];

       s0 [label="Snapshot T=40\n(oldest)", fillcolor=lightyellow];
       s1 [label="Snapshot T=41\n(middle)", fillcolor=lightyellow];
       s2 [label="Snapshot T=42\n(newest)", fillcolor=lightgreen];

       next [label="Next commit\noverwrites oldest", fillcolor=orange, style=dashed];

       s0 -> s1 -> s2;
       s2 -> next [style=dashed];
       next -> s0 [style=dashed, label="wrap\naround"];
   }

RAID-Style Redundancy
----------------------

.. graphviz::
   :caption: Redundant Storage Across Ranks

   digraph redundancy {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       subgraph cluster_ranks {
           label="Distributed Storage";
           style=filled;
           fillcolor=lightblue;

           r0 [label="Rank 0\n• Data 0\n• Parity 0", fillcolor=lightgreen];
           r1 [label="Rank 1\n• Data 1\n• Parity 1", fillcolor=lightgreen];
           r2 [label="Rank 2\n• Data 2\n• Parity 2", fillcolor=lightgreen];
           r3 [label="Rank 3\n• Data 3\n• Parity 3", fillcolor=lightgreen];
       }

       recover [label="If Rank 2 fails:\nReconstruct from\nData 0,1,3 + Parity", shape=note, fillcolor=pink];
   }

Store/Restore Operations
-------------------------

.. graphviz::
   :caption: Checkpoint and Restore Flow

   digraph store_restore {
       rankdir=LR;

       subgraph cluster_store {
           label="Store (Checkpoint)";
           style=filled;
           fillcolor=lightgreen;

           store_call [label="member_store()"];
           serialize [label="Serialize data"];
           parity [label="Compute parity"];
           distribute [label="Distribute"];
           commit [label="commit()"];

           store_call -> serialize -> parity -> distribute -> commit;
       }

       subgraph cluster_restore {
           label="Restore";
           style=filled;
           fillcolor=lightcoral;

           restore_call [label="member_restore()"];
           find [label="Find snapshot"];
           fetch [label="Fetch data"];
           reconstruct [label="Reconstruct"];
           deserialize [label="Deserialize"];

           restore_call -> find -> fetch -> reconstruct -> deserialize;
       }
   }

API Usage Example
-----------------

.. code-block:: c

   // Create group
   Fenix_Data_group_create(
       0,                               // group_id
       res_comm,                        // communicator
       0,                               // start_time
       3,                               // depth (ring buffer size)
       FENIX_DATA_POLICY_IN_MEMORY_RAID,// policy
       (int[]){1, 2},                   // policy_value (redundancy)
       NULL                             // policy_name
   );

   // Create member (initial rank)
   Fenix_Data_member_create(
       0,                               // group_id
       0,                               // member_id
       &state,                          // buffer pointer
       sizeof(state),                   // count
       MPI_BYTE                         // datatype
   );

   // Store checkpoint
   Fenix_Data_member_store(
       0,                               // group_id
       0,                               // member_id
       FENIX_DATA_SUBSET_FULL          // subset (or custom)
   );

   // Commit
   Fenix_Data_commit_barrier(0, NULL);

   // Restore (recovered rank)
   Fenix_Data_member_define(
       0,                               // group_id
       0,                               // member_id
       &state,                          // buffer pointer
       sizeof(state),                   // count
       MPI_BYTE                         // datatype
   );

   Fenix_Data_member_restore(
       0,                               // group_id
       0,                               // member_id
       &state,                          // target buffer
       sizeof(state),                   // max_count
       FENIX_DATA_SNAPSHOT_LATEST,     // snapshot
       NULL                             // subset
   );

.. seealso::

   * :doc:`10-checkpoint-timeline` - Timing details
   * :doc:`05-architecture` - Overall architecture
   * :doc:`00-quick-reference` - API quick reference
