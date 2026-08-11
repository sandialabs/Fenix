member_load
===========

.. operation:: local

Load a member's committed data into user's data.

.. c:function:: int Fenix_Data_member_load(int group_id, int member_id, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to load. Data is loaded into the buffer specified by ATTRIBUTE_BUFFER.
   :param int time_stamp: [in] Timestamp of the snapshot to load. Use FENIX_DATA_SNAPSHOT_ALL to load from the most recent snapshot for each element.
   :param Fenix_Data_subset* found_data: [out] Subset describing which elements were successfully loaded. Caller must free with :c:func:`Fenix_Data_subset_delete` unless FENIX_DATA_SUBSET_IGNORE is passed.
   :returns: FENIX_SUCCESS if successful, FENIX_WARNING_PARTIAL_RESTORE if only partial data loaded

.. cpp:function:: int fenix::data::member_load(int group_id, int member_id, int time_stamp = FENIX_DATA_SNAPSHOT_ALL, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to load into its ATTRIBUTE_BUFFER
   :param int time_stamp: [in] Timestamp of snapshot to load. Default: FENIX_DATA_SNAPSHOT_ALL (most recent for each element).
   :param DataSubset data_found: [out] Subset successfully loaded. Default: SUBSET_IGNORE (don't report).
   :returns: FENIX_SUCCESS if successful

.. note::
   Attempts to load up to ATTRIBUTE_COUNT elements into ATTRIBUTE_BUFFER. For members without
   a serializer, data is loaded by directly copying memory. Otherwise, data is loaded by calls
   to this member's serializer.

.. note::
   If time stamp is FENIX_DATA_SNAPSHOT_ALL, attempts to load each element from the most recent
   available snapshot the individual element was committed in. User is responsible for freeing
   the subset returned in found_data, unless found_data is FENIX_DATA_SUBSET_IGNORE.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Data loaded successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - No data found at the specified timestamp
- :c:enumerator:`FENIX_WARNING_PARTIAL_RESTORE` - Only partial data was loaded

Example
-------

This example demonstrates loading checkpointed data after a failure recovery:

.. code-block:: c

   #include <fenix.h>
   #include <stdio.h>

   int main(int argc, char** argv) {
       int error;
       int recovered, role;
       double* simulation_data;
       int data_size = 1000;
       int group_id, member_id;

       // Initialize Fenix
       error = Fenix_Init(&recovered, MPI_COMM_WORLD, NULL, &role,
                          MPI_INFO_NULL, &error);

       // Allocate data buffer
       simulation_data = (double*)malloc(data_size * sizeof(double));

       // Create data group and member
       Fenix_Data_group_create(0, 0, MPI_COMM_WORLD, 0, &group_id, &error);
       Fenix_Data_member_create(group_id, 0, simulation_data, data_size,
                                FENIX_DATA_MEMBER_DOUBLE, &member_id, &error);

       if (recovered == FENIX_ROLE_RECOVERED_RANK) {
           // Rank recovered from failure - load most recent checkpoint
           Fenix_Data_subset* found_data;

           error = Fenix_Data_member_load(group_id, member_id,
                                          FENIX_DATA_SNAPSHOT_ALL,
                                          &found_data);

           if (error == FENIX_SUCCESS) {
               printf("Successfully loaded all checkpointed data\n");
           } else if (error == FENIX_WARNING_PARTIAL_RESTORE) {
               printf("Warning: Only partial data restored\n");

               // Query which elements were successfully loaded
               int num_found;
               Fenix_Data_subset_num_blocks(found_data, &num_found);
               printf("Loaded %d blocks of data\n", num_found);
           } else {
               fprintf(stderr, "Error loading data: %d\n", error);
           }

           // Clean up subset
           if (found_data != FENIX_DATA_SUBSET_IGNORE) {
               Fenix_Data_subset_delete(&found_data, &error);
           }
       } else {
           // Initialize data for the first time
           for (int i = 0; i < data_size; i++) {
               simulation_data[i] = 0.0;
           }
       }

       // Continue with computation...

       free(simulation_data);
       Fenix_Finalize();
       return 0;
   }

In C++, the same operation is simpler with automatic resource management:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <vector>
   #include <iostream>

   int main(int argc, char** argv) {
       int error;

       // Initialize Fenix with RAII wrapper
       auto fenix = fenix::initialize(&argc, &argv, MPI_COMM_WORLD, &error);

       std::vector<double> simulation_data(1000);

       // Create group and member
       int group_id = fenix::data::group_create(0, MPI_COMM_WORLD, 0);
       int member_id = fenix::data::member_create(group_id, 0,
                                                  simulation_data.data(),
                                                  simulation_data.size(),
                                                  FENIX_DATA_MEMBER_DOUBLE);

       if (fenix.get_role() == FENIX_ROLE_RECOVERED_RANK) {
           // Load most recent checkpoint
           fenix::data::DataSubset found_data;
           error = fenix::data::member_load(group_id, member_id,
                                           FENIX_DATA_SNAPSHOT_ALL,
                                           found_data);

           if (error == FENIX_SUCCESS) {
               std::cout << "Successfully loaded checkpoint\n";
           } else if (error == FENIX_WARNING_PARTIAL_RESTORE) {
               std::cout << "Warning: Partial restore\n";
           }
       } else {
           // Initialize fresh data
           std::fill(simulation_data.begin(), simulation_data.end(), 0.0);
       }

       // Continue computation...

       return 0;
   }

.. seealso::
   :c:func:`Fenix_Data_member_load_to`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_lrestore`, :c:func:`Fenix_Data_member_repair`
