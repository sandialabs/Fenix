member_fcreate
==============

.. operation:: collective

Create a data member with custom file-based serialization.

This function creates a data member and registers a custom serialization function that
will be used to serialize and deserialize the member's data. The serialization function
operates on FILE pointers, allowing custom handling of complex data structures that
cannot be represented with MPI datatypes alone.

.. important::
   **Difference from member_create**:

   - ``member_fcreate`` creates a member WITH a custom file-based serializer
   - ``member_create`` creates a member WITHOUT a custom serializer (uses default memory copy)
   - Both are non-idempotent (fail if member already exists)
   - For idempotent version with serializer, use :c:func:`Fenix_Data_member_fdefine`

.. note::
   For function signature and parameter details, see the serialization overload of :c:func:`Fenix_Data_member_create`.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Member created successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_EXISTS` - Member with this ID already exists in the group
- :c:enumerator:`FENIX_ERROR_MEMBER_CREATE` - Failed to create member (internal error)

Serializer Function
-------------------

The serializer function must conform to the :c:type:`Fenix_Serialize_file_fn` signature (see :doc:`/api/types` for the full type definition).

**Parameters:**

- **FILE\* fp**: Memory-backed file pointer to read from or write to
- **int direction**: Either :c:macro:`FENIX_SERIALIZE` or :c:macro:`FENIX_DESERIALIZE`
- **void\* buf**: Pointer to user buffer (from member's ATTRIBUTE_BUFFER)
- **int offset**: First index to be (de)serialized
- **int count**: Number of entries to (de)serialize, or :c:macro:`FENIX_RESIZEABLE`
- **void\* context**: User-defined context pointer (from fcreate's ctx parameter)

**Serializer Behavior:**

When ``direction == FENIX_SERIALIZE``:
   - ``fp`` is write-only
   - ``buf`` is the member's buffer (from FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER)
   - Function should write data values [offset, offset+count) to ``fp``
   - If staging FENIX_DATA_SUBSET_FULL of a FENIX_RESIZEABLE member, ``offset=0`` and ``count=FENIX_RESIZEABLE``

When ``direction == FENIX_DESERIALIZE``:
   - ``fp`` is read-only
   - ``buf`` is the target buffer from the restore/load operation
   - Function should read data values [offset, offset+count) from ``fp``

**Important Constraints:**

- Do NOT manipulate the file's position indicator except by reading/writing
- Will be invoked once per contiguous data region to be (de)serialized
- Must be deterministic and consistent across all ranks for collective operations

Usage Examples
--------------

**Basic Custom Serialization (C):**

.. code-block:: c

   #include <fenix.h>

   // Simple serializer for array of integers
   void serialize_int_array(
       FILE* fp, int direction, void* buf, int offset, int count, void* ctx
   ) {
       int* data = (int*)buf;

       if (direction == FENIX_SERIALIZE) {
           // Write data to file
           size_t written = fwrite(data + offset, sizeof(int), count, fp);
           if (written != count) {
               fprintf(stderr, "Serialization failed\n");
           }
       } else {  // FENIX_DESERIALIZE
           // Read data from file
           size_t read = fread(data + offset, sizeof(int), count, fp);
           if (read != count) {
               fprintf(stderr, "Deserialization failed\n");
           }
       }
   }

   int main(int argc, char** argv) {
       // ... MPI and Fenix initialization ...

       int group_id = 1;
       int member_id = 100;
       int* data = malloc(1000 * sizeof(int));

       // Create member with custom serializer
       int ret = Fenix_Data_member_fcreate(
           group_id,
           member_id,
           data,
           1000,
           MPI_INT,
           serialize_int_array,
           NULL  // No context needed
       );

       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to create member: %d\n", ret);
           return 1;
       }

       // Now store the data
       Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);

       // ... rest of application ...
   }

**Complex Data Structure (C):**

.. code-block:: c

   // Serialize a linked list
   typedef struct Node {
       int value;
       struct Node* next;
   } Node;

   typedef struct {
       Node** head_ptr;  // Pointer to head pointer, for updating on deserialize
   } ListContext;

   void serialize_linked_list(
       FILE* fp, int direction, void* buf, int offset, int count, void* ctx
   ) {
       ListContext* context = (ListContext*)ctx;

       if (direction == FENIX_SERIALIZE) {
           Node* current = *(context->head_ptr);
           int node_count = 0;

           // First, count and write the number of nodes
           Node* temp = current;
           while (temp != NULL) {
               node_count++;
               temp = temp->next;
           }
           fwrite(&node_count, sizeof(int), 1, fp);

           // Write each node's value
           while (current != NULL) {
               fwrite(&current->value, sizeof(int), 1, fp);
               current = current->next;
           }
       } else {  // FENIX_DESERIALIZE
           int node_count;
           fread(&node_count, sizeof(int), 1, fp);

           // Free existing list if any
           Node* current = *(context->head_ptr);
           while (current != NULL) {
               Node* next = current->next;
               free(current);
               current = next;
           }

           // Rebuild the list
           Node* prev = NULL;
           for (int i = 0; i < node_count; i++) {
               Node* new_node = malloc(sizeof(Node));
               fread(&new_node->value, sizeof(int), 1, fp);
               new_node->next = NULL;

               if (prev == NULL) {
                   *(context->head_ptr) = new_node;
               } else {
                   prev->next = new_node;
               }
               prev = new_node;
           }
       }
   }

   int main(int argc, char** argv) {
       // ... initialization ...

       Node* list_head = create_list();
       ListContext context = { .head_ptr = &list_head };

       Fenix_Data_member_fcreate(
           group_id,
           member_id,
           &list_head,
           1,
           MPI_BYTE,
           serialize_linked_list,
           &context
       );

       // Store and checkpoint as normal
       Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   }

**Resizable Data (C++):**

.. code-block:: cpp

   #include <fenix.hpp>
   #include <vector>

   int main(int argc, char** argv) {
       // ... initialization ...

       int group_id = 1;
       int member_id = 100;
       std::vector<double> data;

       // Create with lambda serializer for resizable vector
       fenix::data::member_create(
           group_id,
           member_id,
           nullptr,  // Buffer managed by serializer
           FENIX_RESIZEABLE,
           MPI_DOUBLE,
           [&data](FILE* fp, int dir, void* buf, int offset, int count) {
               if (dir == FENIX_SERIALIZE) {
                   // count will be FENIX_RESIZEABLE for full subset
                   size_t size = data.size();
                   fwrite(&size, sizeof(size_t), 1, fp);
                   fwrite(data.data(), sizeof(double), size, fp);
               } else {  // FENIX_DESERIALIZE
                   size_t size;
                   fread(&size, sizeof(size_t), 1, fp);
                   data.resize(size);
                   fread(data.data(), sizeof(double), size, fp);
               }
           }
       );

       // Grow the vector dynamically
       data.resize(1000);
       for (int i = 0; i < 1000; i++) {
           data[i] = i * 0.5;
       }

       // Store will use the custom serializer
       fenix::data::member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   }

**Using Context for Multiple Buffers:**

.. code-block:: c

   typedef struct {
       double* array1;
       int* array2;
       size_t size;
   } MultiArrayContext;

   void serialize_multi_array(
       FILE* fp, int direction, void* buf, int offset, int count, void* ctx
   ) {
       MultiArrayContext* context = (MultiArrayContext*)ctx;

       if (direction == FENIX_SERIALIZE) {
           fwrite(context->array1 + offset, sizeof(double), count, fp);
           fwrite(context->array2 + offset, sizeof(int), count, fp);
       } else {
           fread(context->array1 + offset, sizeof(double), count, fp);
           fread(context->array2 + offset, sizeof(int), count, fp);
       }
   }

   int main(int argc, char** argv) {
       double* arr1 = malloc(1000 * sizeof(double));
       int* arr2 = malloc(1000 * sizeof(int));

       MultiArrayContext context = {
           .array1 = arr1,
           .array2 = arr2,
           .size = 1000
       };

       Fenix_Data_member_fcreate(
           group_id, member_id,
           NULL,  // Multiple buffers, managed by context
           1000,
           MPI_BYTE,
           serialize_multi_array,
           &context
       );
   }

When to Use Custom Serialization
---------------------------------

Use ``member_fcreate`` with custom serialization when:

- **Complex data structures**: Linked lists, trees, graphs, or other pointer-based structures
- **Non-contiguous data**: Multiple separate arrays that should be checkpointed together
- **Compressed data**: You want to compress data before storing
- **Variable-size data**: Data size changes between checkpoints (use FENIX_RESIZEABLE)
- **Custom formats**: Need to serialize to a specific format for compatibility
- **Selective serialization**: Only certain fields of a struct need checkpointing

Use plain ``member_create`` when:

- Data is a simple contiguous array
- MPI datatype accurately represents your data
- No custom processing needed during checkpoint/restore

Common Pitfalls
---------------

**1. Forgetting to handle FENIX_RESIZEABLE:**

.. code-block:: c

   // WRONG: Assumes count is always a valid number
   void bad_serializer(FILE* fp, int dir, void* buf, int off, int cnt, void* ctx) {
       fwrite((char*)buf + off, 1, cnt, fp);  // FAILS if cnt == FENIX_RESIZEABLE!
   }

   // CORRECT: Check for FENIX_RESIZEABLE
   void good_serializer(FILE* fp, int dir, void* buf, int off, int cnt, void* ctx) {
       int actual_count = (cnt == FENIX_RESIZEABLE) ? get_actual_size(buf) : cnt;
       fwrite((char*)buf + off, 1, actual_count, fp);
   }

**2. Context lifetime issues:**

.. code-block:: c

   // WRONG: Context goes out of scope
   void create_member_bad() {
       int local_var = 42;
       Fenix_Data_member_fcreate(
           group_id, member_id, data, count, MPI_INT,
           my_serializer,
           &local_var  // DANGLING POINTER when function returns!
       );
   }

   // CORRECT: Use heap-allocated or global context
   int* context = malloc(sizeof(int));
   *context = 42;
   Fenix_Data_member_fcreate(
       group_id, member_id, data, count, MPI_INT,
       my_serializer,
       context
   );

**3. File position manipulation:**

.. code-block:: c

   // WRONG: Manipulating file position
   void bad_serializer(FILE* fp, int dir, void* buf, int off, int cnt, void* ctx) {
       fseek(fp, 0, SEEK_SET);  // DON'T DO THIS!
       fwrite(buf, 1, cnt, fp);
   }

   // CORRECT: Only read/write, let Fenix manage position
   void good_serializer(FILE* fp, int dir, void* buf, int off, int cnt, void* ctx) {
       fwrite(buf, 1, cnt, fp);  // Simple write, no seeking
   }

**4. Not checking return values:**

.. code-block:: c

   // WRONG: Ignoring errors
   void bad_serializer(FILE* fp, int dir, void* buf, int off, int cnt, void* ctx) {
       fwrite(buf, sizeof(int), cnt, fp);  // Might fail silently
   }

   // CORRECT: Check for errors
   void good_serializer(FILE* fp, int dir, void* buf, int off, int cnt, void* ctx) {
       size_t written = fwrite(buf, sizeof(int), cnt, fp);
       if (written != cnt) {
           fprintf(stderr, "Serialization error: wrote %zu/%d elements\n",
                   written, cnt);
           // Handle error appropriately
       }
   }

**5. Member already exists:**

.. code-block:: c

   // This will fail the second time
   Fenix_Data_member_fcreate(group_id, member_id, data, count, MPI_INT,
                              serializer, ctx);
   // ... later ...
   Fenix_Data_member_fcreate(group_id, member_id, data, count, MPI_INT,
                              serializer, ctx);  // ERROR: FENIX_ERROR_MEMBER_EXISTS

   // Use fdefine for idempotent behavior
   Fenix_Data_member_fdefine(group_id, member_id, data, count, MPI_INT,
                              serializer, ctx);  // OK: Updates if exists

Performance Considerations
--------------------------

- **Minimize serialization overhead**: Keep serialization functions efficient
- **Buffer management**: Consider using stage operations to prepare data before storing
- **Subset operations**: Use :c:func:`Fenix_Data_member_stage` for incremental updates
- **FENIX_RESIZEABLE**: Has slight overhead vs fixed-size members

Comparison with Other Functions
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 25 25 25

   * - Function
     - Serializer
     - Idempotent
     - Use Case
   * - member_fcreate
     - Yes (file)
     - No
     - Complex data, first creation
   * - member_fdefine
     - Yes (file)
     - Yes
     - Complex data, re-initialization OK
   * - member_create
     - No
     - No
     - Simple data, first creation
   * - member_define
     - No
     - Yes
     - Simple data, re-initialization OK

.. seealso::
   :c:func:`Fenix_Data_member_fdefine`,
   :c:func:`Fenix_Data_member_create`,
   :c:func:`Fenix_Data_member_stage`,
   :c:func:`Fenix_Data_member_store`,
   :c:func:`Fenix_Data_member_restore`,
   :c:func:`Fenix_Data_group_create`,
   :doc:`/guides/data-recovery`
