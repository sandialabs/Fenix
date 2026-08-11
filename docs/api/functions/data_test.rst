test
====

.. operation:: local

Test for completion of a non-blocking data operation.

.. c:function:: int Fenix_Data_test(Fenix_Request request, int* flag)

   :param Fenix_Request request: [in] Request handle returned from :c:func:`Fenix_Data_member_istore` or :c:func:`Fenix_Data_member_istorev`.
   :param int* flag: [out] Pointer to integer set to 1 (true) if the operation has completed, 0 (false) if still in progress.
   :returns: FENIX_SUCCESS if test successful (regardless of completion status), error code otherwise

Return Codes
------------

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Return Code
     - Condition
   * - :c:enumerator:`FENIX_SUCCESS`
     - Test completed successfully and flag was set to indicate completion status
   * - :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
     - Fenix library has not been initialized via :c:func:`Fenix_Init`

.. note::
   Since this function is currently unimplemented, the actual return codes may differ
   when the function is implemented. Additional error codes that may be added in a
   future implementation include:

   - :c:enumerator:`FENIX_ERROR_DATA_WAIT` - Invalid or corrupted request handle
   - :c:enumerator:`FENIX_ERROR_CANCELLED` - The request was cancelled before completion

.. warning::
   **Implementation status:**

   This function is **unimplemented**. Calling it will print a fatal error message
   and the behavior is undefined. Since the asynchronous store operations
   (:c:func:`Fenix_Data_member_istore` and :c:func:`Fenix_Data_member_istorev`)
   are also unimplemented, this function currently has no valid use case.

Example
-------

The following example demonstrates how Fenix_Data_test would be used if it were implemented. This allows polling for completion of a non-blocking checkpoint operation while continuing computation:

.. code-block:: c

   #include <fenix.h>
   #include <stdio.h>

   int group_id = 1;
   int member_id = 1;
   double data[1000];
   Fenix_Request request;
   int flag = 0;
   int rc;

   // Initialize data
   for (int i = 0; i < 1000; i++) {
       data[i] = (double)i;
   }

   // Start non-blocking checkpoint (theoretical - currently unimplemented)
   rc = Fenix_Data_member_istore(group_id, member_id,
                                   FENIX_DATA_SUBSET_FULL, &request);
   if (rc != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to start checkpoint: %d\n", rc);
       return rc;
   }

   // Continue computation while checkpoint is in progress
   printf("Checkpoint initiated, continuing computation...\n");

   // Poll for completion while doing other work
   do {
       // Do some computational work
       for (int i = 0; i < 100; i++) {
           data[i] = data[i] * 1.1;  // Some computation
       }

       // Test if checkpoint has completed
       rc = Fenix_Data_test(request, &flag);
       if (rc != FENIX_SUCCESS) {
           fprintf(stderr, "Error testing checkpoint status: %d\n", rc);
           return rc;
       }

       if (!flag) {
           printf("Checkpoint still in progress...\n");
       }
   } while (!flag);

   printf("Checkpoint completed successfully!\n");

.. note::
   This example shows the intended usage pattern. Since both :c:func:`Fenix_Data_member_istore`
   and Fenix_Data_test are currently unimplemented, this code will not work. Use the
   synchronous :c:func:`Fenix_Data_member_store` for production code.

.. seealso::
   :c:func:`Fenix_Data_wait`, :c:func:`Fenix_Data_member_istore`
