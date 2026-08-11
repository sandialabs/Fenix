member_fdefine
==============

.. operation:: collective

Define a data member with file-based custom serialization.

This function combines :c:func:`Fenix_Data_member_define` with the ability to specify
a custom file-based serializer function. It is the idempotent version of
:c:func:`Fenix_Data_member_fcreate`.

.. important::
   **Key Features**:

   - Idempotent - creates member if it doesn't exist, or updates attributes and serializer if it does
   - Enables custom serialization for complex data structures that cannot be represented with MPI datatypes alone
   - Uses FILE* interface for serialization, allowing standard I/O operations
   - Can remove serializer by passing NULL

.. note::
   For function signature and parameter details, see the serialization overload of :c:func:`Fenix_Data_member_define`.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Member defined successfully with serializer
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_CREATE` - Failed to create/update member
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` - Cannot set serializer while staging operation is active

When to Use
-----------

Use ``member_fdefine`` when:

- Your data structure is too complex for MPI datatypes alone (linked lists, trees, graphs)
- You need custom packing/unpacking logic
- You want FILE*-based I/O operations for serialization
- Your buffer address may change (due to realloc) and you need idempotent updates
- You need to remove or change a serializer on an existing member

Serializer Function
-------------------

The serializer function signature is defined by :c:type:`Fenix_Serialize_file_fn`:

.. code-block:: c

   typedef void (*Fenix_Serialize_file_fn)(
       FILE* fp,      // Memory-backed file pointer
       int direction, // FENIX_SERIALIZE or FENIX_DESERIALIZE
       void* buf,     // User buffer containing the data
       int offset,    // First element index to process
       int count,     // Number of elements to process
       void* context  // User-provided context
   );

The serializer will be invoked by Fenix during store and restore operations:

- **During store**: ``direction=FENIX_SERIALIZE``, write data from ``buf`` to ``fp``
- **During restore**: ``direction=FENIX_DESERIALIZE``, read data from ``fp`` into ``buf``

Usage Examples
--------------

**Example 1: Linked List Serialization**

.. code-block:: c

   // Define a linked list structure
   typedef struct Node {
       int value;
       struct Node* next;
   } Node;

   typedef struct {
       Node* head;
       int count;
   } LinkedList;

   // Custom serializer for linked list
   void serialize_linked_list(FILE* fp, int direction, void* buf,
                              int offset, int count, void* ctx) {
       LinkedList* list = (LinkedList*)buf;

       if (direction == FENIX_SERIALIZE) {
           // Write number of nodes
           fwrite(&list->count, sizeof(int), 1, fp);

           // Write each node's value
           Node* current = list->head;
           for (int i = 0; i < list->count && current != NULL; i++) {
               fwrite(&current->value, sizeof(int), 1, fp);
               current = current->next;
           }
       } else { // FENIX_DESERIALIZE
           // Read number of nodes
           int node_count;
           fread(&node_count, sizeof(int), 1, fp);

           // Rebuild the linked list
           Node* prev = NULL;
           for (int i = 0; i < node_count; i++) {
               Node* new_node = malloc(sizeof(Node));
               fread(&new_node->value, sizeof(int), 1, fp);
               new_node->next = NULL;

               if (prev == NULL) {
                   list->head = new_node;
               } else {
                   prev->next = new_node;
               }
               prev = new_node;
           }
           list->count = node_count;
       }
   }

   // Create the member with custom serializer
   LinkedList my_list = {NULL, 0};
   // ... populate the list ...

   int ret = Fenix_Data_member_fdefine(
       group_id, member_id,
       &my_list,           // Pointer to the list structure
       1,                  // One "element" (the entire list)
       MPI_BYTE,           // Use MPI_BYTE for custom serialization
       serialize_linked_list,
       NULL                // No extra context needed
   );

**Example 2: Dynamic Buffer with Reallocation**

.. code-block:: c

   typedef struct {
       double* data;
       int size;
       int capacity;
   } DynamicArray;

   void serialize_dynamic_array(FILE* fp, int direction, void* buf,
                                int offset, int count, void* ctx) {
       DynamicArray* arr = (DynamicArray*)buf;

       if (direction == FENIX_SERIALIZE) {
           fwrite(&arr->size, sizeof(int), 1, fp);
           fwrite(arr->data, sizeof(double), arr->size, fp);
       } else {
           fread(&arr->size, sizeof(int), 1, fp);
           // Reallocate if needed
           if (arr->capacity < arr->size) {
               arr->data = realloc(arr->data, arr->size * sizeof(double));
               arr->capacity = arr->size;
           }
           fread(arr->data, sizeof(double), arr->size, fp);
       }
   }

   DynamicArray array = {NULL, 0, 0};

   // Initial definition
   Fenix_Data_member_fdefine(
       group_id, member_id,
       &array, FENIX_RESIZEABLE, MPI_BYTE,
       serialize_dynamic_array, NULL
   );

   // ... add data to array, possibly causing reallocation ...

   // Idempotent redefinition after buffer may have moved
   // (though in this case the array struct address stays the same)
   Fenix_Data_member_fdefine(
       group_id, member_id,
       &array, FENIX_RESIZEABLE, MPI_BYTE,
       serialize_dynamic_array, NULL
   );

