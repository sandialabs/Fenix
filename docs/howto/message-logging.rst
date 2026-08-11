Setting Up Message Logging
==========================

Learn how to use Fenix's message logging system to automatically replay MPI communications after failures, enabling localized recovery without global rollback.

.. contents:: Quick Jump
   :local:
   :depth: 2

What is Message Logging?
-------------------------

Message logging records MPI communication operations so they can be replayed after a failure. This enables **sender-based message logging**, where:

- Non-failed ranks can replay messages to recovered ranks
- Recovered ranks don't need to recompute everything
- Only failed ranks need to roll back to their checkpoint

Benefits
~~~~~~~~

- **Faster recovery**: Only failed ranks roll back
- **Less wasted work**: Survivor ranks continue from current state
- **Automatic replay**: Fenix handles message replay transparently
- **Localized failures**: Failures affect only the failed rank's work

When to Use Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use message logging when:

- Your app does heavy computation between communications
- Failures are infrequent (logging has overhead)
- You want faster recovery than full rollback
- Your communication patterns are recordable

Don't use message logging when:

- Communication dominates computation (overhead too high)
- Failures are very frequent
- Memory is severely constrained (logs consume memory)
- You need the simplest possible solution

Prerequisites
-------------

- Fenix built with message logging support (default)
- Working knowledge of checkpointing (:doc:`checkpoint-data`)
- Understanding of MPI communication patterns

Basic Message Logging
----------------------

Minimal Example
~~~~~~~~~~~~~~~

Here's a simple example showing message logging basics:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   constexpr int GROUP = 0;
   constexpr int DATA_MEMBER = 0;
   constexpr int MLOG_MEMBER = 1;
   constexpr int MLOG_ID = 0;

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank, n_ranks;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &n_ranks);

     // Create message log (keep 10 regions)
     fenix::mlog::create(MLOG_ID, res_comm, /*depth=*/10);

     // Set up application state
     int iteration = 0;

     if (fenix::role() == fenix::INITIAL_RANK) {
       // Initial ranks: create checkpoint structures
       fenix::data::group_create(GROUP);
       fenix::data::member_create(GROUP, DATA_MEMBER, &iteration, 1, MPI_INT);
       fenix::mlog::create_data_member(MLOG_ID, GROUP, MLOG_MEMBER);

       fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL);
     } else {
       // Recovered ranks: restore from checkpoint
       fenix::data::group_create(GROUP);
       fenix::data::member_restore(GROUP, DATA_MEMBER);
       fenix::mlog::create_data_member(MLOG_ID, GROUP, MLOG_MEMBER);
       fenix::data::member_restore(GROUP, MLOG_MEMBER);

       // Sync message logs - replay missing messages
       fenix::mlog::sync(MLOG_ID, iteration);
     }

     // Enable inline recovery
     fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

     // Main loop with logging
     for (iteration = 0; iteration < 100; iteration++) {
       // Activate logging for this iteration
       fenix::mlog::activate(MLOG_ID, iteration);

       // All MPI communication in this region is logged
       int send_data = rank * 100 + iteration;
       int recv_data = 0;
       int next_rank = (rank + 1) % n_ranks;
       int prev_rank = (rank + n_ranks - 1) % n_ranks;

       MPI_Sendrecv(&send_data, 1, MPI_INT, next_rank, 0,
                    &recv_data, 1, MPI_INT, prev_rank, 0,
                    res_comm, MPI_STATUS_IGNORE);

       // Checkpoint every 10 iterations
       if (iteration % 10 == 0) {
         fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL,
                                 /*storev_ids=*/{MLOG_MEMBER});
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

This example shows the complete pattern: create log, checkpoint it with your data, activate it during work, and it automatically handles recovery.

Step-by-Step Guide
------------------

Step 1: Create a Message Log
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create a message log object:

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   constexpr int LOG_ID = 0;  // Unique ID for this log
   constexpr int DEPTH = 10;  // Keep 10 regions in memory

   mlog::create(LOG_ID, resilient_comm, DEPTH);

The ``depth`` parameter controls how many regions (windows of logged messages) to keep in memory. Larger depth uses more memory but allows recovery from older checkpoints.

Rule of thumb: ``depth = 2 * checkpoint_frequency``

Step 2: Create a Data Member for the Log
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To checkpoint the message log state, create a data member for it:

.. code-block:: cpp

   constexpr int GROUP = 0;
   constexpr int MLOG_MEMBER = 99;

   // After creating your data group
   fenix::data::group_create(GROUP);

   // Link the log to a data member
   mlog::create_data_member(LOG_ID, GROUP, MLOG_MEMBER);

This allows you to checkpoint and restore the message log along with your application data.

Step 3: Activate the Log
~~~~~~~~~~~~~~~~~~~~~~~~~

Before MPI communication, activate the log:

.. code-block:: cpp

   // Activate log and start region 0
   mlog::activate(LOG_ID, /*region_id=*/0);

   // Now all MPI communication is logged
   MPI_Send(buf, count, MPI_INT, dest, tag, res_comm);
   MPI_Recv(buf, count, MPI_INT, src, tag, res_comm, &status);

Only the currently active log records messages. Deactivate by activating ``FENIX_MLOG_NONE``:

.. code-block:: cpp

   mlog::activate(FENIX_MLOG_NONE);  // Stop logging

Step 4: Use Regions to Match Checkpoints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Regions organize logged messages into windows that correspond to your application's logical phases:

.. code-block:: cpp

   for (int iteration = 0; iteration < 1000; iteration++) {
     // Start new region for this iteration
     mlog::activate(LOG_ID, iteration);

     // ... MPI communication ...

     // Checkpoint every 10 iterations
     if (iteration % 10 == 0) {
       fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL,
                               /*storev_ids=*/{MLOG_MEMBER});
     }
   }

