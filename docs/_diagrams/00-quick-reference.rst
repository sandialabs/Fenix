Fenix Quick Reference Card
==========================

Quick reference for common Fenix operations and patterns.

.. _quick-reference:

Initialization
--------------

**C API:**

.. code-block:: c

   int role, error;
   MPI_Comm res_comm;
   Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
             &argc, &argv, n_spares, &error);

**C++ API:**

.. code-block:: cpp

   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 2});
   int role = fenix::role();

Rank Roles
----------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Role
     - Description
   * - INITIAL_RANK
     - First time, initialize data
   * - RECOVERED_RANK
     - Spare promoted, restore data
   * - SURVIVOR_RANK
     - Survived failure (longjmp only)
   * - SPARE_RANK
     - Never seen by user (internal)

Recovery Patterns
-----------------

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 40

   * - Pattern
     - Language
     - RAII Safe
     - Best For
   * - LONGJMP
     - C, (C++)
     - ❌ No
     - Simple apps
   * - INLINE
     - C, C++
     - ✅ Yes
     - C apps
   * - EXCEPTION
     - C++ only
     - ✅ Yes
     - Modern C++

**Setting the recovery mode:**

.. code-block:: c

   // Longjmp (default)
   Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_JUMP);

   // Inline
   Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_RETURN);

   // Exception (C++ only)
   Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_THROW);

Data Recovery
-------------

**Create Group:**

.. code-block:: c

   Fenix_Data_group_create(group_id, comm, time, depth, policy, ...);

**Register Member:**

.. code-block:: c

   // For initial ranks:
   Fenix_Data_member_create(group_id, member_id, ptr, count, datatype);

   // For recovered ranks:
   Fenix_Data_member_define(group_id, member_id, ptr, count, datatype);

**Checkpoint:**

.. code-block:: c

   Fenix_Data_member_store(group_id, member_id, subset);
   Fenix_Data_commit_barrier(group_id, timestamp);

**Restore:**

.. code-block:: c

   Fenix_Data_member_restore(group_id, member_id, ptr, count,
                            FENIX_DATA_SNAPSHOT_LATEST, subset);

Message Logging
---------------

.. code-block:: c

   // Enable
   Fenix_Message_logging_enable(comm, max_messages);

   // Disable
   Fenix_Message_logging_disable(comm);

Typical Usage Pattern
---------------------

.. code-block:: c

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // 1. Initialize Fenix
     int role, error;
     MPI_Comm comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &comm, &argc, &argv, 2, &error);

     int rank;
     MPI_Comm_rank(comm, &rank);

     // 2. Setup data recovery
     Fenix_Data_group_create(0, comm, 0, 1, FENIX_DATA_POLICY_IN_MEMORY_RAID,
                            (int[]){1, 2}, NULL);

     struct { int iter; double data[1000]; } state;

     if (role == FENIX_ROLE_INITIAL_RANK) {
       // Initialize
       state.iter = 0;
       Fenix_Data_member_create(0, 0, &state, sizeof(state), MPI_BYTE);
     } else {
       // Restore
       Fenix_Data_member_define(0, 0, &state, sizeof(state), MPI_BYTE);
       Fenix_Data_member_restore(0, 0, &state, sizeof(state),
                                FENIX_DATA_SNAPSHOT_LATEST, NULL);
     }

     // 3. Initial checkpoint
     Fenix_Data_member_store(0, 0, FENIX_DATA_SUBSET_FULL);
     Fenix_Data_commit_barrier(0, NULL);

     // 4. Application loop
     for (int i = state.iter; i < 100; i++) {
       state.iter = i;

       // Work
       for (int j = 0; j < 1000; j++) {
         state.data[j] = state.data[j] * 2.0;
       }

       // MPI communication
       MPI_Allreduce(MPI_IN_PLACE, state.data, 1000,
                    MPI_DOUBLE, MPI_SUM, comm);

       // Periodic checkpoint
       if (i % 10 == 0) {
         Fenix_Data_member_store(0, 0, FENIX_DATA_SUBSET_FULL);
         Fenix_Data_commit_barrier(0, NULL);
       }
     }

     // 5. Cleanup
     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Inline Recovery with Callbacks
-------------------------------

