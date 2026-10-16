mlog_activate
=============

.. operation:: local

.. c:function:: int Fenix_Mlog_activate(int mlog_id)

.. cpp:function:: int fenix::mlog::activate(int mlog_id)

   :param int mlog_id: The log to activate. May be :c:macro:`FENIX_MLOG_NONE`.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Activate a given message log, deactivating any previously active log.

Only the active log records messages. If an error occurs while changing the
active log, no message log remains active.


.. c:function:: int Fenix_Mlog_activate_region(int mlog_id, int region_id)

.. cpp:function:: int fenix::mlog::activate(int mlog_id, int region_id)

   :param int mlog_id: The log to activate.
   :param int region_id: Region ID with the same semantics as :c:func:`Fenix_Mlog_begin_region`.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Utility function - activate a message log and begin a region.
Equivalent to calling :c:func:`Fenix_Mlog_activate` and then
:c:func:`Fenix_Mlog_begin_region` with the same arguments.

.. important::
   Inline recovery is active only when all of the following are true:

   #. :c:enumerator:`FENIX_MLOG_RECOVERY_MODE` is not
      :c:enumerator:`FENIX_MLOG_RECOVERY_MANUAL`
   #. :c:enumerator:`FENIX_RECOVERY_MODE` is not
      :c:enumerator:`FENIX_RECOVERY_IGNORE`
   #. An mlog is active

.. seealso::
   :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_active`, :c:func:`Fenix_Mlog_begin_region`
