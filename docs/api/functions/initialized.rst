initialized
===========

.. operation:: local

Check if Fenix has been initialized has been called.

.. c:function:: int Fenix_Initialized(int* flag)

   :param int* flag: Pointer to an integer set to 1 if :c:func:`Fenix_Init` has been called, or 0 otherwise.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

.. cpp:function:: bool fenix::initialized()

   :returns: ``true`` if :cpp:func:`fenix::init` has been called, ``false`` otherwise
