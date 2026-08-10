member_define
=============

.. operation:: collective

Idempotent version of member_create.

If this member does not exist, behaves as :c:func:`Fenix_Data_member_create`. If this member
does exist, updates its attributes (with the same restrictions as :c:func:`Fenix_Data_member_attr_set`).

.. c:function:: int Fenix_Data_member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: Identifier to a data group within which to create the member
   :param int member_id: An integer unique within the data group that identifies the data in buffer
   :param void* buffer: Address of the data to be copied to redundant storage
   :param int count: The maximum number of contiguous elements of type datatype of the data to be stored
   :param MPI_Datatype datatype: The MPI_Datatype of the elements in buffer
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: Identifier to a data group within which to create the member
   :param int member_id: An integer unique within the data group
   :param void* buffer: Address of the data
   :param int count: Maximum number of elements
   :param MPI_Datatype datatype: The MPI_Datatype
   :returns: FENIX_SUCCESS if successful

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

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_attr_set`
