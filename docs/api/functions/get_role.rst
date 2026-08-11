get_role
========

.. operation:: local

Query the role of a rank to determine its recovery status.

This function returns the rank's :c:type:`Fenix_Rank_role`, which indicates whether
the rank is in its initial state, has survived failures, or has recovered from being
a spare. This information is crucial for determining what recovery actions to take.

.. c:function:: int Fenix_get_rank_role(MPI_Comm comm, int rank, int* role)

   :param MPI_Comm comm: [in] Fenix resilient communicator
   :param int rank: [in] Rank to query (in the given communicator)
   :param int* role: [out] The rank's :c:type:`Fenix_Rank_role`
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. c:function:: Fenix_Rank_role Fenix_get_role()

   Query the **current rank's** role.

   :returns: The current rank's :c:type:`Fenix_Rank_role`

.. cpp:function:: Role fenix::get_role()

   :returns: The current rank's :c:type:`Fenix_Rank_role`

.. warning::
   **Implementation status:**

   - :c:func:`Fenix_get_role()` (no parameters): **Fully implemented** - queries the current rank's role
   - :c:func:`Fenix_get_rank_role`: **Unimplemented** - for querying other ranks' roles

   Only the parameterless version that queries the current rank is available.
   To query another rank's role, that rank must communicate its role explicitly using MPI.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Role retrieved successfully

**Rank Roles:**

.. c:enumerator:: FENIX_ROLE_INITIAL_RANK

   No failures have occurred yet. This is the state on first execution before any
   ranks have failed. All ranks start with this role.

.. c:enumerator:: FENIX_ROLE_RECOVERED_RANK

   This rank was a spare and was just activated to replace a failed rank, OR was
   spawned to replace a failed rank. Recovered ranks typically need to restore
   application state from checkpoints.

.. c:enumerator:: FENIX_ROLE_SURVIVOR_RANK

   This rank was an active (non-spare) rank before the most recent failure and
   survived the failure. Survivors may or may not need to restore state, depending
   on the application's recovery strategy.

.. c:enumerator:: FENIX_ROLE_SPARE_RANK

   This rank was a spare when Fenix finalized (only possible with
   :c:enumerator:`FENIX_SPARE_FINALIZE_RELEASE`).

**Usage Examples:**

.. code-block:: c

   // C example - Check own role after Init
   int role, error;
   MPI_Comm fenix_comm;
   Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

   switch (role) {
       case FENIX_ROLE_INITIAL_RANK:
           printf("First run, no failures yet\n");
           initialize_simulation();
           break;

       case FENIX_ROLE_RECOVERED_RANK:
           printf("I'm a recovered rank - need to restore state\n");
           restore_from_checkpoint();
           break;

       case FENIX_ROLE_SURVIVOR_RANK:
           printf("I'm a survivor - I have valid state\n");
           // May still want to sync with recovered ranks
           break;
   }

.. code-block:: c

   // C example - Query another rank's role
   int my_rank, other_rank = 0;
   int other_role;

   MPI_Comm_rank(fenix_comm, &my_rank);

   if (my_rank != other_rank) {
       Fenix_get_rank_role(fenix_comm, other_rank, &other_role);
       printf("Rank %d has role %d\n", other_rank, other_role);
   }

.. code-block:: cpp

   // C++ example with enum
   fenix::Role role = fenix::get_role();

   if (role == fenix::RECOVERED_RANK) {
       std::cout << "Recovered - restoring state\n";
       restore_checkpoint();
   } else if (role == fenix::SURVIVOR_RANK) {
       std::cout << "Survived failure\n";
   } else {
       std::cout << "Initial execution\n";
       initialize();
   }

**Role-Based Recovery Strategies:**

Different roles may require different recovery actions:

.. code-block:: c

   void recovery_callback(MPI_Comm comm, int error, void* ctx) {
       int my_rank, my_role;
       MPI_Comm_rank(comm, &my_rank);
       Fenix_get_rank_role(comm, my_rank, &my_role);

       if (my_role == FENIX_ROLE_RECOVERED_RANK) {
           // I'm new - must restore everything
           printf("Rank %d: Restoring full state\n", my_rank);
           restore_all_data();
           rebuild_local_state();
       } else if (my_role == FENIX_ROLE_SURVIVOR_RANK) {
           // I survived - maybe only need to sync boundaries
           printf("Rank %d: Survivor, syncing with neighbors\n", my_rank);
           sync_boundary_data();
       }
   }

**Important Notes:**

- Role is set by :c:func:`Fenix_Init` and can change each time a failure occurs
- RECOVERED_RANK is only guaranteed to be correct immediately after a single failure
- For multiple consecutive failures, the role may not accurately reflect the full history
- Always check role after each recovery to determine necessary actions

**Common Use Cases:**

1. **Conditional restoration**: Only recovered ranks restore from checkpoint
2. **Selective reinitialization**: Different initialization for initial vs recovered ranks
3. **Load balancing**: Redistribute work based on which ranks survived
4. **Logging**: Track how many ranks recovered vs survived
5. **Debugging**: Identify which ranks are in what state during recovery

**Common Pitfalls:**

- **Assuming role persists**: Role can change after each failure. Always check after recovery.
- **Over-relying on role**: For multiple failures, role may not capture full history. Use application-level state tracking for complex recovery.
- **Querying during recovery**: Role may be undefined or stale during the recovery process. Wait until callbacks or after Init returns.

**Role Transition Diagram:**

.. code-block:: text

   First Run:
   [INITIAL_RANK] ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━> (No failures)

   After First Failure:
   [INITIAL_RANK] ━━━━failure━━━> [SURVIVOR_RANK]  (active ranks that survived)
   [SPARE_RANK]   ━━━━activated━━> [RECOVERED_RANK] (spares that replaced failed ranks)

   After Subsequent Failures:
   [SURVIVOR_RANK]   ━━━━failure━━━> [SURVIVOR_RANK]   (survivors remain survivors)
   [RECOVERED_RANK]  ━━━━failure━━━> [SURVIVOR_RANK]   (recovered become survivors)
   [SPARE_RANK]      ━━━━activated━━> [RECOVERED_RANK] (new spares activated)

.. seealso::
   :c:type:`Fenix_Rank_role`, :c:func:`Fenix_Init`, :c:func:`Fenix_Callback_register`, :doc:`/guides/process-recovery`
