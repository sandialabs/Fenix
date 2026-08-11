Process Recovery
================

Process recovery within Fenix can be broken down into three steps: detection,
communicator recovery, and application recovery.

----

Detecting Failures
------------------

Fenix is built on top of ULFM (User Level Failure Mitigation) MPI, which provides
the low-level mechanisms for fault tolerance. Specific fault detection mechanisms
and options can be found in the `ULFM documentation <https://docs.open-mpi.org/en/v5.0.x/features/ulfm.html>`_.
At a high level, Fenix detects failures when an MPI function call is made that
involves a failed rank. Detection is **not collectively consistent**, meaning some
ranks may detect a failure and fail an MPI operation while other ranks successfully
complete the same operation (e.g., in a collective operation, some ranks may finish
their ``MPI_Bcast`` while others detect the failure and return an error).

Once a failure is detected, Fenix automatically **revokes** the communicator that
the failed operation was using (and the top-level communicator output by
:c:func:`Fenix_Init`, which are usually the same). Revocation is permanent and
propagates knowledge of the failure: all future MPI operations on the revoked
communicator by any rank will fail immediately with an error code. This ensures
all ranks learn about the failure, even ranks that would never have directly
communicated with the failed rank.

Since failures can only be detected during MPI function calls, applications with
long periods of communication-free computation will experience delays in beginning
recovery. Such applications may benefit from inserting periodic calls to
:c:func:`Fenix_Process_detect_failures` to allow ranks to participate in global recovery
operations with less delay.

Fenix will only detect and respond to failures that occur on the communicator
provided by :c:func:`Fenix_Init` or any communicators derived from it. Faults on other
communicators will, by default, abort the application. Note that having
multiple derived communicators is not currently recommended, and may lead to
deadlock. In fact, even one derived communicator may lead to deadlock if not
used carefully. If you have a use case that requires multiple communicators,
please contact us about your use case -- we can provide guidance and may be
able to update Fenix to support it.

**Advanced:** Applications may wish to handle some failures themselves - either
ignoring them or implementing custom recovery logic in certain code regions.
This is not generally recommended. Significant care must be taken to ensure
that the application does not attempt to enter two incompatible recovery steps.
However, if you wish to do this, you can include "fenix_ext.h" and manually set
``fenix.ignore_errs`` to a non-zero value. This will cause Fenix's error handler
to simply return any errors it encounters as the exit code of the application's
MPI function call. Alternatively, applications may temporarily replace the
communicator's error handler to avoid Fenix recovery. If you have a use case
that would benefit from this, you can contact us for guidance and/or to request
some specific error handling features.

----

Communicator Recovery
---------------------

Once a failure has been detected, Fenix will begin the collective process of
rebuilding the resilient communicator provided by :c:func:`Fenix_Init`. There are two
ways to rebuild: replacing failed ranks with spare ranks, or shrinking the
communicator to exclude the failed ranks. If there are any spare ranks available,
Fenix will use those to replace the failed ranks and maintain the original
communicator size and guarantee that surviving processes keep the same rank ID.
If there are not enough spare ranks, some processes may have a different rank ID on
the new communicator, and Fenix will warn the user about this by setting the
error code for :c:func:`Fenix_Init` to :c:macro:`FENIX_WARNING_SPARE_RANKS_DEPLETED`.

**Advanced:** Communicator recovery is collective, blocking, and not
**interruptible** (cannot be interrupted by additional failures). ULFM exposes
special non-interruptible MPI functions (e.g., ``MPIX_Comm_agree``,
``MPIX_Comm_shrink``) that continue despite additional failures or revocations
during recovery. If multiple collective, non-interruptible operations are started
by different ranks in different orders, the application will deadlock - just as a
non-resilient application would deadlock if ranks called different collectives
(e.g., ``MPI_Allreduce`` vs. ``MPI_Bcast``) in different orders.

