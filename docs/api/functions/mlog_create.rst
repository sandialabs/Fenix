mlog_create
===========

.. operation:: collective

Create a message log for a communicator.

.. note::
   Message logging is an **optional** feature of Fenix. Applications can use
   process recovery and data recovery without message logging.

.. c:function:: int Fenix_Mlog_create(int mlog_id, MPI_Comm* comm, int depth)

   :param int mlog_id: [in] User-defined unique identifier for this message log. Must be unique across all message logs.
   :param MPI_Comm* comm: [in] Pointer to the MPI communicator to create message log for. Typically the Fenix resilient communicator.
   :param int depth: [in] Maximum number of messages to buffer in the log. Controls memory usage vs. recoverability. Must be >= 1.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. seealso::
   :c:func:`Fenix_Mlog_activate`
