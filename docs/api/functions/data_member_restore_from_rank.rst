member_restore_from_rank
========================

.. operation:: collective

Restore a member from a specific rank's data.

.. c:function:: int Fenix_Data_member_restore_from_rank(int group_id, int member_id, void* data, int max_count, int time_stamp, Fenix_Data_subset* found_data, int source_rank)

   :param int group_id: The group to restore from
   :param int member_id: The member to restore
   :param void* data: The buffer to store the restored data
   :param int max_count: The maximum number of elements to restore
   :param int time_stamp: The time stamp of the snapshot to restore from
   :param Fenix_Data_subset* found_data: The subset of the data that was found
   :param int source_rank: The rank to restore from
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is currently unimplemented.

.. seealso::
   :c:func:`Fenix_Data_member_restore`
