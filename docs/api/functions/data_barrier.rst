data_barrier
============

.. operation:: collective

Block until all ranks in the group have reached this point.

.. c:function:: int Fenix_Data_barrier(int group_id)

   :param int group_id: [in] The data group whose communicator ranks should synchronize at this barrier point. Must be a valid existing group.
   :returns: FENIX_SUCCESS if successful (when implemented)

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Barrier completed successfully (when implemented)
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized with :c:func:`Fenix_Init`
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist or group_id is invalid

.. note::
   If a :cpp:class:`fenix::CommException` occurs during execution (e.g., due to rank failures), the behavior depends on the :c:type:`FENIX_RESUME_MODE` setting. The exception may be caught and handled by Fenix's error handling mechanism.

.. warning::
   **Implementation status:**

   This function is **unimplemented**.

   As a workaround, use :c:func:`Fenix_Data_commit_barrier` which provides a
   barrier synchronization as part of the commit operation.

Example
-------

The intended use case would synchronize all ranks in a data group:

.. code-block:: c

   int group_id;
   int rank, size;
   MPI_Comm world, fenix_comm;

   Fenix_Init(&argc, &argv, MPI_COMM_WORLD, &world, &fenix_comm, NULL, NULL);
   MPI_Comm_rank(fenix_comm, &rank);
   MPI_Comm_size(fenix_comm, &size);

   // Create and populate data group
   Fenix_Data_group_create(group_id, fenix_comm, 0, 7, &group_id);
   double data[100];
   int member_id;
   Fenix_Data_member_create(group_id, 0, data, 100, MPI_DOUBLE, &member_id);

   // Do some asynchronous computation
   compute_local_data(data);

   // Would synchronize all ranks in the group
   // Fenix_Data_barrier(group_id);  // UNIMPLEMENTED

   // Workaround: Use commit_barrier instead
   Fenix_Data_commit_barrier(group_id, &time_stamp);

.. seealso::
   :c:func:`Fenix_Data_commit_barrier`
