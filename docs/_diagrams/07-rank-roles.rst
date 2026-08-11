Rank Roles and State Machine
============================

Fenix assigns each rank a role that describes its state in the recovery lifecycle.

.. _rank-roles:

Rank Role Types
---------------

.. graphviz::
   :caption: Four Rank Roles in Fenix

   digraph rank_roles {
       rankdir=LR;
       node [shape=box, style="rounded,filled"];

       initial [label="FENIX_ROLE_INITIAL_RANK\n◉\nFirst time through\nNormal startup\nNo recovery needed", fillcolor=lightgreen];

       recovered [label="FENIX_ROLE_RECOVERED_RANK\n◎\nSpare promoted to active\nAfter failure detected\nMust restore state", fillcolor=lightcoral];

       survivor [label="FENIX_ROLE_SURVIVOR_RANK\n◉*\nWas active, still active\nSurvived a failure\nMay help recovery", fillcolor=lightyellow];

       spare [label="FENIX_ROLE_SPARE_RANK\n●\nWaiting to be activated\nIdle in Fenix_Init\nNot part of user comm", fillcolor=lightgray];

       {rank=same; initial; recovered;}
       {rank=same; survivor; spare;}
   }

.. note::
   **Symbol Legend:**
      - ◉ = Active rank
      - ◎ = Recovered rank (promoted spare)
      - ◉* = Survivor rank
      - ● = Spare rank (waiting)

State Transition Diagram
-------------------------

.. graphviz::
   :caption: Rank Role State Transitions

   digraph state_transitions {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       start [label="Application\nStarts", shape=ellipse, fillcolor=lightblue];
       init [label="Fenix_Init()\nDetermine Role", fillcolor=lightyellow];

       spare [label="SPARE_RANK\n●\nwaiting", fillcolor=lightgray];
       initial [label="INITIAL_RANK\n◉\nactive", fillcolor=lightgreen];

       running [label="Application\nruns normally", fillcolor=lightgreen];
       finalize [label="Fenix_Finalize()", shape=ellipse, fillcolor=lightblue];

       failure [label="FAILURE OCCURS!\nRank N fails\n✗", fillcolor=red, fontcolor=white];
       detection [label="Detection &\nRevocation", fillcolor=orange];

       recovered [label="RECOVERED_RANK\n◎\npromoted spare", fillcolor=lightcoral];
       survivor [label="SURVIVOR_RANK\n◉*\nsurvived failure", fillcolor=lightyellow];

       continued [label="Application\ncontinues with\nrepaired comm", fillcolor=lightgreen];

       start -> init [label="Fenix_Init()"];
       init -> spare [label="spare"];
       init -> initial [label="active"];

       spare -> spare [label="Wait in\nFenix_Init()"];
       initial -> running [label="Execute"];
       running -> finalize [label="No failures"];
       finalize -> spare [label="spares\nreleased", style=dashed];

       running -> failure [label="failure"];
       failure -> detection;
       detection -> recovered [label="spare\nactivated"];
       detection -> survivor [label="active\nsurvives"];

       recovered -> continued [label="Restore data"];
       survivor -> continued [label="Continue"];
   }

Detailed Role Descriptions
---------------------------

FENIX_ROLE_INITIAL_RANK
^^^^^^^^^^^^^^^^^^^^^^^

**Who:** Ranks that are active from the start

**When:** First time through Fenix_Init(), before any failures

**Characteristics:**
  - Part of the user communicator
  - Have a rank ID in [0, N-1] where N = active ranks
  - Must initialize application state
  - Should create data groups and members
  - Should take initial checkpoint

**Code Pattern:**

.. code-block:: c

   int role;
   Fenix_Init(&role, MPI_COMM_WORLD, &res_comm, ...);

   if (role == FENIX_ROLE_INITIAL_RANK) {
     // Initialize data
     state.iteration = 0;
     initialize_arrays();

     // Create data recovery structures
     Fenix_Data_group_create(GROUP_ID, ...);
     Fenix_Data_member_create(GROUP_ID, MEMBER_ID, ...);

     // Take initial checkpoint
     Fenix_Data_member_store(GROUP_ID, MEMBER_ID, ...);
     Fenix_Data_commit(GROUP_ID, ...);
   }

FENIX_ROLE_RECOVERED_RANK
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Who:** Spare ranks that were promoted to replace failed ranks

**When:** After Fenix_Init() when recovering from failure

**Characteristics:**
  - Was waiting in Fenix_Init() as spare
  - Promoted to active status
  - Assumes rank ID of failed rank
  - Must restore state from checkpoint
  - Does NOT have callback registrations (must re-register)
  - May have different MPI context than original rank

