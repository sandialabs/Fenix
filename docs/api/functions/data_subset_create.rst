subset_create
=============

.. operation:: local

Create a data subset for use in store and restore operations.

.. c:function:: int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset, int stride, Fenix_Data_subset* subset_specifier)

   :param int num_blocks: The number of contiguous data blocks
   :param int start_offset: The index of the first element in the first data block
   :param int end_offset: The index of the last element in the first data block
   :param int stride: Regular shift between successive data blocks
   :param Fenix_Data_subset* subset_specifier: The created subset
   :returns: FENIX_SUCCESS if successful

.. note::
   Creates a subset based on num_blocks pairs of {start_offset,end_offset},
   {start_offset+stride,end_offset+stride}, {start_offset+2*stride,end_offset+2*stride}, etc.
   The value of start_offset must be smaller than or equal to the value of end_offset to
   indicate non-negative block size. Otherwise, the function returns an error code.

.. note::
   Created subsets must be deleted with :c:func:`Fenix_Data_subset_delete` to free memory.

.. seealso::
   :c:func:`Fenix_Data_subset_createv`, :c:func:`Fenix_Data_subset_delete`, :c:func:`Fenix_Data_member_store`
