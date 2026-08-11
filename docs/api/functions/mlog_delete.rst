mlog_delete
===========

.. operation:: local

Delete a message log and free its resources.

.. c:function:: int Fenix_Mlog_delete(int mlog_id)

   :param int mlog_id: [in] The message log identifier to delete. Frees all resources including logged messages. Must be a valid existing message log.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::mlog::mlog_delete(int mlog_id)

   :param int mlog_id: [in] The message log to delete
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_activate`
