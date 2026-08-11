finalize
========

.. operation:: collective

Finalize Fenix and clean up all resources.

This function must be called by all active ranks before MPI_Finalize. It frees all
Fenix internal resources including data groups, callbacks, and message logs. Any
remaining spare ranks will automatically exit (default) or be released per the
:c:enumerator:`FENIX_SPARE_FINALIZE_MODE` setting.

.. c:function:: int Fenix_Finalize()

   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: void fenix::finalize()

   C++ wrapper for Fenix_Finalize.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Successfully finalized
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix was not initialized

**Usage Examples:**

.. code-block:: c

   // C example - typical finalization sequence
   int main(int argc, char** argv) {
       MPI_Init(&argc, &argv);

       int role, error;
       MPI_Comm fenix_comm;
       Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

       // Application work...

       // Clean up Fenix before MPI
       int ret = Fenix_Finalize();
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Fenix_Finalize failed with error %d\n", ret);
       }

       MPI_Finalize();
       return 0;
   }

.. code-block:: cpp

   // C++ example
   fenix::finalize();
   MPI_Finalize();

**Behavior with Spare Ranks:**

By default (:c:enumerator:`FENIX_SPARE_FINALIZE_EXIT`), spare ranks that were never
activated will call MPI_Finalize and exit when active ranks call Fenix_Finalize.

If :c:enumerator:`FENIX_SPARE_FINALIZE_RELEASE` is configured, spare ranks will
instead return from Fenix_Init with role :c:enumerator:`FENIX_ROLE_SPARE_RANK`,
allowing the application to use them for other purposes.

**Common Pitfalls:**

- **Calling MPI_Finalize first**: Always call Fenix_Finalize before MPI_Finalize.
- **Not all ranks calling finalize**: All active ranks must call Fenix_Finalize collectively.
- **Forgetting to clean up data groups**: While Fenix_Finalize cleans up internal state, applications should delete data groups explicitly for cleaner code.

.. seealso::
   :c:func:`Fenix_Init`, :c:func:`Fenix_set_option`, :c:enumerator:`FENIX_SPARE_FINALIZE_MODE`
