snapshot_delete
===============

.. operation:: local

Delete a snapshot from a data group.

.. c:function:: int Fenix_Data_snapshot_delete(int group_id, int time_stamp)

.. cpp:function:: int fenix::data::snapshot_delete(int group_id, int timestamp)

   :param int group_id: The group to delete from
   :param int timestamp: The time stamp of the snapshot to delete
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_group_get_number_of_snapshots`
