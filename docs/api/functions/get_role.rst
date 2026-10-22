get_role
========

.. operation:: local

Query the role of a rank to determine its recovery status.

.. c:function:: Fenix_Rank_role Fenix_get_role()

.. cpp:function:: Role fenix::role()

   :returns: The current rank's :c:type:`Fenix_Rank_role`

**Rank Roles:**

.. c:enumerator:: FENIX_ROLE_INITIAL_RANK

   No failures have occurred yet. This is the state on first execution before any
   ranks have failed. All active ranks start with this role.

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

**Role-Based Recovery Strategies:**

Different roles may require different recovery actions:

.. code-block:: c

   void recovery_callback(MPI_Comm comm, int error, void* ctx) {
       if (fenix::role() == FENIX_ROLE_RECOVERED_RANK) {
           // I'm new - must restore everything
           printf("Rank %d: Restoring full state\n", my_rank);
           restore_all_data();
           rebuild_local_state();
       } else if (fenix::role() == FENIX_ROLE_SURVIVOR_RANK) {
           // I survived - maybe only need to sync boundaries
           printf("Rank %d: Survivor, syncing with neighbors\n", my_rank);
           sync_boundary_data();
       }
   }

**Common Pitfall: Over-relying on role**

A rank's role does not necessarily capture its full history, which is
particularly important when multiple failures occur in quick succession. For
instance, the example above assumes that survivor ranks do not need to restore
their full state - but if a failure interrupted a recovered rank while it was
restoring its data and state, it may report as a survivor after the subsequent
recovery while still needing to finish its full restoration.

**Role Transition Diagram:**

.. code-block:: text

   First Run:
   [INITIAL_RANK] ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━> (No failures)

   After First Failure:
   [INITIAL_RANK] ━━━━failure━━━━> [SURVIVOR_RANK]  (active ranks that survived)
   [SPARE_RANK]   ━━━━activated━━> [RECOVERED_RANK] (spares that replaced failed ranks)

   After Subsequent Failures:
   [SURVIVOR_RANK]   ━━━━failure━━━━> [SURVIVOR_RANK]  (survivors remain survivors)
   [RECOVERED_RANK]  ━━━━failure━━━━> [SURVIVOR_RANK]  (recovered become survivors)
   [SPARE_RANK]      ━━━━activated━━> [RECOVERED_RANK] (new spares activated)

.. seealso::
   :c:type:`Fenix_Rank_role`, :c:func:`Fenix_Init`, :c:func:`Fenix_Callback_register`, :doc:`/guides/process-recovery`