When restoring, you'll sync to the region corresponding to your checkpoint.

Step 5: Enable Inline Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For automatic recovery without interrupting application control-flow:

.. code-block:: cpp

   // Enable inline recovery
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

With this setting:

- MPI errors trigger automatic recovery
- Message logs sync automatically after recovery
- Your application continues inline (no exception, longjmp, or returned error code)

Step 6: Restore and Sync on Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a rank recovers, restore the log and sync:

.. code-block:: cpp

   if (fenix::role() != fenix::INITIAL_RANK) {
     // Recovered or survivor rank

     // Restore application state
     fenix::data::group_create(GROUP);
     fenix::data::member_restore(GROUP, DATA_MEMBER);

     // Restore message log state
     fenix::mlog::create_data_member(LOG_ID, GROUP, MLOG_MEMBER);
     fenix::data::member_restore(GROUP, MLOG_MEMBER);

     // Sync logs - replay messages to get back in sync
     // Use the iteration we checkpointed at
     mlog::sync(LOG_ID, checkpointed_iteration);

     printf("Rank %d recovered to iteration %d\n", rank, checkpointed_iteration);
   }

``mlog::sync()`` is collective and replays messages from all ranks to bring everyone to a consistent state.

Complete Working Example
-------------------------

Stencil Computation with Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This example shows a 1D stencil computation with message logging:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <signal.h>

   constexpr int GROUP = 0;
   constexpr int STATE_MEMBER = 0;
   constexpr int ARRAY_MEMBER = 1;
   constexpr int MLOG_MEMBER = 2;
   constexpr int LOG_ID = 0;

   struct State {
     int rank;
     int iteration;
   };

   void inject_failure(int rank, int iteration) {
     // Fail rank 1 at iteration 25
     if (rank == 1 && iteration == 25) {
       printf("Rank %d: Injecting failure at iteration %d\n", rank, iteration);
       fflush(stdout);
       raise(SIGKILL);
     }
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank, n_ranks;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &n_ranks);

     // Create message log (keep 20 regions)
     fenix::mlog::create(LOG_ID, res_comm, 20);

     // Application state
     State state;
     const int local_size = 100;
     std::vector<double> data(local_size);

     if (fenix::role() == fenix::INITIAL_RANK) {
       // Initialize
       state.rank = rank;
       state.iteration = 0;

       for (int i = 0; i < local_size; i++) {
         data[i] = rank * 1000.0 + i;
       }

       // Create checkpoint structures
       fenix::data::group_create(GROUP);
       fenix::data::member_create(GROUP, STATE_MEMBER, &state,
                                  2, MPI_INT);
       fenix::data::member_create(GROUP, ARRAY_MEMBER, data.data(),
                                  local_size, MPI_DOUBLE);
       fenix::mlog::create_data_member(LOG_ID, GROUP, MLOG_MEMBER);

       // Initial checkpoint
       fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL,
                               {MLOG_MEMBER});
     } else {
       // Recovery
       printf("Rank %d: Recovering from failure\n", rank);

       // Restore state
       fenix::data::group_create(GROUP);
       fenix::data::member_restore(GROUP, STATE_MEMBER);

       data.resize(local_size);
       fenix::data::member_restore(GROUP, ARRAY_MEMBER);

       // Restore and sync message log
       fenix::mlog::create_data_member(LOG_ID, GROUP, MLOG_MEMBER);
       fenix::data::member_restore(GROUP, MLOG_MEMBER);

       mlog::sync(LOG_ID, state.iteration);

       printf("Rank %d: Recovered to iteration %d\n", rank, state.iteration);
     }

     // Enable inline recovery
     fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

     // Computation loop
     const int left_rank = (rank + n_ranks - 1) % n_ranks;
     const int right_rank = (rank + 1) % n_ranks;

     for (state.iteration; state.iteration < 100; state.iteration++) {
       inject_failure(rank, state.iteration);

       // Activate logging for this iteration
       mlog::activate(LOG_ID, state.iteration);

       // Exchange boundary data
       double left_boundary = data[0];
       double right_boundary = data[local_size - 1];
       double left_neighbor, right_neighbor;

       MPI_Sendrecv(&right_boundary, 1, MPI_DOUBLE, right_rank, 0,
                    &left_neighbor, 1, MPI_DOUBLE, left_rank, 0,
                    res_comm, MPI_STATUS_IGNORE);

       MPI_Sendrecv(&left_boundary, 1, MPI_DOUBLE, left_rank, 0,
                    &right_neighbor, 1, MPI_DOUBLE, right_rank, 0,
                    res_comm, MPI_STATUS_IGNORE);

       // Update interior points (simple averaging)
       std::vector<double> new_data(local_size);
       new_data[0] = (left_neighbor + data[1]) / 2.0;
       for (int i = 1; i < local_size - 1; i++) {
         new_data[i] = (data[i-1] + data[i+1]) / 2.0;
       }
       new_data[local_size - 1] = (data[local_size - 2] + right_neighbor) / 2.0;

       data = new_data;

       // Checkpoint every 10 iterations
       if (state.iteration % 10 == 0) {
         fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL,
                                 {MLOG_MEMBER});
       }
     }

     printf("Rank %d: Completed all iterations\n", rank);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Run with:

