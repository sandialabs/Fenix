group_created
=============

.. operation:: local

Query if a data group exists on this rank.

.. c:function:: int Fenix_Data_group_created(int group_id)

   :param int group_id: Group identifier
   :returns: A truthy value if the group exists

.. cpp:function:: bool fenix::data::group_created(int group_id)

   :param int group_id: Group identifier
   :returns: True if the group exists

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_group_delete`
