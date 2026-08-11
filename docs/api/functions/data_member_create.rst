member_create
=============

.. operation:: collective

Create a data member within a group with a fixed buffer location.

A data member represents a piece of application data to be checkpointed. This function
creates a member where the buffer address is fixed - Fenix will always read from and
write to the specified buffer address during store and restore operations.

.. important::
   **Difference from member_define**:

   Creates a new data member. Fails if the member already exists (non-idempotent). For an idempotent version that updates existing members, use :c:func:`Fenix_Data_member_define`.

   Both functions save buffer pointers for use in subsequent operations.

.. c:function:: int Fenix_Data_member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

.. cpp:function:: int fenix::data::member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: [in] The data group to add this member to (must already exist)
   :param int member_id: [in] Unique identifier for this member within the group
   :param void* buffer: [in] Pointer to the data buffer. This address is saved and used for all operations.
   :param int count: [in] Number of elements of the given datatype, or :c:macro:`FENIX_RESIZEABLE` for variable-size data
   :param MPI_Datatype datatype: [in] MPI datatype describing each element
   :returns: FENIX_SUCCESS if successful, error code otherwise

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Member created successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_EXISTS` - Member with this ID already exists in the group
- :c:enumerator:`FENIX_ERROR_MEMBER_CREATE` - Failed to create member (internal error)

**Basic Usage Examples:**

.. code-block:: c

   // C example - Create a member for a static array
   int group_id = 1;
   int member_id = 100;

   // Static array that won't move in memory
   static double simulation_data[1000];

   int ret = Fenix_Data_member_create(
       group_id,
       member_id,
       simulation_data,      // Fixed buffer address
       1000,                 // 1000 elements
       MPI_DOUBLE
   );

   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to create member: %d\n", ret);
   }

   // Later, store without specifying buffer again
   // Can use member_store (uniform subsets) or member_storev (varying subsets)
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);

.. code-block:: cpp

   // C++ example with std::vector - be careful!
   int group_id = 1;
   int member_id = 200;

   std::vector<double> data(1000);

   // CAUTION: vector may reallocate and move!
   // Only safe if vector size is fixed
   fenix::data::member_create(
       group_id, member_id,
       data.data(),  // Buffer may become invalid if vector resizes!
       1000,
       MPI_DOUBLE
   );

With Custom Serialization
--------------------------

For complex data structures that cannot be represented with MPI datatypes alone, use the serialization variants that accept a custom serialization function.

.. c:function:: int Fenix_Data_member_fcreate(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, Fenix_Serialize_file_fn serializer, void* ctx)

   :param int group_id: [in] Identifier to a data group within which to create the member
   :param int member_id: [in] An integer unique within the data group
   :param void* buffer: [in] Address of the data to be serialized
   :param int count: [in] Maximum number of contiguous elements
   :param MPI_Datatype datatype: [in] The MPI_Datatype of the elements
   :param Fenix_Serialize_file_fn serializer: [in] Serializer function to use
   :param void* ctx: [in] User-defined context passed to serializer
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, SerializeFunc serializer)

   :param int group_id: [in] Identifier to a data group
   :param int member_id: [in] Member identifier
   :param void* buffer: [in] Address of the data
   :param int count: [in] Maximum number of elements
   :param MPI_Datatype datatype: [in] The MPI_Datatype
   :param SerializeFunc serializer: [in] Serializer function (file or stream based)
   :returns: FENIX_SUCCESS if successful

.. note::
   The serializer function will be invoked to serialize and deserialize the data of this member.
   See :c:type:`Fenix_Serialize_file_fn` for details on the serializer function signature.

**Custom Serialization Example:**

.. code-block:: c

   // Example with linked list that needs custom serialization
   typedef struct Node {
       int value;
       struct Node* next;
   } Node;

   void serialize_list(FILE* fp, int mode, void* buffer, int count, int datatype_size, void* ctx) {
       Node* head = (Node*)buffer;
       // Serialize the linked list to the file
       for (Node* n = head; n != NULL; n = n->next) {
           fwrite(&n->value, sizeof(int), 1, fp);
       }
   }

   Node* head = create_linked_list();
   Fenix_Data_member_fcreate(
       group_id, member_id,
       head, 1, MPI_BYTE,
       serialize_list,
       NULL  // No extra context needed
   );

**Common Pitfalls:**

- **Dynamic memory that moves**: If using dynamically allocated memory that may be reallocated (like std::vector), the buffer pointer may become invalid. Use :c:func:`Fenix_Data_member_define` instead.
- **Deallocating the buffer**: Don't free or deallocate the buffer while the member exists. Fenix keeps the pointer.
- **Stack-allocated data**: Don't create members for stack-local variables that will go out of scope.
- **Overlapping members**: Different members can't point to overlapping memory regions.

**When to Use member_create vs member_define:**

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Scenario
     - Use member_create
     - Use member_define
   * - Creating new members
     - Non-idempotent (fails if exists)
     - Idempotent (updates if exists)
   * - Repeated initialization
     - Fails on second call
     - Updates member on each call
   * - Error on duplicate
     - Yes - returns error code
     - No - silently updates
   * - Use case
     - When duplicates indicate bugs
     - When re-initialization is normal

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_member_define`, :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`, :doc:`/guides/data-recovery`
