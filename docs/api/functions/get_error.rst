error
=====

.. operation:: local

Get the error code from the most recent failure.

.. c:function:: int Fenix_get_error()

   Get the error code from the most recent process failure that triggered recovery. Useful for logging or conditional recovery logic based on error type.

   :returns: The MPI error code from the most recent failure, or FENIX_SUCCESS if no failures have occurred

.. cpp:function:: int fenix::error()

   :returns: The MPI error code from the most recent failure, or FENIX_SUCCESS if no failures

Return Codes
------------

- :c:enumerator:`FENIX_SUCCESS` - No failures have occurred, or all failures were successfully recovered without depleting spare ranks.

- :c:enumerator:`FENIX_WARNING_SPARE_RANKS_DEPLETED` - The most recent recovery exhausted all available spare ranks. The resilient communicator was repaired by shrinking rather than replacing failed ranks with spares. This is not an error condition, but indicates that future failures cannot be recovered by rank replacement unless ``FENIX_RECOVERY_MODE`` is set to ``FENIX_RECOVERY_SPAWN`` (currently unimplemented).

.. code-block:: c

   // C example
   int error = Fenix_get_error();
   if (error != FENIX_SUCCESS) {
       // Handle error
   }

.. code-block:: cpp

   // C++ example
   int error = fenix::error();
