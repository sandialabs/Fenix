member_create
=============

.. operation:: collective

Create a data member within a group.

A data member represents a piece of application data to be checkpointed.

.. c:function:: int Fenix_Data_member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

.. cpp:function:: int fenix::data::member_create(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

   :param int group_id: The group to add this member to
   :param int member_id: Unique identifier for this member within the group
   :param void* buffer: Pointer to the data
   :param int count: Number of elements
   :param MPI_Datatype datatype: MPI datatype of elements
   :returns: FENIX_SUCCESS if successful

With Custom Serialization
--------------------------

For complex data structures that cannot be represented with MPI datatypes alone, use the serialization variants that accept a custom serialization function.

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

.. note::
   The serializer function will be invoked to serialize and deserialize the data of this member.
   See :c:type:`Fenix_Serialize_file_fn` for details on the serializer function signature.

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_define`
