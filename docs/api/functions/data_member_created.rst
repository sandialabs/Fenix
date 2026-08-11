member_created
==============

.. operation:: local

Query if a data member exists on this rank.

.. c:function:: int Fenix_Data_member_created(int group_id, int member_id)

   :param int group_id: [in] The data group containing the member
   :param int member_id: [in] The member identifier to check
   :returns: Non-zero (true) if the member exists in the specified group on this rank, 0 (false) otherwise

.. cpp:function:: bool fenix::data::member_created(int group_id, int member_id)

   :param int group_id: [in] The data group containing the member
   :param int member_id: [in] The member identifier to check
   :returns: True if the member exists in the specified group on this rank, false otherwise

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_delete`
