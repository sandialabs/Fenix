mlog_delete
===========

.. operation:: local

Delete a message log.

.. c:function:: int Fenix_Mlog_delete(int mlog_id)

.. cpp:function:: int fenix::mlog::mlog_delete(int mlog_id)

   :param int mlog_id: The log to delete.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Deleting a log frees its stored message-log state.

.. seealso::
   :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_activate`
