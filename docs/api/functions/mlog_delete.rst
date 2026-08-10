mlog_delete
===========

.. operation:: local

Delete a message log and free its resources.

.. c:function:: int Fenix_Mlog_delete(int mlog_id)

.. cpp:function:: int fenix::mlog::mlog_delete(int mlog_id)

   :param int mlog_id: The mlog to delete
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_activate`
