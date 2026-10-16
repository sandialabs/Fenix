group_snapshots
===============

.. operation:: local

Query the available snapshots in a data group. The C API provides functions to get the count and iterate by position.

C API
-----

.. c:function:: int Fenix_Data_group_get_number_of_snapshots(int group_id, int* number_of_snapshots)

   :param int group_id: The data group to query. Must be a valid existing group.
   :param int* number_of_snapshots: Pointer to store the count of locally-available snapshots for this group. May vary per rank if failures occurred during commit.
   :returns: FENIX_SUCCESS if successful, error code if group invalid

.. c:function:: int Fenix_Data_group_get_snapshot_at_position(int group_id, int position, int* time_stamp)

   :param int group_id: The data group to query. Must be a valid existing group.
   :param int position: The 0-based index of the snapshot to retrieve. Must be in range [0, number_of_snapshots). Position 0 is the most recent snapshot.
   :param int* time_stamp: Pointer to store the timestamp identifier for the snapshot at this position.
   :returns: FENIX_SUCCESS if successful, error code if position out of range or group invalid

C++ API
-------

.. cpp:function:: std::optional<std::vector<int>> fenix::data::group_snapshots(int group_id)

   :param int group_id: The data group to query
   :returns: std::optional containing vector of snapshot timestamps if group exists, std::nullopt otherwise. The vector's position 0 is the most recent snapshot.

Usage
-----

These functions are used to iterate over all current snapshots in a group. This is useful for:

- Finding the most recent locally-available snapshot
- Finding older snapshots if snapshot's data contains invalid data (e.g. previously undetect silent error)

.. note::
   May include snapshots that are inconsistent across the group. After a failure, some ranks may have successfully
   committed a snapshot while others did not. Use this information to determine which data can be safely restored.
   Alternatively, use commit_barrier to guarantee consistent commits.

Example
-------

This example shows how to query available snapshots and restore from a specific checkpoint:

.. code-block:: c

   int group_id = 1, member_id = 1;
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
       int ts;
       Fenix_Data_group_get_snapshot_at_position(group_id, pos, &ts);
       printf("  Position %d: timestamp %d\n", pos, ts);
   }

   // Restore from the second most recent snapshot (position 1)
   if (num_snapshots >= 2) {
       int restore_ts;
       Fenix_Data_group_get_snapshot_at_position(group_id, 1, &restore_ts);
       ret = Fenix_Data_member_restore(
           group_id, member_id, FENIX_DATA_RESTORE_INPLACE,
           FENIX_DATA_RESTORE_FULL, restore_ts, FENIX_DATA_SUBSET_IGNORE
       );
       if (ret == FENIX_SUCCESS) {
           printf("Successfully restored from timestamp %d\n", restore_timestamp);
       }
   }

.. seealso::
   :c:func:`Fenix_Data_group_get_snapshot_at_position`, :c:func:`Fenix_Data_group_get_number_of_snapshots`,
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_snapshot_delete`,
   :cpp:func:`fenix::data::group_members`
