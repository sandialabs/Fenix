nspare
======

.. operation:: local

Get the number of spare ranks currently available.

.. c:function:: int Fenix_get_nspare()

   :returns: The number of spare ranks available

.. cpp:function:: int fenix::nspare()

   :returns: The number of spare ranks available

.. code-block:: c

   // C example
   int nspare = Fenix_get_nspare();
   printf("Spare ranks available: %d\n", nspare);

.. code-block:: cpp

   // C++ example
   int nspare = fenix::nspare();
