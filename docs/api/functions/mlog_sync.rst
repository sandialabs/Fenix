mlog_sync
=========

.. operation:: collective

Synchronize messages across ranks starting at their given regions.

.. c:function:: int Fenix_Mlog_sync(int mlog_id, int region_id)

   :param int mlog_id: The logger to sync
   :param int region_id: The region that this rank will begin at (may be FENIX_MLOG_CONTINUE)
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::mlog::sync(int mlog_id, int region_id = FENIX_MLOG_CONTINUE)

   :param int mlog_id: The logger to sync
   :param int region_id: The region to begin at
   :returns: FENIX_SUCCESS if successful

.. note::
   Ranks recovering to later states will replay messages to ranks recovering to earlier states.
   If region_id is FENIX_MLOG_CONTINUE, this rank will recover to its latest region's latest
   message state (instead of restarting the region).

.. seealso::
   :c:func:`Fenix_Mlog_begin_region`, :c:func:`Fenix_Mlog_activate`
