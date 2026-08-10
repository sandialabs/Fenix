mlog_create
===========

.. operation:: collective

Create a message log for a communicator.

.. c:function:: int Fenix_Mlog_create(int mlog_id, MPI_Comm* comm, int depth)

   :param int mlog_id: ID for the message log to create
   :param MPI_Comm* comm: Pointer to communicator to create log for
   :param int depth: Logging depth (number of messages to buffer)
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Mlog_activate`
