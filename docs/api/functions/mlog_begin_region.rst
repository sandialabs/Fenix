mlog_begin_region
=================

.. operation:: local

Set the region of a message log.

.. c:function:: int Fenix_Mlog_begin_region(int mlog_id, int region_id)

.. cpp:function:: int fenix::mlog::begin_region(int mlog_id, int region_id)

   :param int mlog_id: The log whose region will be updated.
   :param int region_id: Region ID to set. 
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Regions let applications organize logged communication into phases such as
loop iterations or recovery boundaries.

.. warning::
   ``region_id`` must be positive and greater than the current region, except
   that it may equal the current region if no messages have yet been logged
   there.

.. seealso::
   :c:func:`Fenix_Mlog_activate_region`, :c:func:`Fenix_Mlog_sync`
