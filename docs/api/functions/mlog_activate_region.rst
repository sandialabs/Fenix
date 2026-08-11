mlog_activate_region
====================

.. operation:: local

Activate a message log and begin a region.

.. c:function:: int Fenix_Mlog_activate_region(int mlog_id, int region_id)

   :param int mlog_id: [in] The message log identifier to activate. Must be a valid log created with :c:func:`Fenix_Mlog_create`.
   :param int region_id: [in] The region ID to set for this log. Must be positive and greater than current region (unless no messages logged in current region).
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::mlog::activate(int mlog_id, int region_id)

   :param int mlog_id: [in] The logger to activate and set the region of
   :param int region_id: [in] The region ID to set
   :returns: FENIX_SUCCESS if successful

.. note::
   This helper function is equivalent to::

      Fenix_Mlog_activate(mlog_id);
      Fenix_Mlog_begin_region(mlog_id, region_id);

.. seealso::
   :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_begin_region`
