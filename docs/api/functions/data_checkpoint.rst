checkpoint
==========

.. operation:: collective

Store all members of a group and then commit that group.

.. c:function:: int Fenix_Data_checkpoint(int group_id, const Fenix_Data_subset subset, int num_storev, int* storev_ids, int* time_stamp)

   :param int group_id: The group to checkpoint
   :param Fenix_Data_subset subset: The subset of each member to store
   :param int num_storev: Size of the storev_ids array, or FENIX_STOREV_ALL
   :param int* storev_ids: Array of member ids to store as storev (may be null if num_storev is zero or FENIX_STOREV_ALL)
   :param int* time_stamp: The time stamp of the commit, or FENIX_TIME_STAMP_IGNORE
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::checkpoint(int group_id, const DataSubset& subset, const std::vector<int>& storev_ids = {}, int* time_stamp = nullptr)

   :param int group_id: The group to checkpoint
   :param DataSubset subset: The subset of each member to store
   :param std::vector<int> storev_ids: [in] Member ids to store as storev
   :param int* time_stamp: The time stamp of the commit
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::checkpointv(int group_id, const DataSubset& subset, int* time_stamp = nullptr)

   :param int group_id: The group to checkpoint
   :param DataSubset subset: The subset of each member to store
   :param int* time_stamp: The time stamp of the commit
   :returns: FENIX_SUCCESS if successful

.. note::
   Stores each member in order of creation in the group. If a member's id is listed in storev_ids,
   this is equivalent to invoking :c:func:`Fenix_Data_member_storev`. Otherwise equivalent to
   :c:func:`Fenix_Data_member_store`. After storing, equivalent to :c:func:`Fenix_Data_commit`.

.. note::
   This function supports inline recovery when it is active (see :c:func:`Fenix_Mlog_activate`).

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_commit`
