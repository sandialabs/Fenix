mlog_active
===========

.. operation:: local

.. c:function:: int Fenix_Mlog_active(int* mlog_id)

   :param int* mlog_id: [out] Pointer to the active log ID, or :c:macro:`FENIX_MLOG_NONE` if no log is active.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

.. cpp:function:: int fenix::mlog::active()

   :returns: The active log ID, or :c:macro:`FENIX_MLOG_NONE` if no log is active

Get the currently active message log.

.. seealso::
   :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_create`
