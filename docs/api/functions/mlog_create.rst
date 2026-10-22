mlog_create
===========

.. operation:: local

.. c:function:: int Fenix_Mlog_create(int mlog_id, MPI_Comm* comm, int depth)

.. cpp:function:: int fenix::mlog::create(int mlog_id, MPI_Comm& comm, int depth)

   :param int mlog_id: Unique identifier for the message logger. Must be positive.
   :param MPI_Comm* comm:
   :param MPI_Comm& comm: Communicator whose messages will be logged. Must be repaired after failures.
   :param int depth: Number of regions to retain in the log at once
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

Create a new message logger.

.. note::
   Created message logs must be activated before logging will begin.

.. seealso::
   :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_begin_region`
