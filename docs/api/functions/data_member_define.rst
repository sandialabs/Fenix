member_define
=============

.. operation:: local

As the corresponding :cpp:func:member_create function, but it is not an error
if the member already exists.

.. c:function:: int Fenix_Data_member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

.. c:function:: int Fenix_Data_member_fdefine(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, Fenix_Serialize_file_fn serializer, void* ctx)

.. cpp:function:: int fenix::data::member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype)

.. cpp:function:: int fenix::data::member_define(int group_id, int member_id, void* buffer, int count, MPI_Datatype datatype, SerializeFunc serializer)

   See :cpp:func:member_create for argument definitions.

If the member already exists, these functions will update the member's
attributes per :cpp:func:member_attr_set and will update (or remove) the
member's serializer.

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_member_attr_set`, :doc:`/guides/data-recovery`
