member_load_to
==============

.. operation:: local

Load a member's committed data into a custom destination.

.. c:function:: int Fenix_Data_member_load_to(int group_id, int member_id, void* target, int target_count, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param void* target: The custom load destination
   :param int target_count: The number of elements to attempt to load
   :param int time_stamp: Time stamp of the snapshot to load
   :param Fenix_Data_subset* found_data: Subset of the elements successfully loaded
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_load(int group_id, int member_id, void* target, int target_count = FENIX_DATA_RESTORE_FULL, int time_stamp = FENIX_DATA_SNAPSHOT_ALL, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param void* target: The custom load destination
   :param int target_count: Number of elements to load
   :param int time_stamp: Time stamp of the snapshot to load
   :param DataSubset data_found: Subset successfully loaded
   :returns: FENIX_SUCCESS if successful

.. note::
   As :c:func:`Fenix_Data_member_load`, but with a custom load destination. Attempts to load
   up to target_count elements into target. If target_count is FENIX_DATA_RESTORE_FULL, assumes
   buffer has space to load all available elements.

.. seealso::
   :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_restore`
