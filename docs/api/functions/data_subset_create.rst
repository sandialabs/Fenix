subset_create
=============

.. operation:: local

Create a subset with regular stride pattern.

A **subset** specifies which element ranges of an array to checkpoint. This function creates subsets with regular spacing, useful for patterns like "every Nth element" or "boundary regions at regular intervals".

.. c:function:: int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset, int stride, Fenix_Data_subset* subset_specifier)

   :param int num_blocks: [in] The number of contiguous data blocks in the subset. Must be >= 0.
   :param int start_offset: [in] The index (0-based) of the first element in the first data block. Must be <= end_offset.
   :param int end_offset: [in] The index (0-based) of the last element (inclusive) in the first data block. Must be >= start_offset.
   :param int stride: [in] Regular offset between successive data blocks. Each subsequent block starts at start_offset + i*stride. May be 0 if num_blocks <= 1.
   :param Fenix_Data_subset* subset_specifier: [out] Pointer to store the created subset handle. Must be freed with :c:func:`Fenix_Data_subset_delete`.
   :returns: FENIX_SUCCESS if successful, error code if start_offset > end_offset

**How it works:**

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

**Complete Usage Example:**

This example shows how to create a subset for checkpointing boundary regions of a distributed array, then use it with the data recovery API:

.. code-block:: c

   #include <fenix.h>
   #include <stdio.h>

   int main(int argc, char** argv) {
       // Initialize Fenix
       int error_code;
       MPI_Comm fenix_comm;
       Fenix_Init(&error_code, MPI_COMM_WORLD, NULL, NULL, NULL, NULL,
                  NULL, 0, 0, &fenix_comm);

       int rank, size;
       MPI_Comm_rank(fenix_comm, &rank);
       MPI_Comm_size(fenix_comm, &size);

       // Create a data group for checkpointing
       int group_id = 1;
       int flag;
       int separation = 1;
       Fenix_Data_group_create(group_id, fenix_comm, 0, 3,
                               FENIX_DATA_POLICY_IN_MEMORY_RAID,
                               &separation, &flag);

       // Allocate local array (e.g., for domain decomposition)
       int local_size = 1000;
       double* local_data = malloc(local_size * sizeof(double));

       // Register the data member
       int member_id = 100;
       Fenix_Data_member_create(group_id, member_id, local_data,
                                local_size, MPI_DOUBLE);

       // Create subset for boundary regions (first 10 and last 10 elements)
       Fenix_Data_subset boundary_subset;
       int ret = Fenix_Data_subset_create(
           2,        // 2 blocks: start and end boundaries
           0,        // First block starts at element 0
           9,        // First block ends at element 9 (10 elements)
           990,      // Second block starts 990 elements later (at index 990)
           &boundary_subset
       );

       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Rank %d: Failed to create subset: %d\n", rank, ret);
           MPI_Abort(fenix_comm, 1);
       }

       // Simulation loop
       for (int iter = 0; iter < 100; iter++) {
           // Compute (boundary values change most frequently)
           for (int i = 0; i < local_size; i++) {
               local_data[i] = rank * 1000.0 + iter + i * 0.01;
           }

           // Checkpoint only boundary regions every 10 iterations
           if (iter % 10 == 0) {
               ret = Fenix_Data_member_store(group_id, member_id, boundary_subset);
               if (ret != FENIX_SUCCESS) {
                   fprintf(stderr, "Rank %d: Failed to store data: %d\n", rank, ret);
               } else {
                   int time_stamp;
                   Fenix_Data_commit(group_id, &time_stamp);
                   if (rank == 0) {
                       printf("Boundary checkpoint %d at iteration %d\n",
                              time_stamp, iter);
                   }
               }
           }
       }

       // Clean up subset
       Fenix_Data_subset_delete(&boundary_subset);

       // Clean up Fenix resources
       free(local_data);
       Fenix_Finalize();
       return 0;
   }

This example demonstrates:

- Creating a subset that captures two boundary regions (elements 0-9 and 990-999)
- Using the subset with :c:func:`Fenix_Data_member_store` for selective checkpointing
- Proper error checking and resource cleanup
- Integration with the full Fenix workflow (init, group creation, commit, finalize)

.. note::
   Created subsets must be deleted with :c:func:`Fenix_Data_subset_delete` to free memory.

.. seealso::
   :c:func:`Fenix_Data_subset_createv`, :c:func:`Fenix_Data_subset_delete`, :c:func:`Fenix_Data_member_store`
