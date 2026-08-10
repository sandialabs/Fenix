group_get_member_at_position
============================

.. operation:: local

Get member ID based on member index within a group.

.. c:function:: int Fenix_Data_group_get_member_at_position(int group_id, int* member_id, int position)

   :param int group_id: The group to query
   :param int* member_id: The member id at this index in the group
   :param int position: The position to check, range [0, number_of_members)
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_group_get_number_of_members`
