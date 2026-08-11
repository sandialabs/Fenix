group_created
=============

.. operation:: local

Query if a data group exists on this rank.

.. c:function:: int Fenix_Data_group_created(int group_id)

   :param int group_id: [in] The data group identifier to check
   :returns: Non-zero (true) if the group exists on this rank, 0 (false) otherwise

.. cpp:function:: bool fenix::data::group_created(int group_id)

   :param int group_id: [in] The data group identifier to check
   :returns: True if the group exists on this rank, false otherwise

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_group_delete`
