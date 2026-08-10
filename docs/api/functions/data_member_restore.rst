member_restore
==============

.. operation:: collective

Restore a data member from the checkpoint.

.. c:function:: int Fenix_Data_member_restore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: The group containing the member
   :param int member_id: The member to restore
   :param void* target_buffer: Buffer to restore into
   :param int max_count: Maximum number of elements
   :param int time_stamp: Which checkpoint version to restore
   :param Fenix_Data_subset* found_data: Subset of data that was actually found/restored
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_restore(int group_id, int member_id, void* target_buf, int max_count, int time_stamp)

   :param int group_id: The group containing the member
   :param int member_id: The member to restore
   :param void* target_buf: Buffer to restore into
   :param int max_count: Maximum number of elements
   :param int time_stamp: Which checkpoint version to restore
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_member_store`, :doc:`/guides/data-recovery`