**Code Pattern:**

.. code-block:: c

   if (role == FENIX_ROLE_RECOVERED_RANK) {
     // Restore data from checkpoint
     Fenix_Data_group_create(GROUP_ID, ...);
     Fenix_Data_member_define(GROUP_ID, MEMBER_ID, ...);
     Fenix_Data_member_restore(GROUP_ID, MEMBER_ID, ...);

     // Re-register callbacks (not preserved from failed rank)
     Fenix_Callback_register(recovery_callback, NULL);

     // Reinitialize MPI-dependent structures
     recreate_mpi_datatypes();
     recreate_mpi_windows();

     printf("Rank %d recovered to iteration %d\\n",
            rank, state.iteration);
   }

.. important::
   Recovered ranks have fresh state:
      - No callbacks registered
      - No MPI context preserved
      - Must recreate MPI objects (windows, datatypes, etc.)
      - Application should checkpoint enough info to fully restore

FENIX_ROLE_SURVIVOR_RANK
^^^^^^^^^^^^^^^^^^^^^^^^^

**Who:** Ranks that were active and survived a failure

**When:** After recovery when using longjmp mode

**Characteristics:**
  - Was active before failure
  - Survived the failure (not the failed rank)
  - Keeps same rank ID
  - Keeps callbacks and MPI context
  - May need to adjust state if using longjmp
  - Checkpointed data is still valid

**Code Pattern (longjmp mode):**

.. code-block:: c

   volatile int seen_failures = 0;

   Fenix_Init(&role, ...);  // ◄─── longjmp returns here

   if (role == FENIX_ROLE_SURVIVOR_RANK) {
     // I survived a failure
     seen_failures++;
     printf("Rank %d survived failure #%d\\n",
            rank, seen_failures);

     // May need to refresh state from checkpoint
     // (longjmp may have corrupted local variables)
     restore_from_checkpoint_if_needed();

     // Callbacks are still registered (don't re-register!)
   }

**Code Pattern (inline mode):**

.. code-block:: c

   // In inline mode, survivors don't go back through Init,
   // so they never see SURVIVOR_RANK role.
   // They just continue with their existing INITIAL_RANK role.

.. note::
   SURVIVOR_RANK only appears in longjmp mode!

FENIX_ROLE_SPARE_RANK
^^^^^^^^^^^^^^^^^^^^^^

**Who:** Ranks allocated as spare ranks, not yet needed

**When:** Initially, and while waiting for failures

**Characteristics:**
  - Blocked inside Fenix_Init()
  - Not part of user communicator
  - No rank ID in user comm (undefined)
  - Waiting to be activated
  - Never executes application code until promoted
  - No callbacks, no state

**Lifecycle:**

.. graphviz::
   :caption: Spare Rank Lifecycle

   digraph spare_lifecycle {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       launch [label="1. Application launches\nwith N+S ranks", fillcolor=lightblue];
       enter [label="2. Spare ranks\nenter Fenix_Init()", fillcolor=lightyellow];
       block [label="3. Fenix_Init()\nblocks spares", fillcolor=lightgray];
       wait [label="4. Spares wait\nfor failure", fillcolor=lightgray];
       failure [label="5. Failure occurs!", fillcolor=red, fontcolor=white];
       promote [label="6. One spare promoted\nto RECOVERED_RANK", fillcolor=lightcoral];
       execute [label="7. Now executes\napplication code", fillcolor=lightgreen];

       launch -> enter;
       enter -> block;
       block -> wait;
       wait -> failure;
       failure -> promote;
       promote -> execute;
   }

.. graphviz::
   :caption: Spare Activation Example

   digraph spare_activation {
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

           label_a [label="User Comm (N=4):\nActive ranks", shape=plaintext];
           label_b [label="Spares (S=2):\nWaiting", shape=plaintext];
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

           label_c [label="User Comm (N=4):\nRank 2' recovered", shape=plaintext];
           label_d [label="Remaining Spares (S=1)", shape=plaintext];

           note [label="Was spare,\nnow recovered", shape=note, fillcolor=lightyellow];
           r2b -> note [style=dashed];
       }
   }

.. warning::
   Spare ranks never see SPARE_RANK role in user code!
   They only see RECOVERED_RANK after promotion.

Role Usage Matrix
-----------------

.. list-table:: When to Create vs. Restore Data
   :header-rows: 1
   :widths: 25 25 25 25

   * - Role
     - When Seen
     - Create Data?
     - Restore Data?
   * - INITIAL_RANK
     - First Init
     - Yes (create)
     - No
   * - RECOVERED_RANK
     - After failure
     - No (define)
     - Yes (restore)
   * - SURVIVOR_RANK
     - Longjmp mode
     - Recreate
     - Maybe (if needed)
   * - SPARE_RANK
     - Never in app
     - N/A
     - N/A