.. code-block:: bash

   mpicxx -std=c++20 stencil.cpp -o stencil -lfenix
   mpiexec --with-ft mpi -n 4 ./stencil

Advanced Patterns
-----------------

Manual Recovery Mode
~~~~~~~~~~~~~~~~~~~~

If you want full control over recovery:

.. code-block:: cpp

   // Use manual recovery mode
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::MANUAL);

   // In your recovery callback
   fenix::callback_register([&](MPI_Comm repaired, int mpi_err) {
     // Restore application data
     fenix::data::group_create(GROUP);
     fenix::data::member_restore(GROUP, STATE_MEMBER);
     fenix::mlog::create_data_member(LOG_ID, GROUP, MLOG_MEMBER);
     fenix::data::member_restore(GROUP, MLOG_MEMBER);

     // Manually sync logs
     mlog::sync(LOG_ID, state.iteration);

     printf("Manual recovery complete\n");
   });

Multiple Message Logs
~~~~~~~~~~~~~~~~~~~~~~

You can have multiple logs for different communication patterns:

.. code-block:: cpp

   constexpr int HALO_LOG = 0;
   constexpr int COLLECTIVE_LOG = 1;

   mlog::create(HALO_LOG, res_comm, 10);
   mlog::create(COLLECTIVE_LOG, res_comm, 5);

   // Activate different logs at different times
   mlog::activate(HALO_LOG, iteration);
   // ... halo exchange ...

   mlog::activate(COLLECTIVE_LOG, iteration);
   // ... collective operations ...

   mlog::activate(FENIX_MLOG_NONE);  // Stop logging

Conditional Logging
~~~~~~~~~~~~~~~~~~~

Log only specific iterations:

.. code-block:: cpp

   for (int iter = 0; iter < 1000; iter++) {
     // Only log expensive iterations
     if (iter % 5 == 0) {
       mlog::activate(LOG_ID, iter);
     } else {
       mlog::activate(FENIX_MLOG_NONE);
     }

     // ... work ...
   }

Integration with Checkpointing
-------------------------------

