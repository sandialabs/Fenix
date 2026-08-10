member_repair
=============

.. operation:: collective

Repair the resilient storage of committed data for a member.

.. c:function:: int Fenix_Data_member_repair(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_repair(int group_id, int member_id)

   :param int group_id: The group of the member to repair
   :param int member_id: The member to repair
   :returns: FENIX_SUCCESS if successful

.. note::
   All ranks in this group must call with the same group_id and member_id. May also be matched
   by a call to :c:func:`Fenix_Data_member_restore`. Member does not have to exist locally. If
   member does not exist locally, this function is equivalent to calling :c:func:`Fenix_Data_member_create`
   with a null buffer before this function's normal behavior. If the group's policy is unable to
   rebuild this member (i.e., in the case of an unrecoverable failure pattern), raises
   FENIX_ERROR_NODATA_FOUND.

.. warning::
   Behavior is currently undefined if this group's comm has a different size than it had when
   it committed any of the group's snapshots.

.. seealso::
   :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_load`
