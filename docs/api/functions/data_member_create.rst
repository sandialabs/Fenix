member_create
=============

.. operation:: local "collective semantics"

A data member represents a piece of application data to be checkpointed.

This function raises an error if a member with the specified ``member_id`` already exists in this group.

With Standard Serialization
---------------------------

For simple memory buffers, these functions create a member that is staged by direct memory copies from ``buffer``.

.. c:function:: int Fenix_Data_member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

.. cpp:function:: int fenix::data::member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: The data group to add this member to (must already exist)
   :param int member_id: Unique identifier for this member within the group
   :param void* buffer: Pointer to the data buffer. This address is saved and used for all operations.
   :param int count: Number of elements of the given datatype, or :c:macro:`FENIX_RESIZEABLE` for variable-size data
   :param MPI_Datatype datatype: MPI datatype describing each element
   :returns: FENIX_SUCCESS if successful, error code otherwise


With Custom Serialization
--------------------------

For complex and/or non-contiguous data, use the serialization variants that accept a custom serialization function.

.. c:function:: int Fenix_Data_member_fcreate(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, Fenix_Serialize_file_fn serializer, void* ctx)

   :param int group_id: Identifier to a data group within which to create the member
   :param int member_id: An integer unique within the data group
   :param void* buffer: Address of the data to be serialized
   :param int count: Maximum number of contiguous elements
   :param MPI_Datatype datatype: The MPI_Datatype of the elements
   :param Fenix_Serialize_file_fn serializer: Serializer function to use
   :param void* ctx: User-defined context passed to serializer
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, SerializeFunc serializer)

   :param int group_id: Identifier to a data group
   :param int member_id: Member identifier
   :param void* buffer: Address of the data
   :param int count: Maximum number of elements
   :param MPI_Datatype datatype: The MPI_Datatype
   :param SerializeFunc serializer: Serializer function (file or stream based)
   :returns: FENIX_SUCCESS if successful

Serializer Functions
^^^^^^^^^^^^^^^^^^^^

.. c:function:: void c_serializer(FILE* fp, int direction, void* buffer, int offset, int count, void* context)

.. cpp:function:: void file_serializer(FILE* fp, int direction, void* buffer, int offset, int count)

.. cpp:function:: void stream_serializer(std::iostream& stream, int direction, void* buffer, int offset, int count)

   :param FILE* fp: Memory-backed file pointer to (de)serialize with
   :param std::iostream& stream: Memory-backed std::iostream to (de)serialize with (C++-only)
   :param int direction: One of FENIX_SERIALIZE or FENIX_DESERIALIZE
   :param void* buffer: The buffer to (de)serialize to
   :param int offset: The first element index to be (de)serialized
   :param int count: The number of elements to be (de)serialized
   :param void* context: The context pointer passed during member creation (C-only)

Fenix can invoke a user-provided function with one of these signatures to
(de)serialize a given data member. The function is invoked for each contiguous
element set to be (de)serialized.

Serialization is writing application data into the file or stream for staging.
The function must write elements [offset, offset+count) into the file or stream.
The buffer parameter will be set to the data member's current buffer attribute.

Deserialization is reading the file or stream into application data for loading.
The function may read elements [offset, offset+count) from the file or stream.
The buffer parameter will be set to the data member's current buffer attribute,
or to the target buffer location if :cpp:func:`fenix::data::member_load` was
called with an explicit target buffer.

Note that a member's buffer attribute is never directly read from or written to
by Fenix if that member has a serializer. In that case, the buffer attribute and
target buffer are exclusively used by the application, so the application may
choose for itself how to define and interpret them.

When invoked, the file or stream will already be positioned appropriately for
the given reads or writes. Applications may safely reposition the file or
stream, but the following behavior is undefined:

* Reading data during serialization or writing during deserialization. This may
  or may not be detected and blocked at runtime.
* Writing to any element not included in [offset, offset+count) during
  serialization (excepting dynamically sized serialization, see below).
* Reading from any element not staged as part of the snapshot being
  deserialized.

**Dynamically Sized Serialization:**
For data members without a serializer, it is erroneous to stage SUBSET_FULL to
a member of size FENIX_RESIZEABLE. However, this behavior is acceptable for
members with a serializer. In that case, the serializer will be invoked with
offset=0 and count=FENIX_RESIZEABLE. The function should write all available
data to the file or stream. The element count will be inferred from the
resulting size of the file or stream. It is erroneous for the file or stream to
have a size not divisble by the member's element size upon return. The staged
values of any elements not written to (skipped) by the serializer are undefined.

**Custom Serialization Example:**

.. code-block:: c

   // Example with linked list that needs custom serialization
   typedef struct Node {
       int value;
       struct Node* next;
   } Node;

   void serialize_list(FILE* fp, void* buffer, int dir, int offset, int count, void* ctx) {
       Node* n = (Node*)buffer;
       for (int i = 0; i < offset; i++) n = n->next;
       for (int i = 0; i < count; i++) {
           if (dir == FENIX_SERIALIZE) fwrite(&n->value, sizeof(int), 1, fp);
           else fread(&n->value, sizeof(int), 1, fp);
       }
   }

   int size = 10;
   Node* head = create_linked_list(size);
   Fenix_Data_member_fcreate(
       group_id, member_id,
       head, size, MPI_INT,
       serialize_list,
       NULL  // No extra context needed
   );

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_member_define`, :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`, :doc:`/guides/data-recovery`
