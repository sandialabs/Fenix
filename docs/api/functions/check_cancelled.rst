check_cancelled
===============

.. operation:: local

Check a pre-recovery request without error.

.. c:function:: int Fenix_check_cancelled(MPI_Request* request, MPI_Status* status)

   :param MPI_Request* request: [in] Pointer to the MPI request handle from a non-blocking MPI operation (e.g., MPI_Isend, MPI_Irecv) that may have been interrupted by a failure.
   :param MPI_Status* status: [out] Pointer to MPI_Status structure where request status information will be stored. May be MPI_STATUS_IGNORE if status not needed.
   :returns: Non-zero (true) if the request was cancelled due to failure or has unknown completion status, 0 (false) if it completed successfully

Return Codes
------------

This function uses a non-standard return convention. Rather than returning standard Fenix error codes, it returns a boolean value:

**Boolean Return Values:**

- **0 (false)**: The request completed successfully before the failure
- **Non-zero (true)**: The request was cancelled due to failure or has unknown completion status

**Error Codes:**

The function may also return standard Fenix error codes if exceptions occur during execution:

- :c:enumerator:`FENIX_ERROR_UNINITIALIZED`: Fenix has not been initialized via :c:func:`Fenix_Init`
- :c:enumerator:`FENIX_ERROR_INTERN`: An internal error occurred during execution

.. note::
   This function internally calls ``MPI_Test`` with recovery and resume modes set to ignore errors and return inline. The boolean return value indicates whether the underlying MPI test detected ``MPI_ERR_PROC_FAILED`` or ``MPI_ERR_REVOKED``, which signal that the request's completion status is unknown due to process failure.

.. seealso::
   :c:func:`Fenix_Process_detect_failures`
