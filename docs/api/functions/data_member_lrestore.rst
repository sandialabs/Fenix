member_lrestore
===============

.. operation:: local

Local-only version of member_restore (deprecated).

.. c:function:: int Fenix_Data_member_lrestore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to restore from local checkpoint only (does not reconstruct from redundant storage).
   :param void* target_buffer: [out] Buffer where restored data will be written. Must have space for max_count elements.
   :param int max_count: [in] Maximum number of elements to restore into target_buffer.
   :param int time_stamp: [in] Timestamp of the snapshot to restore from. Use FENIX_DATA_SNAPSHOT_ALL to restore from most recent snapshot for each element.
   :param Fenix_Data_subset* found_data: [out] Subset describing which element ranges were successfully restored. Pass FENIX_DATA_SUBSET_IGNORE if not needed.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_lrestore(int group_id, int member_id, void* target_buffer = FENIX_DATA_RESTORE_INPLACE, int max_length = FENIX_DATA_RESTORE_FULL, int time_stamp = FENIX_DATA_SNAPSHOT_ALL, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: [in] The group to restore from
   :param int member_id: [in] The member to restore (local only)
   :param void* target_buffer: [out] Buffer to restore to. Default: FENIX_DATA_RESTORE_INPLACE (use member's buffer).
   :param int max_length: [in] Maximum elements to restore. Default: FENIX_DATA_RESTORE_FULL (all available).
   :param int time_stamp: [in] Timestamp to restore from. Default: FENIX_DATA_SNAPSHOT_ALL (most recent).
   :param DataSubset data_found: [out] Element ranges restored. Default: SUBSET_IGNORE.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. deprecated::
   Use member_load functions instead.

.. note::
   This function restores the data of a group member from the local snapshot.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Data restored successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - No data found at the specified timestamp
- :c:enumerator:`FENIX_WARNING_PARTIAL_RESTORE` - Only partial data was restored

.. seealso::
   :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_repair`
