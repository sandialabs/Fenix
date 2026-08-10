check_cancelled
===============

.. operation:: local

Check a pre-recovery request without error.

.. c:function:: int Fenix_check_cancelled(MPI_Request* request, MPI_Status* status)

   :param MPI_Request* request: The request to check
   :param MPI_Status* status: The status of the request
   :returns: True if the request was cancelled or has unknown completion status, false if it completed successfully

.. seealso::
   :c:func:`Fenix_Process_detect_failures`
