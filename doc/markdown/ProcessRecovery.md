Process recovery within Fenix can be broken down into three steps: detection,
communicator recovery, and application recovery.

---

## Detecting Failures

Fenix is built on top of ULFM MPI, so specific fault detection mechanisms and
options can be found in the [ULFM
documentation](https://docs.open-mpi.org/en/v5.0.x/features/ulfm.html#). At a
high level, this means that Fenix will detect failures when an MPI function
call is made which involves a failed rank. Detection is not collectively
consistent, meaning some ranks may fail to complete a collective while other
ranks finish successfully. Once a failure is detected, Fenix will 'revoke' the
communicator that the failed operation was using and the top-level communicator
output by #Fenix_Init (these communicators are usually the same). The
revocation is permanent, and means that all future operations on the
communicator by any rank will fail. This allows knowledge of the failed rank to
be propagated to all ranks in the communicator, even if some ranks would never
have directly communicated with the failed rank.

Since failures can only be detected during MPI function calls, applications with
long periods of communication-free computation will experience delays in beginning
recovery. Such applications may benefit from inserting periodic calls to
#Fenix_Process_detect_failures to allow ranks to participate in global recovery
operations with less delay.

Fenix will only detect and respond to failures that occur on the communicator
provided by #Fenix_Init or any communicators derived from it. Faults on other
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
`fenix.ignore_errs` to a non-zero value. This will cause Fenix's error handler
to simply return any errors it encounters as the exit code of the application's
MPI function call. Alternatively, applications may temporarily replace the
communicator's error handler to avoid Fenix recovery. If you have a use case
that would benefit from this, you can contact us for guidance and/or to request
some specific error handling features.

---

## Communicator Recovery

Once a failure has been detected, Fenix will begin the collective process of
rebuilding the resilient communicator provided by #Fenix_Init. There are two
ways to rebuild: replacing failed ranks with spares, or shrinking the
communicator to exclude the failed ranks. If there are any spares available,
Fenix will use those to replace the failed ranks and maintain the original
communicator size and guarantee that surviving processes keep the same rank ID.
If there are not enough spares, some processes may have a different rank ID on
the new communicator, and Fenix will warn the user about this by setting the
error code for #Fenix_Init to #FENIX_WARNING_SPARE_RANKS_DEPLETED.

**Advanced:** Communicator recovery is collective, blocking, and not
interruptable. ULFM exposes some functions (e.g. MPIX_Comm_agree,
MPIX_Comm_shrink) that are also not interrupable -- meaning they will continue
despite any failures or revocations. If multiple collective, non-interruptable
operations are started by different ranks in different orders, the application
will deadlock. This is similar to what would happen if a non-resilient
application called multiple collectives (e.g. `MPI_Allreduce`) in different
orders. However, the preemptive and inconsistent nature of failure recovery
makes it more complex to reason about ordering between ranks. Fenix uses these
ULFM functions internally, so care is taken to ensure that the order of
operations is consistent across ranks. Before any such operation begins, Fenix
first uses MPIX_Comm_agree on the resilient communicator provided by
#Fenix_Init to agree on which 'location' will proceed - if there is any
disagreement, all ranks will enter recovery as if they had detected a failure.
Applications which wish to use these functions themselves should follow this
pattern, providing a unique 'location' value for any operations that may be
interrupted.

---

## Application Recovery

Once a new communicator has been constructed, application recovery begins.
There are two recovery modes: jumping (default) and non-jumping. With jumping
recovery, Fenix will automatically `longjmp` to the #Fenix_Init call site once
communicator recovery is complete. This allows for very simple recovery logic,
since it mimics the traditional teardown-restart pattern. However, `longjmp`
has many undefined semantics according to the C and C++ specifications and may
result in unexpected behavior due to compiler assumptions and optimizations.
Additionally, some applications may be able to more efficiently recover by
continuing inline. Users can initialize Fenix as non-jumping (see test/no_jump)
to instead return an error code from the triggering MPI function call after
communicator recovery. This may require more intrusive code changes (checking
return statuses of each MPI call).

Fenix also allows applications to register one or more callback functions with
#Fenix_Callback_register and #Fenix_Callback_pop, which removes the most
recently registered callback. These callbacks are invoked after communicator
recovery, just before control returns to the application. Callbacks are
executed in the reverse order they were registered. 

For C++ applications, it is recommended to use Fenix in non-jumping mode and to
register a callback that throws an exception. At it's simplest, wrapping
everything between #Fenix_Init and #Fenix_Finalize in a single try-catch can
give the same simple recovery logic as jumping mode, but without the undefined
behavior of `longjmp`.

#Fenix_Init outputs a role, from #Fenix_Rank_role, which helps inform the
application about the recovery state of the rank. It is important to note that
all spare ranks are captured inside #Fenix_Init until they are used for
recovery. Therefore, after recovery, recovered ranks will not have the same
callbacks registered -- recovered ranks will need to manually invoke any
callbacks that use MPI functions. These roles also help the application more
generally modify it's behavior based on each rank's recovery state.
