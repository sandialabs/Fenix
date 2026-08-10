data_barrier
============

.. operation:: collective

Block until all ranks in the group have reached this point.

.. c:function:: int Fenix_Data_barrier(int group_id)

   :param int group_id: The data group to synchronize
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is currently unimplemented.

.. seealso::
   :c:func:`Fenix_Data_commit_barrier`