Complete Code Example
---------------------

.. code-block:: c

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     int role, error;
     MPI_Comm res_comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
               &argc, &argv, 2, &error);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     // Application state
     struct {
       int iteration;
       double data[1000];
     } state;

     // Handle different roles
     switch (role) {
       case FENIX_ROLE_INITIAL_RANK:
         printf("Rank %d: Initial rank, starting fresh\\n", rank);

         // Initialize
         state.iteration = 0;
         for (int i = 0; i < 1000; i++) state.data[i] = 0.0;

         // Setup data recovery
         Fenix_Data_group_create(0, res_comm, 0, 1,
                                FENIX_DATA_POLICY_IN_MEMORY_RAID,
                                (int[]){1, 2}, NULL);
         Fenix_Data_member_create(0, 0, &state, sizeof(state), MPI_BYTE);
         Fenix_Data_member_store(0, 0, FENIX_DATA_SUBSET_FULL);
         Fenix_Data_commit(0, NULL);
         break;

       case FENIX_ROLE_RECOVERED_RANK:
         printf("Rank %d: Recovered rank, restoring state\\n", rank);

         // Restore from checkpoint
         Fenix_Data_group_create(0, res_comm, 0, 1,
                                FENIX_DATA_POLICY_IN_MEMORY_RAID,
                                (int[]){1, 2}, NULL);
         Fenix_Data_member_define(0, 0, &state, sizeof(state), MPI_BYTE);
         Fenix_Data_member_restore(0, 0, &state, sizeof(state),
                                  FENIX_DATA_SNAPSHOT_LATEST, NULL);

         printf("Rank %d: Restored to iteration %d\\n",
                rank, state.iteration);
         break;

       case FENIX_ROLE_SURVIVOR_RANK:
         printf("Rank %d: Survivor rank, continuing\\n", rank);
         // May need to refresh state if using longjmp
         break;

       default:
         fprintf(stderr, "Unknown role: %d\\n", role);
         MPI_Abort(res_comm, 1);
     }

     // Application loop (all active ranks execute this)
     for (int i = state.iteration; i < 100; i++) {
       state.iteration = i;

       // Do work...
       for (int j = 0; j < 1000; j++) {
         state.data[j] = state.data[j] * 2.0 + rank;
       }

       // MPI operations
       MPI_Allreduce(MPI_IN_PLACE, state.data, 1000,
                    MPI_DOUBLE, MPI_SUM, res_comm);

       // Checkpoint
       if (i % 10 == 0) {
         Fenix_Data_member_store(0, 0, FENIX_DATA_SUBSET_FULL);
         Fenix_Data_commit(0, NULL);
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Role Changes Over Time
----------------------

Example: 4 ranks with 2 spare ranks, multiple failures

.. list-table::
   :header-rows: 1
   :widths: 10 30 15 15 15 15

   * - Time
     - Event
     - Rank 0
     - Rank 1
     - Rank 2
     - Rank 3
   * - t0
     - Fenix_Init returns
     - INITIAL ◉
     - INITIAL ◉
     - INITIAL ◉
     - INITIAL ◉
   * -
     - Spares (waiting)
     -
     -
     - SPARE ●
     - SPARE ●
   * - t1
     - Rank 2 fails
     - INITIAL ◉
     - INITIAL ◉
     - ✗
     - INITIAL ◉
   * - t2
     - Recovery (longjmp)
     - SURVIVOR ◉*
     - SURVIVOR ◉*
     - RECOVERED ◎
     - SURVIVOR ◉*
   * -
     - Remaining spare
     -
     -
     -
     - SPARE ●
   * - t3
     - Rank 1 fails
     - SURVIVOR ◉*
     - ✗
     - RECOVERED ◎
     - SURVIVOR ◉*
   * - t4
     - Recovery (last spare)
     - SURVIVOR ◉*
     - RECOVERED ◎
     - RECOVERED ◎
     - SURVIVOR ◉*
   * -
     - No spares left!
     - (none)
     -
     -
     -
   * - t5
     - Another failure, NO SPARE!
     - SURVIVOR ◉*
     - RECOVERED ◎
     - ✗
     - SURVIVOR ◉*
   * -
     - ⚠️ Comm shrinks to 3
     - Rank IDs may change!
     -
     -
     -

.. seealso::

   * :doc:`01-basic-recovery-flow` - Recovery process overview
   * :doc:`02-longjmp-recovery` - Longjmp mode behavior
   * :doc:`09-spare-rank-layout` - Spare rank management
