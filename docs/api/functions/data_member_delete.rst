member_delete
=============

.. operation:: local

Delete a data member from a group.

.. c:function:: int Fenix_Data_member_delete(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_delete(int group_id, int member_id)

   :param int group_id: The group to delete from
   :param int member_id: The member to delete
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_created`
