init
====

.. operation:: collective

Build a resilient communicator and set the restart point.

.. c:function:: void Fenix_Init(int* role, MPI_Comm comm, MPI_Comm* newcomm, int** argc, char*** argv, int spare_ranks, int* error)

   :param int* role: [out] The current :c:type:`Fenix_Rank_role` of this rank.
   :param MPI_Comm comm: [in] Communicator from which to construct the resilient communicator.
   :param MPI_Comm* newcomm: [out] Resilient output communicator.
   :param int** argc: [in/out] Pointer to ``main``\'s ``argc`` parameter.
   :param char*** argv: [in/out] Pointer to ``main``\'s ``argv`` parameter.
   :param int spare_ranks: [in] Number of ranks in ``comm`` to reserve from ``newcomm`` as spares.
   :param int* error: [out] Status shared across all ranks released by :c:func:`Fenix_Init`. Reports :c:enumerator:`FENIX_SUCCESS` on normal execution, or :c:enumerator:`FENIX_WARNING_SPARE_RANKS_DEPLETED` if repair had to continue by shrinking because spare ranks were exhausted.

.. cpp:function:: void fenix::init(const args::FenixInitArgs args)

   :param args: Initialization arguments (`role`, `in_comm`, `out_comm`, `argc`, `argv`, `spares`, and `err`)

This function must be called by all ranks in the input communicator after MPI
initialization. All calling ranks must pass the same values for ``comm`` and
``spare_ranks``, and each rank must call init exactly once.

No Fenix functions may be called before initialization, except
:c:func:`Fenix_Initialized` and :c:func:`Fenix_set_option`.

.. note::
   The C API is macro-backed. As with other macro-based entry points, it cannot
   be used where a function pointer is required.

.. important::
   :c:enumerator:`FENIX_RESUME_JUMP` is the default resume mode for the C API.
   Variables used across the jump are subject to the usual ``longjmp`` caveats;
   in C++, :c:enumerator:`FENIX_RESUME_THROW` is usually the safer choice (and
   if the default for the C++ API).

.. code-block:: c

   int role, error;
   MPI_Comm fenix_comm;

   Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

   if (role == FENIX_ROLE_RECOVERED_RANK) {
       /* restore application state */
   }

.. code-block:: cpp

   MPI_Comm fenix_comm;
   fenix::init({.out_comm = &fenix_comm, .spares = 2});
   
   if (fenix::role() == FENIX_ROLE_RECOVERED_RANK) {
       /* restore application state */
   }

.. seealso::
   :c:func:`Fenix_Finalize`, :c:func:`Fenix_Initialized`, :c:func:`Fenix_set_option`, :c:type:`Fenix_Rank_role`, :doc:`/guides/process-recovery`
