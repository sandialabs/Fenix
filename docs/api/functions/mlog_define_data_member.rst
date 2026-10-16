mlog_define_data_member
=======================

.. operation:: local

Define a data member that can be used to stage and restore a message log.

.. c:function:: int Fenix_Mlog_define_data_member(int mlog_id, int group_id, int member_id)

.. cpp:function:: int fenix::mlog::define_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: [in] The log to link to the data member.
   :param int group_id: [in] The data group to define the member within.
   :param int member_id: [in] Identifier for the member.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

This function follows :c:func:`Fenix_Data_member_define` semantics for the
underlying data member, meaning this function can be safely called even if the
member already exists.

.. seealso::
   :c:func:`Fenix_Mlog_create_data_member`, :c:func:`Fenix_Data_member_define`