.. code-block:: c

   void recovery_callback(MPI_Comm comm, int err, void* data) {
     // Restore state here
     Fenix_Data_member_restore(0, 0, data, size,
                              FENIX_DATA_SNAPSHOT_LATEST, NULL);
   }

   int main() {
     // ...Init...

     Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_RETURN);
     Fenix_Callback_register(recovery_callback, &state);

     for (int i = 0; i < N; i++) {
       int ret = MPI_Allreduce(...);
       if (ret == MPI_ERR_PROC_FAILED) {
         // Recovered! Callback already ran
         continue;
       }
     }
   }

Exception Recovery (C++)
------------------------

.. code-block:: cpp

   fenix::init({.resume_mode = fenix::RESUME_THROW});

   fenix::callback_register([&](MPI_Comm c, int err) {
     fenix::data::member_restore(group_id, member_id);
   });

   while (state.iter < MAX) {
     try {
       for (; state.iter < MAX; state.iter++) {
         // Work...
         MPI_Allreduce(..., res_comm);  // May throw
       }
       break;
     } catch (fenix::CommException& e) {
       // Recovered, continue
     }
   }

Common Errors
-------------

.. list-table::
   :widths: 50 50

   * - **Error**
     - **Solution**
   * - "MPI_ERR_COMM: invalid communicator"
     - Use ``res_comm`` not ``MPI_COMM_WORLD``
   * - "member_restore: group not found"
     - Call ``group_create`` after ``Init``
   * - "Variables have wrong values"
     - Use ``volatile`` (longjmp) or switch modes
   * - "Deadlock after failure"
     - Check for mismatched collectives
   * - "Segfault after recovery"
     - Recreate pointers or use inline/exception

Building & Running
------------------

**Compile:**

.. code-block:: bash

   mpicc -o app app.c -lfenix -I$FENIX_DIR/include -L$FENIX_DIR/lib

**Run:**

.. code-block:: bash

   mpiexec --with-ft mpi -n 6 ./app
   # 4 active + 2 spares = 6 total ranks

**Test locally:**

.. code-block:: bash

   mpiexec --with-ft mpi --allow-run-as-root \\
     --map-by :oversubscribe -n 6 ./app

Required MPI Flags
------------------

For Open MPI 5+ with ULFM:

.. code-block:: bash

   --with-ft mpi           # Enable fault tolerance
   --allow-run-as-root     # If running as root (testing)
   --map-by :oversubscribe # Allow oversubscription

Checkpoint Frequency
--------------------

**Formula:**

.. math::

   checkpoint\_interval = \sqrt{2 \times iteration\_time \times MTBF}

**Rule of thumb:**

- Fast iterations (< 1ms): Every 50-100 iterations
- Medium (1-10ms): Every 20-50 iterations
- Slow (> 100ms): Every 1-5 iterations

Memory Overhead
---------------

.. list-table::
   :widths: 40 60

   * - **Component**
     - **Overhead**
   * - Process Recovery
     - ~0% (no extra memory)
   * - Data Recovery
     - ~2-3× data size (redundancy)
   * - Message Logging
     - ~10-50 MB per rank (configurable)
   * - Spare Ranks
     - ~10% per spare (minimal state)

Performance Impact
------------------

**Normal Operation:**

- Process recovery: 0% overhead
- Data checkpoints: 1-5% overhead
- Message logging: 10-20% overhead

**Recovery:**

- Process repair: 10-50 ms
- Data restore: 10-100 ms
- Message replay: 50-500 ms

Key Concepts
------------

.. glossary::

   Resilient Comm
      Communicator that survives failures (``res_comm``)

   Recovery
      Process of replacing failed rank and restoring state

   Revocation
      Marking comm as failed (permanent)

   Shrink
      Removing failed ranks from comm

Symbol Legend
-------------

.. list-table::
   :widths: 20 80

   * - ✅
     - Recommended
   * - ❌
     - Not recommended
   * - ⚠️
     - Use with caution

.. seealso::

   * :doc:`01-basic-recovery-flow` - Detailed recovery process
   * :doc:`07-rank-roles` - Understanding rank roles
   * :doc:`12-decision-recovery-pattern` - Choosing the right pattern
