Process Recovery
================

Initialization and Finalization
--------------------------------

.. c:function:: void Fenix_Init(int* role, MPI_Comm comm, MPI_Comm* newcomm, int** argc, char*** argv, int spare_ranks, int* error)

   Build a resilient communicator and set the restart point.

   This function must be called by all ranks in *comm*, after MPI
   initialization. All calling ranks must pass the same values for the
   parameters *comm* and *spare_ranks*. Fenix_Init must be called exactly
   once by each rank. This function is used:

   1. to activate the Fenix library,
   2. to specify extra resources in case of rank failure, and
   3. to create a logical resume point for when :c:macro:`FENIX_RESUME_JUMP` is set

   .. note::
      This function uses :c:macro:`FENIX_RESUME_JUMP` by default, which exposes
      applications to various undefined behaviors if they do not take care about
      how variables are used before and after the jump. See :c:enumerator:`FENIX_RESUME_JUMP` for
      more information.

   It is recommended to access argc and argv only after executing Fenix_Init,
   since command line arguments passed to this function that apply to Fenix may
   be removed by Fenix_Init.

   .. rubric:: Collective Operation

   Fenix_Init is blocking in the following sense. If it is entered for the
   first time via a regular, explicit function call, it must be entered by all
   ranks in communicator *comm*. If it is entered after an error intercepted by
   Fenix (due to :c:macro:`FENIX_RESUME_MODE` :c:enumerator:`FENIX_RESUME_JUMP`), no ranks are allowed to
   exit from it until all *non-failed* ranks have returned control to it.

   .. note::
      Typically, control is returned automatically through revocation of
      the resilient communicator, which means ranks that have long delays between
      MPI function calls or ranks that only use communicators unaffected by failure
      may lead to long delays between a failure and its recovery. See
      :c:func:`Fenix_Process_detect_failures` for a way to improve this behavior.

   Ranks to be used as spare ranks by Fenix will be available to the application
   only before Fenix_Init or after they are used to replace a failed rank (in
   which case they become active ranks). This document refers to the latter as
   RECOVERED ranks (see :c:type:`Fenix_Rank_role`). Note that all spare ranks that
   have not been used to recover from failures (and, therefore, are still
   reserved by Fenix and kept inside Fenix_Init) will automatically call
   MPI_Finalize and exit when all active ranks have entered the
   :c:func:`Fenix_Finalize` call.

   No Fenix functions may be called before Fenix_Init, except
   :c:func:`Fenix_Initialized` and :c:func:`Fenix_set_option`.

   :param role: [out] The current :c:type:`Fenix_Rank_role` of this rank
   :param comm: [in] Communicator to construct the resilient communicator from
   :param newcomm: [out] Resilient output communicator
   :param argc: [inout] Pointer to application main's argc parameter
   :param argv: [inout] Pointer to application main's argv parameter
   :param spare_ranks: [in] Number of ranks in comm to reserve from newcomm. These ranks are reserved to substitute for failed ranks.
   :param error: [out] The return status of Fenix_Init. Used to signal that a non-fatal error or special condition was encountered in the execution of Fenix_Init, or FENIX_SUCCESS otherwise. It has the same value across all ranks released by Fenix_Init. If spawning is not enabled (:c:macro:`FENIX_RECOVERY_MODE` is not :c:enumerator:`FENIX_RECOVERY_SPAWN`) and spare ranks have been depleted, Fenix will repair resilience communicators by shrinking them and will report such shrinkage in this return parameter through the value FENIX_WARNING_SPARE_RANKS_DEPLETED.

   .. note::
      Any Fenix function without a return type may be implemented via macros, in which case it cannot be used to resolve function pointers.

   .. seealso::
      - :c:func:`Fenix_Finalize`
      - :c:type:`Fenix_Rank_role`
      - :doc:`/guides/process-recovery`

.. c:function:: int Fenix_Finalize()

   Finalize Fenix and clean up resources.

   .. rubric:: Collective Operation

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :c:func:`Fenix_Init`

.. c:function:: int Fenix_Initialized(int* flag)

   Sets flag to true if Fenix_Init has been called, else false.

   .. rubric:: Local Operation

   :param flag: [out] Pointer to the flag to be set.

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Finalized(int* flag)

   Sets flag to true if Fenix_Finalize has been called, else false.

   .. rubric:: Local Operation

   :param flag: [out] Pointer to the flag to be set.

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Configuration
-------------

.. c:function:: int Fenix_set_option(Fenix_Setting_name name, void* value)

   Configure a global Fenix setting.

   .. rubric:: Local Operation

   Each :c:type:`Fenix_Setting_name` will describe its function and valid options.

   If called prior to Fenix_Init, the setting will apply to future Fenix_Inits.

   :param name: [in] The setting to configure
   :param value: [in] Pointer to the new value

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :c:func:`Fenix_get_option`
      - :c:type:`Fenix_Setting_name`

.. c:function:: int Fenix_get_option(Fenix_Setting_name name, void* value)

   Get the current value of a global Fenix setting.

   .. rubric:: Local Operation

   :param name: [in] The setting to query
   :param value: [out] Pointer to store the current value

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :c:func:`Fenix_set_option`
      - :c:type:`Fenix_Setting_name`

Callbacks
---------

.. c:function:: int Fenix_Callback_register(void (*recover)(MPI_Comm, int, void*), void* callback_data)

   Register a callback function to be invoked after communicator recovery.

   .. rubric:: Local Operation

   Callbacks are invoked after communicator recovery, just before control returns
   to the application. Callbacks are executed in the reverse order they were registered.

   :param recover: [in] The callback function to register
   :param callback_data: [in] User data to pass to the callback

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :c:func:`Fenix_Callback_pop`
      - :doc:`/guides/process-recovery`

.. c:function:: int Fenix_Callback_pop()

   Remove the most recently registered callback.

   .. rubric:: Local Operation

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :c:func:`Fenix_Callback_register`

Failure Detection
-----------------

.. c:function:: int Fenix_Process_detect_failures(MPI_Comm* newcomm, int* error)

   Check for failures and repair communicator if needed.

   .. rubric:: Collective Operation

   Applications with long periods of communication-free computation may benefit
   from inserting periodic calls to this function to allow ranks to participate
   in global recovery operations with less delay.

   :param newcomm: [out] Repaired communicator (if repair was needed)
   :param error: [out] Error code indicating if recovery occurred

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :doc:`/guides/process-recovery`

Query Functions
---------------

.. c:function:: int Fenix_get_role(int* role)

   Get the current :c:type:`Fenix_Rank_role` of this rank.

   .. rubric:: Local Operation

   :param role: [out] The current role

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :c:type:`Fenix_Rank_role`

.. c:function:: int Fenix_get_error(int* error)

   Get the error code from the most recent Fenix_Init.

   .. rubric:: Local Operation

   :param error: [out] The error code

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_get_nspare(int* nspare)

   Get the number of spare ranks currently available.

   .. rubric:: Local Operation

   :param nspare: [out] Number of spare ranks

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise
