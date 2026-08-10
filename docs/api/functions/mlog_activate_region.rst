mlog_activate_region
====================

.. operation:: local

Activate a message log and begin a region.

.. c:function:: int Fenix_Mlog_activate_region(int mlog_id, int region_id)

.. cpp:function:: int fenix::mlog::activate(int mlog_id, int region_id)

   :param int mlog_id: The logger to activate and set the region of
   :param int region_id: The region ID to set
   :returns: FENIX_SUCCESS if successful

.. note::
   This helper function is equivalent to::

      Fenix_Mlog_activate(mlog_id);
      Fenix_Mlog_begin_region(mlog_id, region_id);

.. seealso::
   :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_begin_region`
