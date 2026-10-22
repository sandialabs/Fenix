member_restore
==============

.. operation:: collective

Helper function to repair and load a data member. This function can be matched remotely by :cpp:func:`member_repair`.

.. c:function:: int Fenix_Data_member_restore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* data_found)

.. cpp:function:: int fenix::data::member_restore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, DataSubset& data_found)

   :param int group_id: The group containing the member
   :param int member_id: The member to restore
   :param void* target_buf: Buffer to restore into. Default: FENIX_DATA_RESTORE_INPLACE
   :param int max_count: Maximum number of elements. Default: FENIX_DATA_RESTORE_FULL
   :param int time_stamp: Which checkpoint version to restore. Default: FENIX_DATA_SNAPSHOT_ALL
   :param DataSubset& data_found: The loaded subset of data. Default: SUBSET_IGNORE
   :returns: FENIX_SUCCESS if successful

This function behaves as if calling:
#. :cpp:func:`member_repair`
#. :cpp:func:`member_attr_set` to set ATTRIBUTE_BUFFER to target_buffer, if the member did not exist already.
#. :cpp:func:`member_load_to`

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_repair`, :doc:`/guides/data-recovery`
