member_delete
=============

.. operation:: local

Delete a data member from a group.

.. c:function:: int Fenix_Data_member_delete(int group_id, int member_id)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to delete. Frees the member's metadata and any stored checkpoint data. Must exist in the specified group.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_delete(int group_id, int member_id)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to delete
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_created`
