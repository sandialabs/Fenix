subset_createv
==============

.. operation:: local

Create a data subset with varying start and end offsets.

.. c:function:: int Fenix_Data_subset_createv(int num_blocks, int* array_start_offsets, int* array_end_offsets, Fenix_Data_subset* subset_specifier)

   :param int num_blocks: The number of contiguous data blocks
   :param int* array_start_offsets: The index of the first element in each data block
   :param int* array_end_offsets: The index of the last element in each data block
   :param Fenix_Data_subset* subset_specifier: The created subset
   :returns: FENIX_SUCCESS if successful

.. note::
   As :c:func:`Fenix_Data_subset_create`, but with varying start and end offsets. Creates a
   subset based on num_blocks pairs of {start_offset,end_offset}. The value of start_offset
   must be smaller than or equal to end_offset to indicate non-negative block size. Otherwise,
   the function returns an error code.

.. note::
   Created subsets must be deleted with :c:func:`Fenix_Data_subset_delete` to free memory.

.. seealso::
   :c:func:`Fenix_Data_subset_create`, :c:func:`Fenix_Data_subset_delete`
