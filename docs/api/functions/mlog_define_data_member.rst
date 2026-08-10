mlog_define_data_member
=======================

.. operation:: local

Define a data member that can be used to stage and restore a message log.

.. c:function:: int Fenix_Mlog_define_data_member(int mlog_id, int group_id, int member_id)

.. cpp:function:: int fenix::mlog::define_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: The mlog to link to this data member
   :param int group_id: The data group to create the member within
   :param int member_id: The ID to create the member as
   :returns: FENIX_SUCCESS if successful

.. note::
   See :c:func:`Fenix_Data_member_define` for semantics.

.. seealso::
   :c:func:`Fenix_Mlog_create_data_member`, :c:func:`Fenix_Data_member_define`
