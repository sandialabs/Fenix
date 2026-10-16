checkpoint
==========

.. operation:: "cohort collective"

Convenience function to store all members and commit in one operation. Supports
inline recovery with :c:enumerator:`FENIX_MLOG_RECOVERY_INLINE`.

This function combines multiple :c:func:`Fenix_Data_member_store` calls with
:c:func:`Fenix_Data_commit` into a single call, simplifying the checkpoint workflow.
All members in the group are stored with the same subset specifier, then committed
atomically.

.. c:function:: int Fenix_Data_checkpoint(int group_id, const Fenix_Data_subset subset, int num_storev, int* storev_ids, int* time_stamp)

   :param int group_id: The data group to checkpoint
   :param Fenix_Data_subset subset: Which portion of each member to checkpoint. Use ``FENIX_DATA_SUBSET_FULL`` to checkpoint all elements, or specify a custom subset to checkpoint only certain element ranges.
   :param int num_storev: Number of members requiring storev (with varying subsets per rank), or FENIX_STOREV_ALL to store all as storev
   :param int* storev_ids: Array of member IDs to store using storev. May be NULL if num_storev is 0.
   :param int* time_stamp: The timestamp of the created checkpoint, or FENIX_TIME_STAMP_IGNORE if not needed
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::checkpoint(int group_id, const DataSubset& subset, const std::vector<int>& storev_ids = {}, int* time_stamp = nullptr)

   :param int group_id: The group to checkpoint
   :param DataSubset subset: Which element ranges of each member to checkpoint
   :param std::vector<int> storev_ids: Member ids to store as storev (default: empty = use store for all)
   :param int* time_stamp: The timestamp of the checkpoint (default: nullptr = ignore)
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::checkpointv(int group_id, const DataSubset& subset, int* time_stamp = nullptr)

   :param int group_id: The data group to checkpoint. Must be a valid group created with :cpp:func:`fenix::data::group_create`.
   :param DataSubset subset: Which element ranges of each member to checkpoint. Each rank may specify different ranges. Use ``FENIX_DATA_SUBSET_FULL`` to checkpoint all elements on all ranks.
   :param int* time_stamp: The timestamp assigned to this checkpoint version. Use for later restore operations. Default: nullptr (ignore timestamp).
   :returns: FENIX_SUCCESS if successful, error code otherwise

**How It Works:**

The function iterates through all members in the group (in creation order) and:

1. For members in ``storev_ids``: calls :c:func:`Fenix_Data_member_storev` (each rank may checkpoint different element ranges)
2. For other members: calls :c:func:`Fenix_Data_member_store` (all ranks checkpoint the same element ranges)
3. Finally calls :c:func:`Fenix_Data_commit` to make checkpoint durable

The ``checkpointv`` variant is equivalent to putting all members in ``storev_ids``.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_commit_barrier`, :c:func:`Fenix_Mlog_activate`
