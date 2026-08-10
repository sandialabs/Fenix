commit_barrier
==============

.. operation:: collective

Commit stored data with globally consistent synchronization.

.. c:function:: int Fenix_Data_commit_barrier(int group_id, int* time_stamp)

.. cpp:function:: int fenix::data::commit_barrier(int group_id, int* time_stamp = nullptr)

   :param int group_id: The group to commit
   :param int* time_stamp: The timestamp assigned to this checkpoint
   :returns: FENIX_SUCCESS if successful

.. note::
   This function does not function as a traditional barrier. The commit will proceed if all
   non-failed ranks reach the barrier. This allows for commits to be made when a rank fails
   after storing all of its data into resilient storage.

.. seealso::
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_store`
