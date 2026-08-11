subset_createv
==============

.. operation:: local

Create a subset with arbitrary element ranges.

A **subset** specifies which element ranges of an array to checkpoint. This function creates subsets with arbitrary (non-regular) spacing, useful for checkpointing only specific regions or elements that changed.

.. c:function:: int Fenix_Data_subset_createv(int num_blocks, int* array_start_offsets, int* array_end_offsets, Fenix_Data_subset* subset_specifier)

   :param int num_blocks: [in] The number of contiguous data blocks in the subset. Must be >= 0.
   :param int* array_start_offsets: [in] Array of length num_blocks containing the starting index (0-based, inclusive) for each data block. Each value must be <= the corresponding end offset.
   :param int* array_end_offsets: [in] Array of length num_blocks containing the ending index (0-based, inclusive) for each data block. Each value must be >= the corresponding start offset.
   :param Fenix_Data_subset* subset_specifier: [out] Pointer to store the created subset handle. Must be freed with :c:func:`Fenix_Data_subset_delete`.
   :returns: FENIX_SUCCESS if successful, error code if any start_offset > end_offset

**How it works:**

Creates ``num_blocks`` element ranges with arbitrary spacing. Each block can have a different size and location.

**Example:** Checkpoint elements 0-99, 500-599, and 800-850:

.. code-block:: c

   Fenix_Data_subset subset;
   int starts[] = {0, 500, 800};
   int ends[] = {99, 599, 850};

   Fenix_Data_subset_createv(3, starts, ends, &subset);
   // Creates subset: [0-99], [500-599], [800-850]

**Comparison to subset_create:**

- Use :c:func:`Fenix_Data_subset_create` for regular patterns (e.g., every Nth element)
- Use ``Fenix_Data_subset_createv`` for arbitrary ranges (e.g., specific regions that changed)

**Practical Usage Example:**

A typical use case is checkpointing only modified regions of a large array. This example shows a stencil computation that tracks which blocks were updated:

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   int main(int argc, char** argv) {
       int ierr;
       Fenix_Data_group group;
       int member_id;
       double* simulation_data;
       int data_size = 10000;

       // Initialize Fenix
       Fenix_Init(&ierr, MPI_COMM_WORLD, NULL, NULL, NULL, NULL, 0, 0, 0, 0);

       // Create data group and register data member
       Fenix_Data_group_create(0, 0, 0, 0, &group);
       simulation_data = malloc(data_size * sizeof(double));
       Fenix_Data_member_create(group, 0, simulation_data, data_size, MPI_DOUBLE, &member_id);

       // Simulation: only blocks 10-49, 200-299, and 500-599 were modified
       int num_modified_blocks = 3;
       int modified_starts[] = {10, 200, 500};
       int modified_ends[] = {49, 299, 599};

       // Create subset for only the modified regions
       Fenix_Data_subset modified_subset;
       ierr = Fenix_Data_subset_createv(num_modified_blocks,
                                        modified_starts,
                                        modified_ends,
                                        &modified_subset);
       if (ierr != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to create subset\n");
           MPI_Abort(MPI_COMM_WORLD, 1);
       }

       // Checkpoint only the modified regions (much faster than full checkpoint)
       ierr = Fenix_Data_member_store(group, member_id, modified_subset);
       if (ierr != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to store data subset\n");
       }

       // Commit the checkpoint
       Fenix_Data_commit(group);

       // Clean up
       Fenix_Data_subset_delete(&modified_subset);
       free(simulation_data);
       Fenix_Finalize();
       return 0;
   }

This pattern is efficient for large datasets where only a small fraction changes between checkpoints. Instead of checkpointing all 10,000 elements, only 440 modified elements are stored.

.. note::
   Created subsets must be deleted with :c:func:`Fenix_Data_subset_delete` to free memory.

.. seealso::
   :c:func:`Fenix_Data_subset_create`, :c:func:`Fenix_Data_subset_delete`, :c:func:`Fenix_Data_member_store`
