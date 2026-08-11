member_restore
==============

.. operation:: collective

Restore a data member from a checkpoint to a specified buffer.

This function retrieves previously checkpointed data and writes it to the target buffer.
It can restore from any checkpoint version within the group's retention depth.

.. c:function:: int Fenix_Data_member_restore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: [in] The data group containing the member
   :param int member_id: [in] The member to restore
   :param void* target_buffer: [out] Buffer to restore data into. Must be large enough to hold the data.
   :param int max_count: [in] Maximum number of elements to restore (buffer capacity)
   :param int time_stamp: [in] Which checkpoint version to restore from. Use timestamp from :c:func:`Fenix_Data_commit`.
   :param Fenix_Data_subset* found_data: [out] Optional. Describes what subset of data was actually found and restored. Pass NULL if not needed.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_restore(int group_id, int member_id, void* target_buf, int max_count, int time_stamp)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to restore
   :param void* target_buf: [out] Buffer to restore into
   :param int max_count: [in] Maximum number of elements
   :param int time_stamp: [in] Which checkpoint version to restore
   :returns: FENIX_SUCCESS if successful

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Data restored successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - No checkpoint exists for the given timestamp
- :c:enumerator:`FENIX_ERROR_INVALID_TIMESTAMP` - Timestamp is outside retained depth
- :c:enumerator:`FENIX_ERROR_MEMBER_LOADING` - Failed to load data from checkpoint
- :c:enumerator:`FENIX_WARNING_PARTIAL_RESTORE` - Only partial data was restored

**Usage Examples:**

.. code-block:: c

   // C example - Basic restore after recovery
   void recovery_callback(MPI_Comm comm, int error, void* ctx) {
       int group_id = 1;
       int member_id = 100;
       int time_stamp = *(int*)ctx;  // Last known good timestamp

       // Restore data
       static double data[1000];
       Fenix_Data_subset found;
       int ret = Fenix_Data_member_restore(
           group_id, member_id,
           data,        // Target buffer
           1000,        // Buffer capacity
           time_stamp,  // Checkpoint to restore
           &found       // Info about what was restored
       );

       if (ret == FENIX_SUCCESS) {
           printf("Restored %d elements from timestamp %d\n",
                  found.num_blocks, time_stamp);
       } else if (ret == FENIX_ERROR_NODATA_FOUND) {
           fprintf(stderr, "No checkpoint found at timestamp %d\n", time_stamp);
           // Initialize from scratch
       } else {
           fprintf(stderr, "Restore failed: %d\n", ret);
       }
   }

   int main(int argc, char** argv) {
       int current_checkpoint = 0;

       // Register recovery callback
       Fenix_Callback_register(recovery_callback, &current_checkpoint);

       // ... computation with periodic checkpointing ...

       Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
       Fenix_Data_commit(group_id, &current_checkpoint);
   }

.. code-block:: cpp

   // C++ example with dynamic allocation
   int group_id = 1;
   int member_id = 200;
   int time_stamp = last_checkpoint;

   std::vector<double> data(1000);

   int ret = fenix::data::member_restore(
       group_id, member_id,
       data.data(),
       data.size(),
       time_stamp
   );

   if (ret == FENIX_SUCCESS) {
       std::cout << "Data restored successfully\n";
   } else if (ret == FENIX_ERROR_NODATA_FOUND) {
       std::cout << "No checkpoint found, initializing fresh\n";
       initialize_data(data);
   }

**Restoring to Different Buffer:**

Unlike the member's original buffer, you can restore to any buffer of sufficient size:

.. code-block:: c

   // Original buffer
   double original_data[1000];
   Fenix_Data_member_create(group_id, member_id, original_data, 1000, MPI_DOUBLE);
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &timestamp);

   // Restore to different buffer (e.g., after reallocation)
   double* new_data = malloc(1000 * sizeof(double));
   Fenix_Data_member_restore(group_id, member_id, new_data, 1000, timestamp, NULL);

**Handling Partial Restores:**

The ``found_data`` parameter provides information about what was actually restored,
which may be a subset if the checkpoint was partial:

.. code-block:: c

   Fenix_Data_subset found;
   int ret = Fenix_Data_member_restore(group_id, member_id,
                                       buffer, max_count, timestamp, &found);

   if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
       printf("Warning: Only partial data restored\n");
       // found describes what portion was restored
       // May need to initialize missing portions
   }

**Timestamp Management:**

Keep track of checkpoint timestamps to know which version to restore:

.. code-block:: c

   // Strategy 1: Track latest timestamp
   int latest_time_stamp = 0;
   Fenix_Data_commit(group_id, &latest_time_stamp);
   // Always restore from latest_time_stamp

   // Strategy 2: Track multiple checkpoints
   int timestamps[10];
   int num_checkpoints = 0;
   Fenix_Data_commit(group_id, &timestamps[num_checkpoints++]);
   // Can restore from any timestamp in the array

   // Strategy 3: Use a well-known value
   #define CURRENT_CHECKPOINT 42
   int time_stamp = CURRENT_CHECKPOINT;
   Fenix_Data_commit(group_id, &time_stamp);
   // All ranks know to restore from CURRENT_CHECKPOINT

**Common Pitfalls:**

- **Buffer too small**: Ensure target buffer is large enough. Check the member's count before restoring.
- **Invalid timestamp**: Only timestamps within the group's depth are available. Older timestamps are garbage collected.
- **Restoring before checkpoint exists**: On first run (no failures yet), there's no data to restore. Check for FENIX_ERROR_NODATA_FOUND and initialize normally.
- **Not checking found_data**: If you need to know whether a full or partial restore occurred, check the found_data parameter.
- **Wrong role check**: Only RECOVERED_RANK and SURVIVOR_RANK should restore. INITIAL_RANK has no failures yet.

**Restore Strategies:**

.. code-block:: c

   // Strategy 1: Restore in callback (automatic)
   void callback(MPI_Comm comm, int error, void* data) {
       if (role == FENIX_ROLE_RECOVERED_RANK) {
           // Restore state for recovered ranks
           Fenix_Data_member_restore(...);
       }
       // Survivor ranks may or may not need to restore
   }

   // Strategy 2: Restore after Init returns (manual)
   Fenix_Init(&role, ...);
   if (role != FENIX_ROLE_INITIAL_RANK) {
       // A failure occurred, restore state
       Fenix_Data_member_restore(...);
   }

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_lrestore`, :c:func:`Fenix_Data_member_repair`, :c:func:`Fenix_Callback_register`, :doc:`/guides/data-recovery`, :doc:`/tutorials/02-data-recovery`