Coordinating Logs and Checkpoints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Key principle: Always checkpoint the message log with your data:

.. code-block:: cpp

   // Store members individually
   fenix::data::member_store(GROUP, STATE_MEMBER);
   fenix::data::member_store(GROUP, DATA_MEMBER);

   // Store log state (use storev when subsets vary per rank)
   fenix::data::member_storev(GROUP, MLOG_MEMBER,
                              fenix::data::SUBSET_FULL);

   // Commit all together
   fenix::data::commit_barrier(GROUP);

Or use the convenience function:

.. code-block:: cpp

   // Checkpoint everything, with log using storev
   fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL,
                           /*num_storev=*/1,
                           /*storev_ids=*/{MLOG_MEMBER},
                           &timestamp);

Continue After Checkpoint
~~~~~~~~~~~~~~~~~~~~~~~~~~

Important: After checkpointing, continue in the same region:

.. code-block:: cpp

   int iteration = 0;

   while (iteration < 1000) {
     // Activate region BEFORE checkpoint
     mlog::activate(LOG_ID, iteration);

     // Checkpoint (log is still active)
     if (iteration % 10 == 0) {
       fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL,
                               {MLOG_MEMBER});
     }

     // Continue work in same region
     // ...

     // Only move to next region at end of iteration
     iteration++;
   }

Using FENIX_MLOG_CONTINUE
~~~~~~~~~~~~~~~~~~~~~~~~~~

When syncing, ``FENIX_MLOG_CONTINUE`` means "restore to my latest logged state":

.. code-block:: cpp

   // Restore to most recent region and continue from latest message
   mlog::sync(LOG_ID, FENIX_MLOG_CONTINUE);

This is useful when you want to continue from wherever the log was, rather than restarting a region.

Performance Considerations
--------------------------

Memory Usage
~~~~~~~~~~~~

Message logs consume memory. Each logged message stores:

- Message envelope (source, dest, tag, size)
- Message data copy

Estimate: ``memory = avg_msg_size * messages_per_region * depth``

To reduce memory:

1. Reduce log depth
2. Log fewer regions
3. Checkpoint more frequently

.. code-block:: cpp

   // Keep fewer regions (less memory, shorter rollback window)
   mlog::create(LOG_ID, res_comm, /*depth=*/5);  // Instead of 20

Overhead
~~~~~~~~

Logging adds overhead:

- Copy message data
- Store in log structure
- Periodic cleanup

Minimize overhead by:

1. Don't log trivial messages
2. Use logging only for important iterations
3. Balance logging vs. recomputation cost

.. code-block:: cpp

   // Only log every Nth iteration
   if (iteration % LOG_FREQUENCY == 0) {
     mlog::activate(LOG_ID, iteration);
   } else {
     mlog::activate(FENIX_MLOG_NONE);
   }

Troubleshooting
---------------

Log Not Replaying Messages
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Check that:

1. Log was activated before communication
2. Log was checkpointed with data
3. Log was restored and synced after failure

.. code-block:: cpp

   // Debug: Check if log is active
   int active_log = -1;
   fenix::mlog::active(&active_log);
   printf("Active log: %d\n", active_log);

Out of Memory from Logs
~~~~~~~~~~~~~~~~~~~~~~~~

Reduce log depth or checkpoint more often:

.. code-block:: cpp

   // Smaller depth
   mlog::create(LOG_ID, res_comm, 3);  // Was: 10

   // Checkpoint more frequently
   if (iteration % 5 == 0) {  // Was: % 20
     fenix::data::checkpoint(GROUP, fenix::data::SUBSET_FULL, {MLOG_MEMBER});
   }

Recovery is Slow
~~~~~~~~~~~~~~~~

Message replay takes time. Speed up by:

1. Checkpointing more frequently (less to replay)
2. Reducing messages per region
3. Using partial checkpoints for data

Next Steps
----------

- :doc:`inline-recovery-callbacks` - Use callbacks with message logging
- :doc:`optimize-checkpoints` - Optimize checkpoint performance
- :doc:`handle-cascading-failures` - Handle failures during recovery
- :doc:`/api/message-recovery` - Complete message logging API

See Also
--------

- :doc:`/guides/imr-policy` - Understanding redundancy policies
- :doc:`checkpoint-data` - Basic checkpointing guide
- Sender-based message logging: https://en.wikipedia.org/wiki/Message_logging
