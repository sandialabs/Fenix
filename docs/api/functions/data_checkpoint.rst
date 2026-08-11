checkpoint
==========

.. operation:: collective

Convenience function to store all members and commit in one operation.

This function combines multiple :c:func:`Fenix_Data_member_store` calls with
:c:func:`Fenix_Data_commit` into a single call, simplifying the checkpoint workflow.
All members in the group are stored with the same subset specifier, then committed
atomically.

.. c:function:: int Fenix_Data_checkpoint(int group_id, const Fenix_Data_subset subset, int num_storev, int* storev_ids, int* time_stamp)

   :param int group_id: [in] The data group to checkpoint
   :param Fenix_Data_subset subset: [in] Which portion of each member to checkpoint. Use ``FENIX_DATA_SUBSET_FULL`` to checkpoint all elements, or specify a custom subset to checkpoint only certain element ranges.
   :param int num_storev: [in] Number of members requiring storev (with varying subsets per rank), or FENIX_STOREV_ALL to store all as storev
   :param int* storev_ids: [in] Array of member IDs to store using storev. May be NULL if num_storev is 0.
   :param int* time_stamp: [out] The timestamp of the created checkpoint, or FENIX_TIME_STAMP_IGNORE if not needed
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::checkpoint(int group_id, const DataSubset& subset, const std::vector<int>& storev_ids = {}, int* time_stamp = nullptr)

   :param int group_id: [in] The group to checkpoint
   :param DataSubset subset: [in] Which element ranges of each member to checkpoint
   :param std::vector<int> storev_ids: [in] Member ids to store as storev (default: empty = use store for all)
   :param int* time_stamp: [out] The timestamp of the checkpoint (default: nullptr = ignore)
   :returns: FENIX_SUCCESS if successful

   **Variant:** For checkpointing all members with storev (rank-varying subsets), see :cpp:func:`fenix::data::checkpointv`.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Checkpoint completed successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` - Failed to stage one or more members
- :c:enumerator:`FENIX_ERROR_COMMIT_BARRIER` - Failed during commit

**How It Works:**

The function iterates through all members in the group (in creation order) and:

1. For members in ``storev_ids``: calls :c:func:`Fenix_Data_member_storev` (each rank may checkpoint different element ranges)
2. For other members: calls :c:func:`Fenix_Data_member_store` (all ranks checkpoint the same element ranges)
3. Finally calls :c:func:`Fenix_Data_commit` to make checkpoint durable

**Understanding store vs storev:**

- **store**: All ranks must use the **same subset**. E.g., all ranks checkpoint elements [0-99].
- **storev**: Each rank may use **different subsets**. E.g., rank 0 checkpoints [0-50], rank 1 checkpoints [100-200].

**Usage Examples:**

.. code-block:: c

   // C example - Simple checkpoint of all members
   int group_id = 1;
   int time_stamp;

   // All members created with member_create
   int ret = Fenix_Data_checkpoint(
       group_id,
       FENIX_DATA_SUBSET_FULL,
       0,           // No storev members
       NULL,        // No storev array needed
       &time_stamp
   );

   if (ret == FENIX_SUCCESS) {
       printf("Checkpoint %d created\n", time_stamp);
   }

.. code-block:: c

   // C example - Mixed store and storev
   int group_id = 1;
   int time_stamp;

   // Members 1, 2 use uniform subsets
   // Member 3 uses varying subsets per rank

   int storev_members[] = {3};  // Member 3 needs storev

   Fenix_Data_checkpoint(
       group_id,
       FENIX_DATA_SUBSET_FULL,
       1,              // One storev member
       storev_members, // Array with member 3
       &time_stamp
   );

.. code-block:: cpp

   // C++ example - Checkpoint all members
   int group_id = 1;
   int time_stamp;

   fenix::data::checkpoint(
       group_id,
       FENIX_DATA_SUBSET_FULL,
       {},  // Empty vector = no storev members
       &time_stamp
   );

   std::cout << "Checkpoint " << time_stamp << " created\n";

.. code-block:: cpp

   // C++ example - All members use storev
   int group_id = 1;
   int time_stamp;

   // Use checkpointv variant
   fenix::data::checkpointv(
       group_id,
       FENIX_DATA_SUBSET_FULL,
       &time_stamp
   );

**When to Use:**

Use ``Fenix_Data_checkpoint`` when:

- All members should be checkpointed together
- You want a simplified API (one call instead of multiple store + commit)
- All members use the same subset specifier
- You're checkpointing the entire group periodically

Don't use when:

- Members need different subset specifiers
- You want to checkpoint members selectively
- You need fine control over store order
- Members are stored at different times

**Comparison to Manual Store + Commit:**

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Manual Workflow
     - Using Checkpoint
   * - .. code-block:: c

          Fenix_Data_member_store(
              group_id, member_1,
              FENIX_DATA_SUBSET_FULL);
          Fenix_Data_member_store(
              group_id, member_2,
              FENIX_DATA_SUBSET_FULL);
          Fenix_Data_member_store(
              group_id, member_3,
              FENIX_DATA_SUBSET_FULL);
          Fenix_Data_commit(
              group_id, &timestamp);
     - .. code-block:: c

          Fenix_Data_checkpoint(
              group_id,
              FENIX_DATA_SUBSET_FULL,
              0, NULL,
              &time_stamp);

**Common Pitfalls:**

- **Varying subsets**: If some members use uniform subsets and others use varying subsets per rank, you must specify which need storev in the storev_ids array.
- **Wrong subset**: All members are stored with the same subset. If you need different subsets, use manual store calls.
- **Ignoring timestamp**: Save the timestamp for later restoration.
- **Assuming order**: Members are stored in creation order, not necessarily member ID order.

**Performance Considerations:**

- Checkpoint is a collective operation that synchronizes all ranks
- No performance difference vs manual store+commit - it's just a convenience wrapper
- All members are stored before any are committed (two-phase protocol)

**Message Logging Support:**

This function supports inline recovery when message logging is active. If a failure
occurs during checkpoint, logged operations can be replayed automatically depending
on the :c:macro:`FENIX_MLOG_RECOVERY_MODE` setting.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_commit_barrier`, :c:func:`Fenix_Mlog_activate`
