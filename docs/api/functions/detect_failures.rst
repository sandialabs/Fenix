detect_failures
===============

.. operation:: collective

Explicitly check for failures and optionally recover.

This function allows applications to manually check for process failures and trigger recovery,
rather than relying on automatic detection through MPI calls.

.. c:function:: int Fenix_Process_detect_failures(int do_recovery)

   :param int do_recovery: If non-zero, perform recovery if failures are detected
   :returns: FENIX_SUCCESS if no failures, FENIX_WARNING_PARTIAL_FAIL if failures detected and recovered

.. cpp:function:: int fenix::detect_failures(bool recover = true)

   :param bool recover: If true, perform recovery if failures are detected (default: true)
   :returns: FENIX_SUCCESS if no failures, FENIX_WARNING_PARTIAL_FAIL if failures detected and recovered

.. note::
   This function is useful for applications with long delays between MPI calls, allowing them to detect
   and recover from failures more quickly.

.. code-block:: c

   // C example
   int ret = Fenix_Process_detect_failures(1);
   if (ret == FENIX_WARNING_PARTIAL_FAIL) {
       // Failures occurred and were recovered
   }

.. code-block:: cpp

   // C++ example
   int ret = fenix::detect_failures(true);
   if (ret == FENIX_WARNING_PARTIAL_FAIL) {
       // Failures occurred and were recovered
   }

.. seealso::
   :c:func:`Fenix_Init`, :doc:`/guides/process-recovery`
