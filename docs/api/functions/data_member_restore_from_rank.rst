member_restore_from_rank
========================

.. operation:: collective

Restore a member from a specific rank's data.

.. c:function:: int Fenix_Data_member_restore_from_rank(int group_id, int member_id, void* data, int max_count, int time_stamp, Fenix_Data_subset* found_data, int source_rank)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to restore.
   :param void* data: [out] Buffer where restored data will be written. Must have space for max_count elements.
   :param int max_count: [in] Maximum number of elements to restore into the buffer.
   :param int time_stamp: [in] Timestamp of the snapshot to restore from.
   :param Fenix_Data_subset* found_data: [out] Subset describing which element ranges were found and restored from the source rank. Pass FENIX_DATA_SUBSET_IGNORE if not needed.
   :param int source_rank: [in] Rank (in the group's communicator) from which to copy checkpoint data directly, bypassing redundant storage reconstruction.
   :returns: FENIX_SUCCESS if successful (when implemented)

.. warning::
   **Implementation status:**

   This function is **unimplemented**. Calling it will print a fatal error message
   and the behavior is undefined.

   As a workaround, use :c:func:`Fenix_Data_member_restore` to restore from the
   redundant storage, which automatically reconstructs data from available ranks.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Data restored successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - No data found at the specified timestamp
- :c:enumerator:`FENIX_WARNING_PARTIAL_RESTORE` - Only partial data was restored

.. seealso::
   :c:func:`Fenix_Data_member_restore`
