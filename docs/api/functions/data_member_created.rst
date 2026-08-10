member_created
==============

.. operation:: local

Query if a data member exists on this rank.

.. c:function:: int Fenix_Data_member_created(int group_id, int member_id)

   :param int group_id: Group identifier
   :param int member_id: Member identifier
   :returns: A truthy value if the member exists

.. cpp:function:: bool fenix::data::member_created(int group_id, int member_id)

   :param int group_id: Group identifier
   :param int member_id: Member identifier
   :returns: True if the member exists

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_delete`
