wait
====

.. operation:: local

Wait for a non-blocking data operation to complete.

.. c:function:: int Fenix_Data_wait(Fenix_Request request)

   :param Fenix_Request request: [in] Request handle returned from :c:func:`Fenix_Data_member_istore` or :c:func:`Fenix_Data_member_istorev`. This function blocks until the operation completes.
   :returns: FENIX_SUCCESS when the operation completes successfully, error code if operation failed

.. cpp:function:: int fenix::data::wait(Fenix_Request request)

   :param Fenix_Request request: [in] The request handle to wait on
   :returns: FENIX_SUCCESS when the operation completes

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Wait completed successfully
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized with :c:func:`Fenix_Init`
- :c:enumerator:`FENIX_ERROR_DATA_WAIT` - Failed to wait on the asynchronous data operation (when implemented, this may indicate an invalid request handle or an error in the underlying asynchronous operation)

.. warning::
   **Implementation status:**

   This function is **unimplemented**. Calling it will print a fatal error message
   and the behavior is undefined. Since the asynchronous store operations
   (:c:func:`Fenix_Data_member_istore` and :c:func:`Fenix_Data_member_istorev`)
   are also unimplemented, this function currently has no valid use case.

Example
-------

The following example shows how ``Fenix_Data_wait`` would be used with asynchronous data operations if they were implemented:

.. code-block:: c

   #include <fenix.h>

   int group_id = 0;
   int member_id = 0;
   double simulation_data[1000];
   Fenix_Request request;
   int error;

   // Initialize simulation data
   for (int i = 0; i < 1000; i++) {
       simulation_data[i] = rank * 1000.0 + i;
   }

   // Start non-blocking checkpoint of all elements
   Fenix_Data_subset subset;
   Fenix_Data_subset_create(0, 1000, 1, &subset);

   error = Fenix_Data_member_istore(group_id, member_id, subset, &request);
   if (error != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to start async checkpoint\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
   }

   // Perform computation that doesn't modify simulation_data
   // while checkpoint is in progress...
   do_independent_computation();

   // Wait for checkpoint to complete before modifying data
   error = Fenix_Data_wait(request);
   if (error != FENIX_SUCCESS) {
       fprintf(stderr, "Checkpoint operation failed\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
   }

   // Safe to modify simulation_data now
   update_simulation_data(simulation_data, 1000);

   // Commit all pending checkpoints
   Fenix_Data_commit(group_id, &error);

.. note::
   Since asynchronous operations are unimplemented, use synchronous :c:func:`Fenix_Data_member_store` instead, which completes before returning.

.. seealso::
   :c:func:`Fenix_Data_test`, :c:func:`Fenix_Data_member_istore`
