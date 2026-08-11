snapshot_delete
===============

.. operation:: local

Delete a snapshot from a data group.

.. c:function:: int Fenix_Data_snapshot_delete(int group_id, int time_stamp)

   :param int group_id: [in] The data group containing the snapshot. Must be a valid group ID.
   :param int time_stamp: [in] The timestamp identifier of the snapshot to delete. Frees all checkpoint data for this snapshot across all members in the group.

   **Return Codes:**

   :c:enumerator:`FENIX_SUCCESS`
       Snapshot was successfully deleted.

   :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
       Fenix has not been initialized via :c:func:`Fenix_Init`.

   :c:enumerator:`FENIX_ERROR_INVALID_GROUPID`
       The specified ``group_id`` does not correspond to an existing data group.

   :c:enumerator:`FENIX_ERROR_INVALID_TIMESTAMP`
       The specified ``time_stamp`` does not exist in the group. This occurs when:

       * The timestamp was never committed in this group
       * The timestamp was already deleted
       * The timestamp is negative (invalid value)

.. cpp:function:: int fenix::data::snapshot_delete(int group_id, int timestamp)

   :param int group_id: [in] The group containing the snapshot
   :param int timestamp: [in] The timestamp of the snapshot to delete

   **Return Codes:**

   :c:enumerator:`FENIX_SUCCESS`
       Snapshot was successfully deleted.

   :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
       Fenix has not been initialized via :cpp:func:`fenix::init`.

   :c:enumerator:`FENIX_ERROR_INVALID_GROUPID`
       The specified ``group_id`` does not correspond to an existing data group.

   :c:enumerator:`FENIX_ERROR_INVALID_TIMESTAMP`
       The specified ``timestamp`` does not exist in the group. This occurs when:

       * The timestamp was never committed in this group
       * The timestamp was already deleted
       * The timestamp is negative (invalid value)

.. seealso::
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_group_get_number_of_snapshots`
