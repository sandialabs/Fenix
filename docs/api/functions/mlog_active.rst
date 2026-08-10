mlog_active
===========

.. operation:: local

Get the currently active message log.

.. c:function:: int Fenix_Mlog_active(int* mlog_id)

   :param int* mlog_id: The active log, may be FENIX_MLOG_NONE
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::mlog::active()

   :returns: The active log ID, or FENIX_MLOG_NONE

.. seealso::
   :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_create`
