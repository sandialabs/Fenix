subset_delete
=============

.. operation:: local

Delete a subset and free its memory.

Call this function to free memory allocated by :c:func:`Fenix_Data_subset_create` or :c:func:`Fenix_Data_subset_createv`. Not needed for predefined constants like ``FENIX_DATA_SUBSET_FULL``.

.. c:function:: int Fenix_Data_subset_delete(Fenix_Data_subset* subset_specifier)

   :param Fenix_Data_subset* subset_specifier: [in] Pointer to the subset handle to delete. Frees all memory associated with the subset. The pointer itself is not modified. Must be a valid subset created with :c:func:`Fenix_Data_subset_create` or :c:func:`Fenix_Data_subset_createv`.

**Return Codes:**

:c:enumerator:`FENIX_SUCCESS`
   The subset was successfully deleted and its memory freed.

.. note::
   This function has minimal error checking and will attempt to free the subset's internal memory. Passing an invalid or already-deleted subset may result in undefined behavior. Do not call this function on the predefined constants ``FENIX_DATA_SUBSET_FULL``, ``FENIX_DATA_SUBSET_EMPTY``, or ``FENIX_DATA_SUBSET_PRESTAGED``, as they do not require deletion.

.. seealso::
   :c:func:`Fenix_Data_subset_create`, :c:func:`Fenix_Data_subset_createv`
