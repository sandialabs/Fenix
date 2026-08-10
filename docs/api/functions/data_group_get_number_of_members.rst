group_get_number_of_members
===========================

.. operation:: local

Get the number of members in a data group.

.. c:function:: int Fenix_Data_group_get_number_of_members(int group_id, int* number_of_members)

   :param int group_id: The group to query
   :param int* number_of_members: Number of members in the group
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: std::optional<std::vector<int>> fenix::data::group_members(int group_id)

   :param int group_id: The group to query
   :returns: Vector of member IDs of each member in group_id if group exists

.. seealso::
   :c:func:`Fenix_Data_group_get_member_at_position`
