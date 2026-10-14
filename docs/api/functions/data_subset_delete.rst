subset_delete
=============

.. operation:: local

Delete a subset and free its memory.

Call this function to free memory allocated by :c:func:`Fenix_Data_subset_create` or :c:func:`Fenix_Data_subset_createv`. Cannot be called for predefined constants like ``FENIX_DATA_SUBSET_FULL``.

.. c:function:: int Fenix_Data_subset_delete(Fenix_Data_subset* subset_specifier)

   :param Fenix_Data_subset* subset_specifier: [in] Pointer to the subset handle to delete. Frees all memory associated with the subset. The pointer itself is not modified.
   :returns: FENIX_SUCCESS if successful

.. note::
   Passing an invalid or already-deleted subset may result in undefined behavior. Do not call this function on the predefined subsets like ``FENIX_DATA_SUBSET_FULL``, as they do not require deletion.

.. seealso::
   :c:func:`Fenix_Data_subset_create`, :c:func:`Fenix_Data_subset_createv`
