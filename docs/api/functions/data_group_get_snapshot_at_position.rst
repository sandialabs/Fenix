group_get_snapshot_at_position
==============================

.. operation:: local

Get the time stamp of a snapshot at a given index.

.. c:function:: int Fenix_Data_group_get_snapshot_at_position(int group_id, int position, int* time_stamp)

   :param int group_id: The group to query
   :param int position: The index of the snapshot, must be [0, number_of_snapshots)
   :param int* time_stamp: The time stamp of the snapshot
   :returns: FENIX_SUCCESS if successful

.. note::
   Snapshots are indexed in reverse order in which the user committed them (e.g. the most
   recent available snapshot has position=0).

.. seealso::
   :c:func:`Fenix_Data_group_get_number_of_snapshots`
