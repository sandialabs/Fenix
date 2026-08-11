member_load_to
==============

.. operation:: local

Load a member's committed data into a custom destination.

.. c:function:: int Fenix_Data_member_load_to(int group_id, int member_id, void* target, int target_count, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member whose checkpointed data to load.
   :param void* target: [out] Custom destination buffer where checkpoint data will be loaded. Must have sufficient space for target_count elements.
   :param int target_count: [in] Maximum number of elements to load into target buffer. Use FENIX_DATA_RESTORE_FULL to load all available elements (assumes buffer is sized appropriately).
   :param int time_stamp: [in] Timestamp of the snapshot to load from. Use FENIX_DATA_SNAPSHOT_ALL to load from the most recent snapshot for each element.
   :param Fenix_Data_subset* found_data: [out] Subset describing which element ranges were successfully loaded. Caller must free with :c:func:`Fenix_Data_subset_delete`. Pass FENIX_DATA_SUBSET_IGNORE if not needed.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_load(int group_id, int member_id, void* target, int target_count = FENIX_DATA_RESTORE_FULL, int time_stamp = FENIX_DATA_SNAPSHOT_ALL, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to load
   :param void* target: [out] Custom destination buffer for loading
   :param int target_count: [in] Number of elements to load. Default: FENIX_DATA_RESTORE_FULL (all available).
   :param int time_stamp: [in] Snapshot timestamp. Default: FENIX_DATA_SNAPSHOT_ALL (most recent).
   :param DataSubset data_found: [out] Element ranges loaded. Default: SUBSET_IGNORE.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. note::
   As :c:func:`Fenix_Data_member_load`, but with a custom load destination. Attempts to load
   up to target_count elements into target. If target_count is FENIX_DATA_RESTORE_FULL, assumes
   buffer has space to load all available elements.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Data loaded successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - No data found at the specified timestamp
- :c:enumerator:`FENIX_WARNING_PARTIAL_RESTORE` - Only partial data was loaded

Example
-------

Loading checkpoint data into a temporary buffer for validation:

.. code-block:: c

   #include <fenix.h>
   #include <stdio.h>
   #include <stdlib.h>

   int group_id, member_id;
   double *current_data;  // Active working data
   double *backup_buffer; // Temporary buffer for validation
   int data_size = 1000;

   // Create group and member for checkpointing
   Fenix_Data_group_create(0, 0, MPI_COMM_WORLD, 0, 0,
                           FENIX_DATA_POLICY_IN_MEMORY_RAID, &group_id);
   Fenix_Data_member_create(group_id, 0, current_data, data_size,
                           MPI_DOUBLE, &member_id);

   // Store checkpoint data
   Fenix_Data_commit(group_id, &time_stamp);

   // After failure, load checkpoint into temporary buffer for inspection
   backup_buffer = (double*)malloc(data_size * sizeof(double));
   Fenix_Data_subset *found_data;

   int ret = Fenix_Data_member_load_to(group_id, member_id, backup_buffer,
                                       data_size, FENIX_DATA_SNAPSHOT_ALL,
                                       &found_data);

   if (ret == FENIX_SUCCESS) {
       // Validate backup data before restoring to current_data
       int valid = validate_checkpoint_data(backup_buffer, data_size);
       if (valid) {
           memcpy(current_data, backup_buffer, data_size * sizeof(double));
           printf("Checkpoint data validated and restored\n");
       }
   } else if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
       printf("Warning: Only partial data recovered\n");
       // Inspect found_data subset to determine which ranges were recovered
   }

   Fenix_Data_subset_delete(&found_data);
   free(backup_buffer);

.. seealso::
   :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_restore`
