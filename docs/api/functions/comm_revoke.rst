comm_revoke
===========

.. operation:: local

Mark a communicator as revoked to propagate failure awareness across all ranks.

This function is a convenience wrapper around ``MPIX_Comm_revoke`` from the MPI ULFM
(User Level Failure Mitigation) specification. It marks a communicator as invalid,
causing all future MPI operations on it to fail with ``MPI_ERR_REVOKED``. This ensures
that all ranks quickly become aware of a failure, even if they haven't yet attempted
communication with failed ranks. See Open MPI documentation for more detail.

.. note::
   This is a temporary wrapper function provided until ``MPI_Comm_revoke`` becomes
   part of the standard MPI specification. Applications using Fenix can call this
   instead of directly using ``MPIX_Comm_revoke`` for better portability.

.. cpp:function:: int fenix::comm_revoke(MPI_Comm comm)

   :param MPI_Comm comm: [in] The communicator to revoke
   :returns: MPI return code (MPI_SUCCESS or error code)

**When to Use:**

Applications typically don't need to call this function directly, as Fenix automatically
revokes communicators during its internal recovery process. However, you may want to
use it to manually revoke subcommunicators that Fenix is unaware of as a PRE_RECOVERY
callback to ensure all ranks make it to the recovery operation.

.. seealso::
   :c:func:`Fenix_Process_detect_failures`, :c:func:`Fenix_Init`, :doc:`/guides/process-recovery`, :doc:`/api/exceptions`
