member_define
=============

.. operation:: collective

Define a data member with flexible buffer addressing.

This function is the idempotent version of :c:func:`Fenix_Data_member_create`. It creates
the member if it doesn't exist, or updates its attributes (including the buffer pointer,
count, and datatype) if it does.

.. important::
   **Key Differences from member_create**:

   - ``member_define`` is idempotent - calling it multiple times updates the member's buffer, count, and datatype.
   - ``member_create`` is not idempotent - fails if called twice with the same member ID.
   - Both functions save the buffer pointer for use in subsequent store/restore operations.
   - The main use case for ``member_define`` is when buffer addresses may change (e.g., after realloc) and you need to update the saved pointer.

.. c:function:: int Fenix_Data_member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: [in] Identifier to a data group within which to create the member
   :param int member_id: [in] An integer unique within the data group that identifies the data
   :param void* buffer: [in] Pointer to the data buffer. This address is saved and used for store/restore operations.
   :param int count: [in] The maximum number of contiguous elements of type datatype
   :param MPI_Datatype datatype: [in] The MPI_Datatype of the elements
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: [in] Identifier to a data group within which to create the member
   :param int member_id: [in] An integer unique within the data group
   :param void* buffer: [in] Pointer to the data buffer. This address is saved.
   :param int count: [in] Maximum number of elements
   :param MPI_Datatype datatype: [in] The MPI_Datatype
   :returns: FENIX_SUCCESS if successful

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Member defined successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_CREATE` - Failed to create/update member

**Usage Examples:**

.. code-block:: c

   // C example - Data that may move in memory
   int group_id = 1;
   int member_id = 100;

   // Initial creation with buffer address
   double* data = malloc(1000 * sizeof(double));
   Fenix_Data_member_define(
       group_id, member_id,
       data,     // Buffer pointer is saved
       1000,     // 1000 doubles
       MPI_DOUBLE
   );

   // Store using the saved buffer address
   // Use member_store for uniform subsets across ranks
   Fenix_Data_member_store(group_id, member_id,
                           FENIX_DATA_SUBSET_FULL);

   // If buffer moves due to realloc, update the saved pointer
   double* new_data = realloc(data, 2000 * sizeof(double));
   // Update the member with new buffer address
   Fenix_Data_member_define(group_id, member_id, new_data, 2000, MPI_DOUBLE);
   // Now store with the updated buffer
   Fenix_Data_member_store(group_id, member_id,
                           FENIX_DATA_SUBSET_FULL);

.. code-block:: cpp

   // C++ example with std::vector - update buffer after resize
   int group_id = 1;
   int member_id = 200;

   std::vector<double> data(1000);

   // Define member with initial buffer address
   fenix::data::member_define(
       group_id, member_id,
       data.data(),  // Buffer pointer is saved
       1000, MPI_DOUBLE
   );

   // Store using the saved buffer
   // Use member_store for uniform subsets (or member_storev for varying subsets)
   fenix::data::member_store(
       group_id, member_id,
       FENIX_DATA_SUBSET_FULL
   );

   // Vector resizes - buffer may have moved
   data.resize(2000);

   // Update the member with new buffer location
   fenix::data::member_define(group_id, member_id, data.data(), 2000, MPI_DOUBLE);

   // Restore using the updated buffer
   fenix::data::member_restore(
       group_id, member_id,
       data.data(),  // Specify buffer for restore
       2000, timestamp
   );

**Idempotent Behavior:**

Calling ``member_define`` multiple times with the same group_id and member_id is safe
and will update the member's attributes:

.. code-block:: c

   // First call creates the member
   Fenix_Data_member_define(group_id, member_id, NULL, 1000, MPI_DOUBLE);

   // Second call updates count (acts like attr_set)
   Fenix_Data_member_define(group_id, member_id, NULL, 2000, MPI_DOUBLE);

This makes it useful in recovery callbacks where you want to ensure the member exists
without checking first.

With Custom Serialization
--------------------------

For complex data structures that cannot be represented with MPI datatypes alone, use the serialization variants that accept a custom serialization function.

.. c:function:: int Fenix_Data_member_fdefine(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, Fenix_Serialize_file_fn serializer, void* ctx)

   :param int group_id: Identifier to a data group within which to create the member
   :param int member_id: An integer unique within the data group
   :param void* buffer: Address of the data to be serialized
   :param int count: Maximum number of contiguous elements
   :param MPI_Datatype datatype: The MPI_Datatype of the elements
   :param Fenix_Serialize_file_fn serializer: Serializer function to use (nullptr to remove)
   :param void* ctx: User-defined context passed to serializer
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, SerializeFunc serializer)

   :param int group_id: Identifier to a data group
   :param int member_id: Member identifier
   :param void* buffer: Address of the data
   :param int count: Maximum number of elements
   :param MPI_Datatype datatype: The MPI_Datatype
   :param SerializeFunc serializer: Serializer function
   :returns: FENIX_SUCCESS if successful

.. note::
   Providing a nullptr for serializer will remove any existing serializer.

**Common Pitfalls:**

- **Stale buffer pointers**: After realloc or vector resize, call member_define again to update the saved buffer pointer.
- **Not understanding idempotency**: Calling member_define twice updates the member's attributes (buffer, count, datatype) rather than failing.
- **Confusing store/storev choice with creation method**: Both member_create and member_define can use either store or storev - the choice depends on subset uniformity across ranks.

**Comparison Table: member_create vs member_define:**

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Feature
     - member_create
     - member_define
   * - Buffer pointer
     - Saved at creation
     - Saved at creation or updated if member exists
   * - Idempotent
     - No - fails if member exists
     - Yes - updates buffer/count/datatype if member exists
   * - Dynamic buffers
     - Not safe - pointer may become stale
     - Safe - can call again to update buffer after realloc
   * - Typical use
     - Static buffers that never move
     - Buffers that may move (realloc, vector resize)
   * - Use case
     - Fixed memory location, create once
     - May need to update buffer location

.. note::
   Both ``member_create`` and ``member_define`` can use either :c:func:`Fenix_Data_member_store`
   or :c:func:`Fenix_Data_member_storev`. The choice between ``store`` and ``storev`` depends on
   subset uniformity across ranks, not on which creation function was used:

   - Use ``member_store`` when all ranks store the same subset
   - Use ``member_storev`` when ranks may store different subsets

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_member_attr_set`, :doc:`/guides/data-recovery`
