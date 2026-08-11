nspare
======

.. operation:: local

Get the number of spare ranks currently available.

.. c:function:: int Fenix_get_nspare()

   Get the current count of unused spare ranks available to replace future failures. Decreases each time a failure is recovered using a spare. Useful for monitoring resilience capacity.

   :returns: The number of spare ranks currently available (not yet used for recovery)

.. cpp:function:: int fenix::nspare()

   :returns: The number of spare ranks currently available

Behavior
--------

This function directly returns the spare rank count and does not use Fenix return codes. It must be called after :c:func:`Fenix_Init` has completed; calling it beforehand will trigger an assertion failure and abort the program.

The returned value represents the number of spare ranks that have not yet been used to replace failed ranks. This count decreases each time a failure is recovered using process recovery with spare ranks, and never increases during program execution.

Return Value
------------

Non-negative integer
   The number of spare ranks currently available (0 or greater)

.. note::
   Unlike most Fenix functions, this function does not return a :c:type:`Fenix_Return_codes` value. If Fenix is not initialized, the program will abort via assertion failure rather than returning an error code.

.. code-block:: c

   // C example
   int nspare = Fenix_get_nspare();
   printf("Spare ranks available: %d\n", nspare);

.. code-block:: cpp

   // C++ example
   int nspare = fenix::nspare();
