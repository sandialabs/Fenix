comm_revoke
===========

.. operation:: local

Mark a communicator as revoked to propagate failure awareness across all ranks.

This function is a convenience wrapper around ``MPIX_Comm_revoke`` from the MPI ULFM
(User Level Failure Mitigation) specification. It marks a communicator as invalid,
causing all future MPI operations on it to fail with ``MPI_ERR_REVOKED``. This ensures
that all ranks quickly become aware of a failure, even if they haven't yet attempted
communication with failed ranks.

.. note::
   This is a temporary wrapper function provided until ``MPI_Comm_revoke`` becomes
   part of the standard MPI specification. Applications using Fenix can call this
   instead of directly using ``MPIX_Comm_revoke`` for better portability.

.. cpp:function:: int fenix::comm_revoke(MPI_Comm comm)

   :param MPI_Comm comm: [in] The communicator to revoke
   :returns: MPI return code (MPI_SUCCESS or error code)

**Return Codes:**

- ``MPI_SUCCESS`` - Communicator successfully revoked
- ``MPI_ERR_COMM`` - Invalid communicator
- Other MPI error codes as defined by the MPI implementation

**How Revocation Works:**

Revocation is **local in invocation but collective in effect**:

1. Any single rank can call ``comm_revoke`` on a communicator
2. The MPI runtime marks the communicator as revoked
3. All subsequent MPI operations on this communicator (on any rank) will fail with ``MPI_ERR_REVOKED``
4. This quickly propagates failure awareness without requiring explicit communication

**When to Use:**

Applications typically don't need to call this function directly, as Fenix automatically
revokes communicators during its internal recovery process. However, you may want to
use it in specific scenarios:

1. **Manual failure injection**: Testing your application's failure handling
2. **Coordinated shutdown**: Gracefully stopping a subset of ranks
3. **Custom recovery protocols**: Building your own recovery mechanism on top of ULFM
4. **Explicit failure propagation**: Ensuring all ranks detect a failure immediately

**Usage Examples:**

.. code-block:: cpp

   // C++ example - Manual failure injection for testing
   #include "fenix.hpp"

   void test_failure_recovery() {
       MPI_Comm test_comm;
       MPI_Comm_dup(MPI_COMM_WORLD, &test_comm);

       int rank;
       MPI_Comm_rank(test_comm, &rank);

       // Rank 0 simulates a failure by revoking the communicator
       if (rank == 0) {
           std::cout << "Simulating failure by revoking communicator\n";
           fenix::comm_revoke(test_comm);
       }

       // All ranks will see MPI_ERR_REVOKED on next operation
       int dummy = 0;
       int ret = MPI_Allreduce(MPI_IN_PLACE, &dummy, 1, MPI_INT,
                                MPI_SUM, test_comm);

       if (ret == MPI_ERR_REVOKED) {
           std::cout << "Rank " << rank << " detected revoked communicator\n";
       }

       // Clean up
       MPI_Comm_free(&test_comm);
   }

.. code-block:: cpp

   // C++ example - Custom recovery with explicit revocation
   void custom_recovery_protocol(MPI_Comm comm) {
       int rank;
       MPI_Comm_rank(comm, &rank);

       // Detect a problem in the application logic
       bool local_error = check_for_error();

       // Use MPI_Allreduce to check if any rank has an error
       int global_error = 0;
       MPI_Allreduce(&local_error, &global_error, 1, MPI_INT,
                      MPI_LOR, comm);

       if (global_error) {
           // Critical error detected - revoke communicator
           // to ensure all ranks know about it
           fenix::comm_revoke(comm);

           // Now initiate recovery
           // (In practice, Fenix handles this automatically)
           std::cout << "Rank " << rank << " initiating recovery\n";
       }
   }

**Fenix Internal Usage:**

Fenix uses ``MPIX_Comm_revoke`` internally during the recovery process:

.. code-block:: cpp

   // Fenix's internal recovery workflow (simplified)
   void __fenix_repair_ranks() {
       // 1. Revoke all Fenix-managed communicators
       MPIX_Comm_revoke(*fenix_rt.world);          // Global fenix communicator
       MPIX_Comm_revoke(fenix_rt.new_world);       // Internal active communicator
       if (fenix_rt.user_world_exists)
           MPIX_Comm_revoke(*fenix_rt.user_world); // User-facing communicator

       // 2. Invoke pre-recovery callbacks
       // 3. Shrink/repair communicators
       // 4. Invoke post-recovery callbacks
   }

This ensures all ranks (including those not directly affected by the failure) detect
the problem and participate in recovery.

**Relationship to ULFM:**

This function directly maps to the ULFM extension:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Fenix API
     - ULFM API
   * - ``fenix::comm_revoke(comm)``
     - ``MPIX_Comm_revoke(comm)``

ULFM provides several failure-handling primitives:

- ``MPIX_Comm_revoke``: Mark communicator as invalid (this function)
- ``MPIX_Comm_failure_ack``: Acknowledge known failures
- ``MPIX_Comm_failure_get_acked``: Get list of acknowledged failures
- ``MPIX_Comm_shrink``: Create new communicator without failed ranks
- ``MPIX_Comm_agree``: Fault-tolerant consensus operation

Fenix uses all of these internally to implement transparent recovery.

**Common Pitfalls:**

- **Revoking the wrong communicator**: Only revoke communicators you intend to replace or clean up.
- **Not coordinating recovery**: Revoking a communicator doesn't recover from failures - it only propagates awareness. You still need to create new communicators.
- **Calling on already-revoked communicator**: This is safe but redundant - the communicator is already invalid.
- **Expecting immediate effect on other ranks**: Ranks only see ``MPI_ERR_REVOKED`` when they attempt an MPI operation, not instantly.

**Best Practices:**

1. **Let Fenix handle it**: In most cases, let Fenix automatically manage communicator revocation during recovery
2. **Test failure scenarios**: Use this function to inject failures and test your recovery logic
3. **Document custom usage**: If you use this for custom recovery protocols, document why Fenix's automatic recovery is insufficient
4. **Always check return codes**: Even though revocation usually succeeds, always check the return value

**Performance Considerations:**

- Revocation is a lightweight local operation
- The overhead is minimal compared to the cost of failure detection and recovery
- Revocation doesn't communicate - the ``MPI_ERR_REVOKED`` propagates lazily as ranks attempt operations

.. seealso::
   :c:func:`Fenix_Process_detect_failures`, :c:func:`Fenix_Init`, :doc:`/guides/process-recovery`, :doc:`/api/exceptions`
