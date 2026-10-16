mlog_sync
=========

.. operation:: collective

Synchronize messages across ranks, each starting at its given region.

.. c:function:: int Fenix_Mlog_sync(int mlog_id, int region_id)

.. cpp:function:: int fenix::mlog::sync(int mlog_id, int region_id = FENIX_MLOG_CONTINUE)

   :param int mlog_id: The log to synchronize.
   :param int region_id: Region this rank will begin at. It may be :c:macro:`FENIX_MLOG_CONTINUE`, in which case the rank recovers to its latest region's latest message state instead of restarting that region.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Ranks recovering to later states replay messages to ranks recovering to
earlier states.

.. seealso::
   :c:func:`Fenix_Mlog_begin_region`, :c:func:`Fenix_Mlog_activate`
