mlog_create_data_member
=======================

.. operation:: local


.. c:function:: int Fenix_Mlog_create_data_member(int mlog_id, int group_id, int member_id)

.. cpp:function:: int fenix::mlog::create_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: [in] The log to link to the data member.
   :param int group_id: [in] The data group to create the member within.
   :param int member_id: [in] Identifier to create the member as.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Create a data member that can be used to stage and restore a message log.

The resulting member can be staged and restored along with other Fenix data
members to preserve message log state across recovery.

.. seealso::
   :c:func:`Fenix_Mlog_define_data_member`, :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`
