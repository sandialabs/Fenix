mlog_sync
=========

.. operation:: collective

Synchronize messages across ranks starting at their given regions.

.. c:function:: int Fenix_Mlog_sync(int mlog_id, int region_id)

   :param int mlog_id: [in] The message log identifier to synchronize across all ranks. Must be a valid existing log.
   :param int region_id: [in] The region that this rank will begin at after synchronization. Use FENIX_MLOG_CONTINUE to recover to this rank's latest logged message state within the current region. Ranks with later states will replay messages to ranks recovering to earlier states.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::mlog::sync(int mlog_id, int region_id = FENIX_MLOG_CONTINUE)

   :param int mlog_id: [in] The logger to sync across ranks
   :param int region_id: [in] Region to begin at. Default: FENIX_MLOG_CONTINUE (latest state).
   :returns: FENIX_SUCCESS if successful

.. note::
   Ranks recovering to later states will replay messages to ranks recovering to earlier states.
   If region_id is FENIX_MLOG_CONTINUE, this rank will recover to its latest region's latest
   message state (instead of restarting the region).

Return Codes
------------

- :c:enumerator:`FENIX_SUCCESS` - The operation completed successfully and the message log has been synchronized across all ranks.
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized. :c:func:`Fenix_Init` must be called before this function.
- :c:enumerator:`FENIX_ERROR_INVALID_MLOGID` - The specified ``mlog_id`` does not exist. The message log must be created with :c:func:`Fenix_Mlog_create` before it can be synchronized.

.. seealso::
   :c:func:`Fenix_Mlog_begin_region`, :c:func:`Fenix_Mlog_activate`
