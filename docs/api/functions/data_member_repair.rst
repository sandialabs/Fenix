member_repair
=============

.. operation:: collective

Repair the resilient storage of committed data for a member.

.. c:function:: int Fenix_Data_member_repair(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_repair(int group_id, int member_id)

   :param int group_id: The data group containing the member. All ranks in the group must provide the same value.
   :param int member_id: The member whose redundant storage to repair/reconstruct after failures. All ranks must provide the same value. Member does not need to exist locally (will be created with NULL buffer if missing).
   :returns: FENIX_SUCCESS if successful

The member does not need to exist locally prior to this call. If the member is
created by this call, it will be created without a serializer and with a NULL
buffer (see :cpp:func:`member_define`).

This function can be matched remotely by a call to :cpp:func:`member_restore` on
the same member.

This function does not load any data into the application's memory. See
:cpp:func:`member_load` or :cpp:func:`member_restore`.

.. warning::
   Behavior is currently undefined if this group's comm has a different size than it had when
   it committed any of the group's snapshots.

.. seealso::
   :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_load`
