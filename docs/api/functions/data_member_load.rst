member_load
===========

.. operation:: local

Load a member's committed data into user's data.

.. c:function:: int Fenix_Data_member_load(int group_id, int member_id, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param int time_stamp: Time stamp of the snapshot to load
   :param Fenix_Data_subset* found_data: Subset of the elements successfully loaded
   :returns: FENIX_SUCCESS if successful, FENIX_WARNING_PARTIAL_RESTORE if partial

.. cpp:function:: int fenix::data::member_load(int group_id, int member_id, int time_stamp = FENIX_DATA_SNAPSHOT_ALL, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param int time_stamp: Time stamp of the snapshot to load
   :param DataSubset data_found: Subset successfully loaded
   :returns: FENIX_SUCCESS if successful

.. note::
   Attempts to load up to ATTRIBUTE_COUNT elements into ATTRIBUTE_BUFFER. For members without
   a serializer, data is loaded by directly copying memory. Otherwise, data is loaded by calls
   to this member's serializer.

.. note::
   If time stamp is FENIX_DATA_SNAPSHOT_ALL, attempts to load each element from the most recent
   available snapshot the individual element was committed in. User is responsible for freeing
   the subset returned in found_data, unless found_data is FENIX_DATA_SUBSET_IGNORE.

.. seealso::
   :c:func:`Fenix_Data_member_load_to`, :c:func:`Fenix_Data_member_restore`
