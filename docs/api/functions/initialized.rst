initialized
===========

.. operation:: local

Check if Fenix_Init has been called.

.. c:function:: int Fenix_Initialized(int* flag)

   :param int* flag: [out] Pointer to integer that will be set to 1 (true) if :c:func:`Fenix_Init` has been called, 0 (false) otherwise.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: bool fenix::initialized()

   :returns: true if :cpp:func:`fenix::init` has been called, false otherwise

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
