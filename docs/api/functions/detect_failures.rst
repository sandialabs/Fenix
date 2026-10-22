detect_failures
===============

.. operation:: local

Explicitly check for process failures and optionally trigger recovery.

This function proactively checks for failed ranks and can initiate the recovery protocol.
It's essential for applications with long compute phases that don't make frequent MPI calls,
as Fenix normally detects failures only when MPI operations fail.

.. c:function:: int Fenix_Process_detect_failures(int recover)

.. cpp:function:: int fenix::detect_failures(bool recover = true)

   :param bool recover: If true, perform recovery if failures are detected (default: true)
   :returns: FENIX_SUCCESS if no failures detected, else FENIX_ERROR_PROCESS_FAILURE

This is a local best-effort check. It does not communicate and does not provide
a globally consistent failure view the way communicator agreement does.

If recover is true, this is treated like a normal MPI function call that
encountered a failed process. If recover is false, the behavior is equivalent to
using :c:enumerator:`FENIX_RECOVERY_IGNORE` with
:c:enumerator:`FENIX_RESUME_RETURN` for this check.

If inline message-log recovery is active, detected failures are recovered and
the operation is replayed until no further failures are found, ignoring the
normal resume-mode control transfer for this call.

Detection is lightweight, but not free. This is internally implemented as either
an MPI_Test on an unmatched MPI_ANY_SOURCE receive for a rarely-used internal
communicator, or as a similar MPI_Iprobe. Best called periodically during long
computation phases, not in every iteration of lightweight loops.
