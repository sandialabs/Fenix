nspare
======

.. operation:: local

Get the number of spare ranks currently available, which decreases each time a
failure is recovered using a spare. Useful for monitoring resilience capacity.

.. c:function:: int Fenix_get_nspare()

.. cpp:function:: int fenix::nspare()

   :returns: The number of spare ranks currently available
