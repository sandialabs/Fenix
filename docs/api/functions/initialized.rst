initialized
===========

.. operation:: local

Check if Fenix_Init has been called.

.. c:function:: int Fenix_Initialized(int* flag)

   :param int* flag: Pointer to the flag to be set
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: bool fenix::initialized()

   :returns: true if initialized, false otherwise

.. note::
   The C++ overload returns the result directly as a bool instead of through an output parameter.

.. code-block:: c

   // C example
   int flag;
   Fenix_Initialized(&flag);
   if (flag) { /* initialized */ }

.. code-block:: cpp

   // C++ example
   if (fenix::initialized()) { /* initialized */ }
