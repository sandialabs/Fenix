group_delete
============

.. operation:: local

Delete a data group and all associated resources.

.. c:function:: int Fenix_Data_group_delete(int group_id)

   :param int group_id: [in] The data group to delete. Frees all associated resources including members, snapshots, and redundant storage. Must be a valid existing group.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::group_delete(int group_id)

   :param int group_id: [in] The data group to delete and free
   :returns: FENIX_SUCCESS if successful

Return Codes
------------

.. c:enumerator:: FENIX_SUCCESS

   The data group was successfully deleted and all associated resources were freed.

.. c:enumerator:: FENIX_ERROR_UNINITIALIZED

   :c:func:`Fenix_Init` has not been called.

.. c:enumerator:: FENIX_ERROR_INVALID_GROUPID

   The specified ``group_id`` does not exist or has already been deleted.

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_group_created`
