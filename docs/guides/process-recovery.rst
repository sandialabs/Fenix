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

- :doc:`../howto/choose-recovery-pattern` - Choose between longjmp and inline recovery
- :doc:`../howto/migrate-existing-app` - Add Fenix to existing applications
- :doc:`../tutorials/01-first-program` - First fault-tolerant program tutorial
- :doc:`../api/process-recovery` - Process recovery API reference
- :doc:`../troubleshooting` - Common process recovery issues

----

Application Recovery
--------------------

Once a new communicator has been constructed, application recovery begins.
There are two recovery modes: **longjmp** (default) and **inline** (non-jumping).

With **longjmp recovery**, Fenix automatically uses the C library function ``longjmp``
to jump back to the :c:func:`Fenix_Init` call site once communicator recovery is complete.
This allows for very simple recovery logic, since it mimics the traditional
checkpoint/restart pattern - execution simply restarts from initialization. However,
``longjmp`` has undefined behavior according to the C and C++ specifications:
variables may have unexpected values, C++ destructors may not be called, and compiler
optimizations may break. Variables should be declared ``volatile`` to avoid issues,
but this doesn't solve all problems.

With **inline recovery** (non-jumping mode), Fenix returns an error code from the
failing MPI function call after communicator recovery is complete. Execution continues
inline without jumping. This is more predictable across compilers and optimizations,
but requires checking return codes of MPI calls (or using exceptions in C++).
Additionally, some applications can recover more efficiently by continuing inline
rather than restarting from initialization.

Fenix also allows applications to register one or more callback functions with
:c:func:`Fenix_Callback_register` and :c:func:`Fenix_Callback_pop`, which removes the most
recently registered callback. These callbacks are invoked after communicator
recovery, just before control returns to the application. Callbacks are
executed in the reverse order they were registered.

For C++ applications, it is recommended to use inline recovery (non-jumping mode)
with exceptions. Set ``FENIX_RESUME_MODE`` to ``FENIX_RESUME_THROW``, and Fenix
will throw a ``fenix::CommException`` when a failure occurs. At its simplest,
wrapping everything between :c:func:`Fenix_Init` and :c:func:`Fenix_Finalize` in a
single try-catch can give the same simple recovery logic as longjmp mode, but
without the undefined behavior. C++ exceptions provide clean error handling,
proper destructor calls, and well-defined semantics across all compilers.

:c:func:`Fenix_Init` outputs a role, from :c:type:`Fenix_Rank_role`, which helps inform the
application about the recovery state of the rank. It is important to note that
all spare ranks are captured inside :c:func:`Fenix_Init` until they are used for
recovery. Therefore, after recovery, recovered ranks will not have the same
callbacks registered -- recovered ranks will need to manually invoke any
callbacks that use MPI functions. These roles also help the application more
generally modify its behavior based on each rank's recovery state.
