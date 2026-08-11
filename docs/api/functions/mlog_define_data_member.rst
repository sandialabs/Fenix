mlog_define_data_member
=======================

.. operation:: local

Define a data member that can be used to stage and restore a message log.

.. c:function:: int Fenix_Mlog_define_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: [in] The message log identifier to associate with this data member. The member can be used to checkpoint and restore this message log's state.
   :param int group_id: [in] The data group to create the member within. Must be a valid existing group.
   :param int member_id: [in] The identifier for the member. Creates the member if it doesn't exist, or updates the association if it does (idempotent).
   :returns: FENIX_SUCCESS if successful, error code if IDs invalid

.. cpp:function:: int fenix::mlog::define_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: [in] The mlog to link to this data member
   :param int group_id: [in] The data group to create the member within
   :param int member_id: [in] The ID to create the member as (idempotent)
   :returns: FENIX_SUCCESS if successful

.. note::
   See :c:func:`Fenix_Data_member_define` for semantics.

.. seealso::
   :c:func:`Fenix_Mlog_create_data_member`, :c:func:`Fenix_Data_member_define`
