member_lrestore
===============

.. operation:: local

Local-only version of member_restore (deprecated).

.. c:function:: int Fenix_Data_member_lrestore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: The group to restore from
   :param int member_id: The member to restore
   :param void* target_buffer: The buffer to store the restored data
   :param int max_count: The maximum number of elements to restore
   :param int time_stamp: The time stamp of the snapshot to restore from
   :param Fenix_Data_subset* found_data: The subset of the data that was found in the snapshot
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_lrestore(int group_id, int member_id, void* target_buffer = FENIX_DATA_RESTORE_INPLACE, int max_length = FENIX_DATA_RESTORE_FULL, int time_stamp = FENIX_DATA_SNAPSHOT_ALL, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: The group to restore from
   :param int member_id: The member to restore
   :param void* target_buffer: The buffer to restore to
   :param int max_length: Maximum elements to restore
   :param int time_stamp: Time stamp to restore from
   :param DataSubset data_found: Subset found
   :returns: FENIX_SUCCESS if successful

.. deprecated::
   Use member_load functions instead.

.. note::
   This function restores the data of a group member from the local snapshot.

.. seealso::
   :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_restore`
