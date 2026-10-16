finalize
========

.. operation:: collective

Finalize Fenix and clean up all resources.

.. c:function:: int Fenix_Finalize()

.. cpp:function:: void fenix::finalize()

   :returns: FENIX_SUCCESS if successful, error code otherwise

This function must be called by all active ranks after :c:func:`Fenix_Init` and
before ``MPI_Finalize``. It frees all Fenix internal resources including data
groups, callbacks, and message logs. Any remaining spare ranks will
automatically exit (default) or be released per the
:c:enumerator:`FENIX_SPARE_FINALIZE_MODE` setting.

No Fenix calls are valid after successful finalization except
:c:func:`Fenix_Initialized`.

This function **can** fail due to process failures, in which case it will enter
recovery steps as if it were a failed MPI operation. When inline message-log
recovery is active, a rank will not leave this function until success or a
message-log error occurs. Successful completion guarantees that all active ranks
invoked the function, but not necessarily that all ranks exited the function. If
the function fails, no internal resources will have been cleaned up and no
spares will have finalized, so that they can be used for recovery of any failed
ranks, as needed.

.. seealso::
   :c:func:`Fenix_Init`, :c:func:`Fenix_set_option`, :c:enumerator:`FENIX_SPARE_FINALIZE_MODE`
