commit
======

.. operation:: collective

Commit all staged data to make the checkpoint durable and consistent.

This function finalizes the checkpoint by making all previously staged data (from
:c:func:`Fenix_Data_member_store` calls) durable. It assigns a timestamp to the
checkpoint version and applies the group's redundancy policy to protect the data.

.. c:function:: int Fenix_Data_commit(int group_id, int* time_stamp)

.. cpp:function:: int fenix::data::commit(int group_id, int* time_stamp)

   :param int group_id: [in] The data group to commit
   :param int* time_stamp: [out] The timestamp assigned to this checkpoint version. Use this timestamp for later restore operations.
   :returns: FENIX_SUCCESS if successful, error code otherwise

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Checkpoint committed successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_COMMIT_BARRIER` - Failed during commit synchronization

**Usage Examples:**

.. code-block:: c

   // C example - Complete checkpoint workflow
   int group_id = 1;
   int time_stamp;

   // Stage data for multiple members
   Fenix_Data_member_store(group_id, member_1, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_member_store(group_id, member_2, FENIX_DATA_SUBSET_FULL);

   // Commit to make checkpoint durable
   int ret = Fenix_Data_commit(group_id, &time_stamp);
   if (ret == FENIX_SUCCESS) {
       printf("Created checkpoint with timestamp %d\n", time_stamp);
   } else {
       fprintf(stderr, "Commit failed: %d\n", ret);
   }

   // Save timestamp for later recovery
   current_checkpoint = time_stamp;

.. code-block:: cpp

   // C++ example with error handling
   int group_id = 1;
   int time_stamp;

   try {
       // Stage data
       fenix::data::member_store(group_id, member_1, FENIX_DATA_SUBSET_FULL);
       fenix::data::member_store(group_id, member_2, FENIX_DATA_SUBSET_FULL);

       // Commit
       if (fenix::data::commit(group_id, &time_stamp) == FENIX_SUCCESS) {
           std::cout << "Checkpoint " << time_stamp << " created\n";
       }
   } catch (const fenix::CommException& e) {
       std::cerr << "Checkpoint failed: " << e.what() << "\n";
   }

**Checkpoint Versioning:**

Commits create versioned checkpoints identified by timestamps. The group's ``depth``
parameter (from :c:func:`Fenix_Data_group_create`) determines how many versions are kept:

.. code-block:: c

   // Create group with depth=3 (keep 3 versions)
   Fenix_Data_group_create(group_id, comm, 0, 3, policy, policy_val, &flag);

   // Create checkpoints
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts1);  // ts1 = 0

   // ... work ...

   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts2);  // ts2 = 1

   // ... work ...

   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts3);  // ts3 = 2

   // ... work ...

   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts4);  // ts4 = 3, ts1 is deleted

   // Can restore from ts2, ts3, or ts4, but not ts1 (too old)

**Atomicity Guarantees:**

Commit provides atomic checkpoint semantics:

- Either all staged data is committed successfully, or none is
- All ranks see the same timestamp for a given commit
- After successful commit, data can be restored even if failures occur
- Partial commits are not visible - checkpoint is all-or-nothing

**Performance Considerations:**

- Commit is a collective operation that synchronizes all ranks
- Performance depends on the redundancy policy (IMR requires data redistribution)
- Commit frequency should balance protection vs. overhead
- Consider using :c:func:`Fenix_Data_checkpoint` to combine store+commit in one call

**Common Pitfalls:**

- **Committing without staging**: Calling commit without prior store operations is wasteful but not an error.
- **Not saving timestamp**: You need the timestamp to restore data after recovery. Save it in a well-known location or track it in application logic.
- **Too frequent commits**: Each commit has overhead. Checkpoint at logical intervals (e.g., iteration boundaries) not every line.
- **Ignoring errors**: Commit can fail due to memory pressure or communication errors. Always check the return code.

**Alternative: Combined Checkpoint:**

For simple cases where you want to store and commit in one step, use :c:func:`Fenix_Data_checkpoint`:

.. code-block:: c

   // Instead of:
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &time_stamp);

   // You can use:
   Fenix_Data_checkpoint(group_id, FENIX_DATA_SUBSET_FULL, 0, NULL, &time_stamp);

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_checkpoint`, :c:func:`Fenix_Data_commit_barrier`, :c:func:`Fenix_Data_group_create`, :doc:`/guides/data-recovery`
