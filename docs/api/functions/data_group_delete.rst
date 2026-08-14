group_delete
============

.. operation:: local

Delete a data group and all associated resources.

.. c:function:: int Fenix_Data_group_delete(int group_id)

.. cpp:function:: int fenix::data::group_delete(int group_id)

   :param int group_id: [in] The data group to delete. Frees all associated resources including members, snapshots, and redundant storage.
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_group_created`
