subset_delete
=============

.. operation:: local

Delete a data subset and free its memory.

.. c:function:: int Fenix_Data_subset_delete(Fenix_Data_subset* subset_specifier)

   :param Fenix_Data_subset* subset_specifier: The subset to delete
   :returns: FENIX_SUCCESS if successful

.. note::
   Frees the memory associated with a data subset object.

.. seealso::
   :c:func:`Fenix_Data_subset_create`, :c:func:`Fenix_Data_subset_createv`
