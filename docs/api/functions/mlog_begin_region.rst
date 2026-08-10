mlog_begin_region
=================

.. operation:: local

Set the region of a message logger.

.. c:function:: int Fenix_Mlog_begin_region(int mlog_id, int region_id)

   :param int mlog_id: The logger to set the region of
   :param int region_id: The region ID to set (must be positive and greater than current region_id)
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::mlog::begin_region(int mlog_id, int region_id)

   :param int mlog_id: The logger to set the region of
   :param int region_id: The region ID to set
   :returns: FENIX_SUCCESS if successful

.. note::
   Region ID must be positive and greater than current region_id (may equal current region_id
   if no messages have been logged in the region).

.. seealso::
   :c:func:`Fenix_Mlog_activate_region`, :c:func:`Fenix_Mlog_sync`
