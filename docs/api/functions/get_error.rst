error
=====

.. operation:: local

Get the error code from the most recent failure.

.. c:function:: int Fenix_get_error()

   :returns: The error code from the most recent failure

.. cpp:function:: int fenix::error()

   :returns: The error code from the most recent failure

.. code-block:: c

   // C example
   int error = Fenix_get_error();
   if (error != FENIX_SUCCESS) {
       // Handle error
   }

.. code-block:: cpp

   // C++ example
   int error = fenix::error();
