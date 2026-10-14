error
=====

.. operation:: local

Get the error code from the most recent process recovery.

.. c:function:: int Fenix_get_error()

.. cpp:function:: int fenix::error()

   :returns: The Fenix error code from the most recent recovery.

Utility function, returns the same code that would be set in the error parameter
of :c:func:`Fenix_Init`.