**Example 3: Using Context Parameter**

.. code-block:: c

   typedef struct {
       int compression_level;
       char magic_bytes[4];
   } SerializerConfig;

   void serialize_with_config(FILE* fp, int direction, void* buf,
                              int offset, int count, void* ctx) {
       SerializerConfig* config = (SerializerConfig*)ctx;

       if (direction == FENIX_SERIALIZE) {
           // Write magic bytes
           fwrite(config->magic_bytes, 1, 4, fp);

           // Write data with compression level applied
           double* data = (double*)buf;
           for (int i = offset; i < offset + count; i++) {
               double value = data[i];
               // Apply compression logic based on config->compression_level
               fwrite(&value, sizeof(double), 1, fp);
           }
       } else {
           // Verify magic bytes
           char magic[4];
           fread(magic, 1, 4, fp);
           if (memcmp(magic, config->magic_bytes, 4) != 0) {
               fprintf(stderr, "Invalid magic bytes!\n");
               return;
           }

           // Read decompressed data
           double* data = (double*)buf;
           for (int i = offset; i < offset + count; i++) {
               fread(&data[i], sizeof(double), 1, fp);
           }
       }
   }

   SerializerConfig config = {5, {'F', 'N', 'X', '1'}};
   double data[1000];

   Fenix_Data_member_fdefine(
       group_id, member_id,
       data, 1000, MPI_DOUBLE,
       serialize_with_config,
       &config  // Pass config as context
   );

**Example 4: Removing Serializer**

.. code-block:: c

   // Initially create with serializer
   Fenix_Data_member_fdefine(
       group_id, member_id,
       buffer, count, MPI_DOUBLE,
       my_serializer, ctx
   );

   // Later, remove the serializer (revert to default memcpy)
   Fenix_Data_member_fdefine(
       group_id, member_id,
       buffer, count, MPI_DOUBLE,
       NULL,  // NULL removes the serializer
       NULL
   );

Idempotent Behavior
-------------------

Like :c:func:`Fenix_Data_member_define`, this function is idempotent:

- **First call**: Creates the member with the serializer
- **Subsequent calls**: Updates the buffer, count, datatype, and serializer

.. code-block:: c

   // First call creates member with serializer
   Fenix_Data_member_fdefine(group_id, member_id, buf1, 100, MPI_DOUBLE,
                             serializer1, ctx1);

   // Second call updates buffer and serializer
   Fenix_Data_member_fdefine(group_id, member_id, buf2, 200, MPI_DOUBLE,
                             serializer2, ctx2);

   // Now the member uses buf2, count=200, and serializer2

This is particularly useful in recovery callbacks where you may not know if the member
already exists.

Common Pitfalls
---------------

- **FILE* position manipulation**: Don't use ``fseek``, ``ftell``, or ``rewind`` on the FILE* pointer. Only read/write sequentially.
- **Mismatched read/write sizes**: Ensure the same amount of data is written during SERIALIZE and read during DESERIALIZE.
- **Memory leaks in deserializer**: When deserializing, remember to free any old allocated memory before allocating new.
- **Context lifetime**: The context pointer must remain valid for the lifetime of the member (until it's deleted or the serializer is removed).
- **Serializer with NULL context**: If you don't need context, pass NULL (not required to pass anything).
- **Active staging operations**: Cannot set/change serializer while a staging operation is active (will raise FENIX_ERROR_MEMBER_STAGING).

Relationship to Other Functions
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Function
     - Relationship
   * - :c:func:`Fenix_Data_member_fcreate`
     - Non-idempotent version (fails if member exists)
   * - :c:func:`Fenix_Data_member_define`
     - Idempotent version without custom serializer
   * - :c:func:`Fenix_Data_member_create`
     - Non-idempotent version without custom serializer
   * - :c:func:`Fenix_Data_member_store`
     - Uses the serializer during store operations
   * - :c:func:`Fenix_Data_member_restore`
     - Uses the serializer during restore operations
   * - :c:func:`Fenix_Data_member_stage_begin`
     - Alternative approach for manual staging with FILE*

.. note::
   The serializer function will be invoked by Fenix during:

   - :c:func:`Fenix_Data_member_store` and :c:func:`Fenix_Data_member_storev`
   - :c:func:`Fenix_Data_member_stage`
   - :c:func:`Fenix_Data_member_load` and :c:func:`Fenix_Data_member_restore`

.. seealso::
   :c:type:`Fenix_Serialize_file_fn`, :c:func:`Fenix_Data_member_fcreate`, :c:func:`Fenix_Data_member_define`, :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`, :doc:`/guides/data-recovery`
