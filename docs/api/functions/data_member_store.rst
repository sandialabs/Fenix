member_store
============

.. operation:: collective

Store a data member to the checkpoint staging area.

This function copies data from the member's buffer into Fenix's checkpoint staging area.
The data is not yet durable until :c:func:`Fenix_Data_commit` is called.

.. important::
   Use this function when all ranks will store the same subset. For varying subsets
   per rank, use :c:func:`Fenix_Data_member_storev` instead.

.. c:function:: int Fenix_Data_member_store(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: [in] The data group containing the member
   :param int member_id: [in] The member to store
   :param Fenix_Data_subset subset_specifier: [in] Which portion of the data member to checkpoint. Use ``FENIX_DATA_SUBSET_FULL`` to checkpoint all elements, or create a custom subset to checkpoint only specific element ranges.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_store(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: [in] The data group containing the member
   :param int member_id: [in] The member to store
   :param Fenix_Data_subset subset_specifier: [in] Which subset of the data to store
   :returns: FENIX_SUCCESS if successful

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Data staged successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` - Failed to stage data
- :c:enumerator:`FENIX_ERROR_INVALID_SUBSET` - Invalid subset specifier

**Understanding Data Subsets:**

A **subset** specifies which elements of a data member to checkpoint. Instead of checkpointing an entire array, you can checkpoint only the portions that changed or that are critical for recovery.

.. c:macro:: FENIX_DATA_SUBSET_FULL

   Checkpoint all elements in the member. Use this for full checkpoints where the entire array should be stored.

.. c:macro:: FENIX_DATA_SUBSET_EMPTY

   Checkpoint no elements (empty checkpoint). Use as a placeholder when skipping a member during a particular checkpoint iteration.

**Custom subsets** let you checkpoint only specific element ranges:

- Create with :c:func:`Fenix_Data_subset_create` for regular patterns
- Create with :c:func:`Fenix_Data_subset_createv` for arbitrary ranges
- See :doc:`/howto/partial-checkpoints` for examples and patterns

**Usage Examples:**

.. code-block:: c

   // C example - Basic checkpointing workflow
   int group_id = 1;
   int member_id = 100;

   // Create member with fixed buffer
   static double data[1000];
   Fenix_Data_member_create(group_id, member_id, data, 1000, MPI_DOUBLE);

   // Do some computation
   for (int i = 0; i < 1000; i++) {
       data[i] = compute_value(i);
   }

   // Store the data (staged, not yet committed)
   int ret = Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to store member: %d\n", ret);
   }

   // Make checkpoint durable
   int time_stamp;
   Fenix_Data_commit(group_id, &time_stamp);
   printf("Checkpoint %d created\n", time_stamp);

.. code-block:: cpp

   // C++ example
   static std::array<double, 1000> data;

   fenix::data::member_create(group_id, member_id, data.data(), 1000, MPI_DOUBLE);

   // Compute...
   compute(data);

   // Store and commit
   fenix::data::member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   int time_stamp;
   fenix::data::commit(group_id, &time_stamp);

**Staging vs Commit:**

Fenix uses a two-phase checkpoint protocol:

1. **Store (Staging)**: Data is copied to staging area. Multiple members can be stored independently.
2. **Commit**: All staged data is made durable atomically with a timestamp.

This allows you to checkpoint multiple data members and ensure they're all from the same logical time:

.. code-block:: c

   // Stage multiple members
   Fenix_Data_member_store(group_id, member_1, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_member_store(group_id, member_2, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_member_store(group_id, member_3, FENIX_DATA_SUBSET_FULL);

   // Commit them all atomically
   int time_stamp;
   Fenix_Data_commit(group_id, &time_stamp);
   // Now all three members are checkpointed at the same timestamp

**Common Pitfalls:**

- **Forgetting to commit**: Data stored with member_store is not durable until commit is called.
- **All ranks must use same subset**: If different ranks need to store different subsets, use :c:func:`Fenix_Data_member_storev` instead.
- **Assuming immediate durability**: Store only stages data - it's not protected until commit completes.
- **Not checking return codes**: Always check for errors, especially with custom subsets.

**Performance Considerations:**

- Storing is typically fast as it only copies to local staging area
- The cost scales with the size of the data subset being stored
- Multiple stores can be pipelined before a single commit
- Consider using partial subsets for large data to reduce checkpoint overhead

.. seealso::
   :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_member_istore`, :c:func:`Fenix_Data_member_istorev`, :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_create`, :doc:`/guides/data-recovery`