However, the preemptive and inconsistent nature of failure detection makes it more
complex to reason about ordering between ranks. Different ranks may detect a failure
at different times and enter recovery at different "locations" in the code. Fenix
uses ULFM's non-interruptible functions internally and ensures consistent ordering:
before any such operation begins, Fenix first uses ``MPIX_Comm_agree`` on the
resilient communicator to agree on which code 'location' all ranks will execute.
If there is any disagreement (ranks are at different locations), all ranks enter
recovery as if they had detected a failure. Applications that wish to use these
ULFM functions directly should follow this pattern, providing a unique 'location'
identifier for any operations that may be interrupted.

See Also
--------

- :doc:`../howto/choose-recovery-pattern` - Choose the right recovery pattern
- :doc:`../howto/migrate-existing-app` - Add Fenix to existing applications
- :doc:`../tutorials/01-first-program` - First fault-tolerant program tutorial
- :doc:`../api/process-recovery` - Process recovery API reference
- :doc:`../troubleshooting` - Common process recovery issues

----

Application Recovery
--------------------

Once a new communicator has been constructed, application recovery begins.
Fenix provides three **resume modes** (controlled by ``FENIX_RESUME_MODE``) that determine
how control returns to your application after the communicator is repaired:

**1. RESUME_JUMP (longjmp-based)** - Default for C API

Fenix automatically uses the C library function ``longjmp`` to jump back to the
:c:func:`Fenix_Init` call site once communicator recovery is complete. This allows for
very simple recovery logic, since it mimics the traditional checkpoint/restart pattern -
execution simply restarts from initialization. Practical for large C codebases where adding
comprehensive error checking would be infeasible.

However, ``longjmp`` requires careful handling: stack variables modified between
``Fenix_Init`` and the failure should be declared ``volatile`` to avoid undefined values.
C++ destructors may not be called for objects in the jumped scope.

**2. RESUME_RETURN (return-based)**

Fenix returns an error code (``FENIX_ERROR_PROCESS_FAILURE``) from the failing MPI or
Fenix function call after communicator recovery is complete. Execution continues from
the point of failure. This requires checking return codes but provides fine-grained
control. Primarily intended for small sections of code needing careful error handling
or third-party C libraries. Not recommended for large-scale application-wide recovery.

**3. RESUME_THROW (exception-based)** - Recommended for C++, default for C++ API

Fenix throws a ``fenix::CommException`` when a failure occurs. At its simplest,
wrapping everything between :c:func:`Fenix_Init` and :c:func:`Fenix_Finalize` in a
single try-catch can give the same simple recovery logic as longjmp mode, but
without the undefined behavior. C++ exceptions provide clean error handling,
proper destructor calls, and well-defined semantics across all compilers.

**Recovery Callbacks**

Applications can register one or more callback functions with :c:func:`Fenix_Callback_register`
and :c:func:`Fenix_Callback_pop`. These callbacks are invoked after communicator recovery,
just before control returns to the application (via jump, return, or exception). Callbacks
work with **all resume modes** and are executed in reverse registration order.

**Message Logging**

In addition to resume modes, Fenix provides message logging (``FENIX_MLOG_RECOVERY_MODE``)
to control whether MPI messages are automatically replayed. These settings are independent:
any resume mode can be combined with any message replay mode. When automatic replay succeeds,
the resume mode is not triggered (the MPI function returns success).

See :doc:`../tutorials/03-resume-modes` for detailed examples and :doc:`../tutorials/04-message-logging`
for message replay modes.

:c:func:`Fenix_Init` outputs a role, from :c:type:`Fenix_Rank_role`, which helps inform the
application about the recovery state of the rank. It is important to note that
all spare ranks are captured inside :c:func:`Fenix_Init` until they are used for
recovery. Therefore, after recovery, recovered ranks will not have the same
callbacks registered -- recovered ranks will need to manually invoke any
callbacks that use MPI functions. These roles also help the application more
generally modify its behavior based on each rank's recovery state.
