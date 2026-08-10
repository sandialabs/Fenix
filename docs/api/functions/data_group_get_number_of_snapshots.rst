group_get_number_of_snapshots
=============================

.. operation:: local

Get the number of locally-available snapshots in a data group.

.. c:function:: int Fenix_Data_group_get_number_of_snapshots(int group_id, int* number_of_snapshots)

   :param int group_id: The group to query
   :param int* number_of_snapshots: The number of snapshots in the group
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: std::optional<std::vector<int>> fenix::data::group_snapshots(int group_id)

   :param int group_id: The group to query
   :returns: Vector of timestamps of each snapshot in group_id if group exists

.. note::
   May include snapshots that are inconsistent across the group.

.. seealso::
   :c:func:`Fenix_Data_group_get_snapshot_at_position`, :c:func:`Fenix_Data_commit`
