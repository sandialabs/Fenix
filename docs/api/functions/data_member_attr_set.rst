member_attr_set
===============

.. operation:: local

Set the value of a member's attribute.

.. c:function:: int Fenix_Data_member_attr_set(int group_id, int member_id, int attribute_name, void* attribute_value, int* flag)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member whose attribute to modify. Must exist in the specified group.
   :param int attribute_name: [in] The attribute to set. Valid values: FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER (buffer address), FENIX_DATA_MEMBER_ATTRIBUTE_COUNT (element count), or FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE (MPI datatype). COUNT and DATATYPE may only be set before the first store operation.
   :param void* attribute_value: [in] Pointer to new value for the attribute. Type depends on attribute being set.
   :param int* flag: [out] Set to 1 if the attribute was successfully set, 0 otherwise.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_attr_set(int group_id, int member_id, int attr, void* value, int* flag)

   :param int group_id: [in] The data group to update
   :param int member_id: [in] The member to update
   :param int attr: [in] The attribute to set. Valid: BUFFER, COUNT, or DATATYPE. COUNT and DATATYPE only settable before first store.
   :param void* value: [in] Pointer to new attribute value
   :param int* flag: [out] Set to 1 if successful, 0 otherwise
   :returns: FENIX_SUCCESS if successful

.. note::
   Valid names are FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
   and FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE. The COUNT and DATATYPE attributes may only
   be set before the first store operation. Contrary to the Fenix specification, returning
   to Fenix_Init after a failure does not allow the user to set these attributes again.

**Usage Examples:**

.. code-block:: c

   // C example - Update buffer address after reallocation
   int group_id = 1;
   int member_id = 100;
   int flag;

   // Create member with initial buffer
   double* data = malloc(1000 * sizeof(double));
   Fenix_Data_member_create(group_id, member_id, data, 1000, MPI_DOUBLE);

   // Later, need to reallocate to larger buffer
   data = realloc(data, 2000 * sizeof(double));

   // Update buffer address in Fenix
   int ret = Fenix_Data_member_attr_set(
       group_id,
       member_id,
       FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
       &data,  // Pointer to new buffer address
       &flag
   );

   if (ret == FENIX_SUCCESS && flag == 1) {
       printf("Buffer address updated successfully\n");
   } else {
       fprintf(stderr, "Failed to update buffer address\n");
   }

   // Now can store from new buffer location
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);

.. code-block:: c

   // Example - Update count and datatype before first checkpoint
   int group_id = 1;
   int member_id = 200;
   int flag;

   // Create member with placeholder values
   double* buffer = malloc(100 * sizeof(double));
   Fenix_Data_member_create(group_id, member_id, buffer, 100, MPI_DOUBLE);

   // Before first store, can update count and datatype
   int new_count = 500;
   int ret = Fenix_Data_member_attr_set(
       group_id, member_id,
       FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
       &new_count,
       &flag
   );

   if (ret != FENIX_SUCCESS || flag != 1) {
       fprintf(stderr, "Failed to update count\n");
   }

   // Note: Cannot modify COUNT or DATATYPE after first store!
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);

   // This would fail - already stored once
   int newer_count = 1000;
   Fenix_Data_member_attr_set(
       group_id, member_id,
       FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
       &newer_count,
       &flag
   );
   // flag will be 0 (failed)

.. code-block:: cpp

   // C++ example with std::vector that may reallocate
   int group_id = 1;
   int member_id = 300;
   int flag;

   std::vector<double> data(1000);

   // Create member
   fenix::data::member_create(group_id, member_id, data.data(), 1000, MPI_DOUBLE);

   // Vector resizes - buffer may have moved!
   data.resize(2000);

   // Update Fenix with new buffer address
   void* new_buffer = data.data();
   fenix::data::member_attr_set(
       group_id, member_id,
       FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
       &new_buffer,
       &flag
   );

   if (flag == 1) {
       // Safe to checkpoint now
       fenix::data::member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   }

**Common Use Cases:**

- **Dynamic memory reallocation**: Update BUFFER attribute when using realloc() or std::vector::resize()
- **Pre-store configuration**: Set COUNT or DATATYPE before first checkpoint if initial values were placeholders
- **Pointer updates**: Keep Fenix in sync when application data structures are reorganized in memory

**Important Restrictions:**

- **COUNT and DATATYPE are immutable after first store**: Once you've checkpointed a member, these attributes are locked
- **BUFFER can always be updated**: Even after storing, you can update the buffer address for subsequent operations
- **Always check the flag**: If flag returns 0, the attribute was not set (likely due to restrictions or invalid parameters)

.. seealso::
   :c:func:`Fenix_Data_member_attr_get`, :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_define`
