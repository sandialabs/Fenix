finalized
=========

.. operation:: local

Check if Fenix has been finalized.

.. c:function:: int Fenix_Finalized(int* flag)

   :param int* flag: Pointer to integer that will be set to 1 (true) if :c:func:`Fenix_Finalize` has been called, 0 (false) otherwise.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: bool fenix::finalized()

   :returns: true if :cpp:func:`fenix::finalize` has been called, false otherwise
