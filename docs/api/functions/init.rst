init
====

.. operation:: collective

Build a resilient communicator and set the restart point.

This function must be called by all ranks in the input communicator, after MPI
initialization. All calling ranks must pass the same values for the
communicator and spare ranks parameters. This function must be called exactly
once by each rank. This function is used:

1. to activate the Fenix library,
2. to specify extra resources in case of rank failure, and
3. to create a logical resume point for when :c:enumerator:`FENIX_RESUME_JUMP` is set

.. note::
   This function uses :c:enumerator:`FENIX_RESUME_JUMP` by default, which exposes
   applications to various undefined behaviors if they do not take care about
   how variables are used before and after the jump. See :c:enumerator:`FENIX_RESUME_JUMP` for
   more information.

It is recommended to access argc and argv only after executing Fenix_Init,
since command line arguments passed to this function that apply to Fenix may
be removed by Fenix_Init.

Fenix_Init is blocking in the following sense. If it is entered for the
first time via a regular, explicit function call, it must be entered by all
ranks in the input communicator. If it is entered after an error intercepted by
Fenix (due to :c:enumerator:`FENIX_RESUME_JUMP`), no ranks are allowed to
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

.. c:function:: void Fenix_Init(int* role, MPI_Comm comm, MPI_Comm* newcomm, int** argc, char*** argv, int spare_ranks, int* error)

   :param int* role: The current :c:type:`Fenix_Rank_role` of this rank
   :param MPI_Comm comm: Communicator to construct the resilient communicator from
   :param MPI_Comm* newcomm: Resilient output communicator
   :param int** argc: Pointer to application main's argc parameter
   :param char*** argv: Pointer to application main's argv parameter
   :param int spare_ranks: Number of ranks in comm to reserve from newcomm
   :param int* error: The return status. FENIX_SUCCESS or FENIX_WARNING_SPARE_RANKS_DEPLETED if spare ranks are depleted

.. cpp:function:: void fenix::init(const fenix::args::FenixInitArgs args)

   :param args: Struct containing initialization parameters (role, in_comm, out_comm, argc, argv, spares, err)

.. note::
   The C++ overload accepts a struct (``fenix::args::FenixInitArgs``) with named fields for clarity.
   Any Fenix function without a return type may be implemented via macros, in which case it cannot be used to resolve function pointers.

.. code-block:: c

   // C example
   int role, error;
   MPI_Comm newcomm;
   Fenix_Init(&role, MPI_COMM_WORLD, &newcomm, &argc, &argv, 2, &error);

.. code-block:: cpp

   // C++ example
   int role, error;
   MPI_Comm newcomm;
   fenix::init({&role, MPI_COMM_WORLD, &newcomm, &argc, &argv, 2, &error});

.. seealso::
   :c:func:`Fenix_Finalize`, :c:type:`Fenix_Rank_role`, :doc:`/guides/process-recovery`
