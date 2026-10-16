subset_create
=============

.. operation:: local

Functions for creating **subsets**, which specify the element ranges of a data member to stage.

.. note::
   Created subsets must be deleted with :c:func:`Fenix_Data_subset_delete` to free memory.

.. c:function:: int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset, int stride, Fenix_Data_subset* subset_specifier)

   :param int num_blocks: The number of contiguous data blocks in the subset. Must be >= 0.
   :param int start_offset: The index (0-based) of the first element in the first data block. Must be <= end_offset.
   :param int end_offset: The index (0-based) of the last element (inclusive) in the first data block. Must be >= start_offset.
   :param int stride: Regular offset between successive data blocks. Each subsequent block starts at start_offset + i*stride. May be 0 if num_blocks <= 1.
   :param Fenix_Data_subset* subset_specifier: Pointer to store the created subset handle to.
   :returns: FENIX_SUCCESS if successful

Creates ``num_blocks`` element ranges with regular spacing:

- Block 0: elements [start_offset, end_offset]
- Block 1: elements [start_offset+stride, end_offset+stride]
- Block 2: elements [start_offset+2*stride, end_offset+2*stride]
- ...and so on

**Example:** Checkpoint every 10th element from a 1000-element array:

.. code-block:: c

   Fenix_Data_subset subset;
   Fenix_Data_subset_create(
     100,  // 100 blocks
     0,    // Start at element 0
     0,    // Single element per block
     10,   // Jump 10 elements between blocks
     &subset
   );
   // Creates subset: [0], [10], [20], [30], ..., [990]

.. c:function:: int Fenix_Data_subset_createv(int num_blocks, int* array_start_offsets, int* array_end_offsets, Fenix_Data_subset* subset_specifier)

   :param int num_blocks: The number of contiguous data blocks in the subset. Must be >= 0.
   :param int* array_start_offsets: Array of length num_blocks containing the starting index (0-based, inclusive) for each data block. Each value must be <= the corresponding end offset.
   :param int* array_end_offsets: Array of length num_blocks containing the ending index (0-based, inclusive) for each data block. Each value must be >= the corresponding start offset.
   :param Fenix_Data_subset* subset_specifier: Pointer to store the created subset handle to.
   :returns: FENIX_SUCCESS if successful

Creates ``num_blocks`` element ranges with arbitrary spacing. Each block can have a different size and location.

**Example:** Checkpoint elements 0-99, 500-599, and 800-850:

.. code-block:: c

   Fenix_Data_subset subset;
   int starts[] = {0, 500, 800};
   int ends[] = {99, 599, 850};

   Fenix_Data_subset_createv(3, starts, ends, &subset);
   // Creates subset: [0-99], [500-599], [800-850]

.. seealso::
   :c:func:`Fenix_Data_subset_delete`, :c:func:`Fenix_Data_member_stage`
