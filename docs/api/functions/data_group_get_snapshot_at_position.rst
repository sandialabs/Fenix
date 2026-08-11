group_get_snapshot_at_position
==============================

.. operation:: local

Get the time stamp of a snapshot at a given index.

.. c:function:: int Fenix_Data_group_get_snapshot_at_position(int group_id, int position, int* time_stamp)

   :param int group_id: [in] The data group to query. Must be a valid existing group.
   :param int position: [in] The 0-based index of the snapshot to retrieve. Must be in range [0, number_of_snapshots). Position 0 is the most recent snapshot.
   :param int* time_stamp: [out] Pointer to store the timestamp identifier for the snapshot at this position.
   :returns: FENIX_SUCCESS if successful, error code if position out of range or group invalid

.. note::
   Snapshots are indexed in reverse order in which the user committed them (e.g. the most
   recent available snapshot has position=0).

Example
-------

This example shows how to query available snapshots and restore from a specific checkpoint:

.. code-block:: c

   int group_id = 1;
   int num_snapshots;
   int ret;

   // Query how many snapshots are available
   ret = Fenix_Data_group_get_number_of_snapshots(group_id, &num_snapshots);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to get snapshot count\n");
       return ret;
   }

   printf("Found %d available snapshots\n", num_snapshots);

   // Iterate through snapshots (position 0 is most recent)
   for (int pos = 0; pos < num_snapshots; pos++) {
       int time_stamp;
       ret = Fenix_Data_group_get_snapshot_at_position(group_id, pos, &time_stamp);
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to get snapshot at position %d\n", pos);
           continue;
       }
       printf("  Position %d: timestamp %d\n", pos, time_stamp);
   }

   // Restore from the second most recent snapshot (position 1)
   if (num_snapshots >= 2) {
       int restore_timestamp;
       ret = Fenix_Data_group_get_snapshot_at_position(group_id, 1, &restore_timestamp);
       if (ret == FENIX_SUCCESS) {
           ret = Fenix_Data_member_restore(member_id, restore_timestamp);
           if (ret == FENIX_SUCCESS) {
               printf("Successfully restored from timestamp %d\n", restore_timestamp);
           }
       }
   }

.. seealso::
   :c:func:`Fenix_Data_group_get_number_of_snapshots`
