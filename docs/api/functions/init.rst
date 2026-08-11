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

   :param int* role: [out] The current :c:type:`Fenix_Rank_role` of this rank after initialization. One of FENIX_ROLE_INITIAL_RANK, FENIX_ROLE_RECOVERED_RANK, or FENIX_ROLE_SURVIVOR_RANK.
   :param MPI_Comm comm: [in] Communicator to construct the resilient communicator from (typically MPI_COMM_WORLD)
   :param MPI_Comm* newcomm: [out] Resilient output communicator. Use this communicator for all subsequent MPI operations.
   :param int** argc: [in/out] Pointer to application main's argc parameter. Fenix may remove command-line arguments.
   :param char*** argv: [in/out] Pointer to application main's argv parameter. Fenix may remove command-line arguments.
   :param int spare_ranks: [in] Number of ranks in comm to reserve as spare ranks. These ranks won't appear in newcomm initially but will be used to replace failed ranks.
   :param int* error: [out] Return status: FENIX_SUCCESS on normal initialization, or FENIX_WARNING_SPARE_RANKS_DEPLETED if spare ranks were exhausted during recovery.

.. cpp:function:: void fenix::init(const fenix::args::FenixInitArgs args)

   :param args: Struct containing initialization parameters (role, in_comm, out_comm, argc, argv, spares, err). See :cpp:class:`fenix::args::FenixInitArgs`.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Normal initialization
- :c:enumerator:`FENIX_WARNING_SPARE_RANKS_DEPLETED` - Recovery occurred but spare ranks were depleted

.. note::
   The C++ overload accepts a struct (``fenix::args::FenixInitArgs``) with named fields for clarity.
   Any Fenix function without a return type may be implemented via macros, in which case it cannot be used to resolve function pointers.

**Usage Examples:**

.. code-block:: c

   // C example - Basic initialization with 2 spare ranks
   int main(int argc, char** argv) {
       int role, error;
       MPI_Comm fenix_comm;

       MPI_Init(&argc, &argv);

       // Initialize Fenix with 2 spare ranks
       Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

       if (error == FENIX_WARNING_SPARE_RANKS_DEPLETED) {
           fprintf(stderr, "Warning: Spare ranks depleted, running with reduced size\n");
       }

       // Check rank's role
       if (role == FENIX_ROLE_INITIAL_RANK) {
           printf("Initial rank - no failures yet\n");
       } else if (role == FENIX_ROLE_RECOVERED_RANK) {
           printf("Recovered rank - I was just activated to replace a failure\n");
       } else if (role == FENIX_ROLE_SURVIVOR_RANK) {
           printf("Survivor rank - I survived a failure\n");
       }

       // Use fenix_comm for all MPI operations
       int rank;
       MPI_Comm_rank(fenix_comm, &rank);

       // Application work...

       Fenix_Finalize();
       MPI_Finalize();
       return 0;
   }

.. code-block:: cpp

   // C++ example with exception handling
   int main(int argc, char** argv) {
       int role, error;
       MPI_Comm fenix_comm;

       MPI_Init(&argc, &argv);

       // Configure to use exceptions instead of longjmp
       fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

       fenix::init({&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error});

       try {
           // Application work with automatic recovery
           int rank, size;
           MPI_Comm_rank(fenix_comm, &rank);
           MPI_Comm_size(fenix_comm, &size);

           int data = rank * 100;
           if (rank < size - 1) {
               MPI_Send(&data, 1, MPI_INT, rank + 1, 0, fenix_comm);
           }
       } catch (const fenix::CommException& e) {
           // Failure detected and recovered
           std::cerr << "Recovered from failure: " << e.what() << std::endl;
       }

       Fenix_Finalize();
       MPI_Finalize();
       return 0;
   }

**Common Pitfalls:**

- **Using MPI_COMM_WORLD after initialization**: Always use the ``newcomm`` communicator for MPI operations after Fenix_Init, not the original communicator.
- **Variable corruption with longjmp**: When using FENIX_RESUME_JUMP (default), local variables may be corrupted after recovery. Declare important variables as ``volatile`` or use heap allocation.
- **Forgetting to check role**: Applications should check the returned role to determine if they need to restore state (RECOVERED_RANK) or continue normally.
- **Not enough spares**: If you don't allocate enough spare ranks, the communicator will shrink when spares are depleted, which may affect parallel algorithms that assume a fixed size.

**Performance Considerations:**

- Spare ranks consume resources while waiting. Configure :c:enumerator:`FENIX_SPARE_WAIT_MODE` to balance responsiveness vs. CPU usage.
- For large-scale applications, consider using FENIX_SPARE_WAIT_SLEEP to reduce busy-waiting overhead.

.. seealso::
   :c:func:`Fenix_Finalize`, :c:type:`Fenix_Rank_role`, :c:func:`Fenix_set_option`, :doc:`/guides/process-recovery`
