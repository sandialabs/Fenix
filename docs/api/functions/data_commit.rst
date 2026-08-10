commit
======

.. operation:: collective

Commit all stored data to make the checkpoint durable.

.. c:function:: int Fenix_Data_commit(int group_id, int* time_stamp)

.. cpp:function:: int fenix::data::commit(int group_id, int* time_stamp)

   :param int group_id: The group to commit
   :param int* time_stamp: The timestamp assigned to this checkpoint
   :returns: FENIX_SUCCESS if successful

.. note::
   Data must be stored via :c:func:`Fenix_Data_member_store` before committing.

.. seealso::
   :c:func:`Fenix_Data_member_store`
