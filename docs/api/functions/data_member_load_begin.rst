member_load_begin
=================

.. operation:: local

Open a file for reading a member's committed data from.

.. c:function:: int Fenix_Data_member_load_begin(int group_id, int member_id, FILE** fpp, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member whose checkpoint to open for reading.
   :param FILE** fpp: [out] Pointer to FILE* that will be set to a read-only file pointer for accessing checkpoint data. Do not close this file manually. Must call :c:func:`Fenix_Data_member_load_end` when done.
   :param int time_stamp: [in] Timestamp of the snapshot to load. Must be a specific timestamp, not FENIX_DATA_SNAPSHOT_ALL.
   :param Fenix_Data_subset* found_data: [out] Subset describing which elements are available in the file. Data outside this subset is undefined. Pass FENIX_DATA_SUBSET_IGNORE if not needed.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_load_begin(int group_id, int member_id, FILE** fp, int time_stamp = FENIX_DATA_SNAPSHOT_LATEST, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to load
   :param FILE** fp: [out] Read-only file pointer for checkpoint data
   :param int time_stamp: [in] Snapshot timestamp. Default: FENIX_DATA_SNAPSHOT_LATEST (most recent).
   :param DataSubset data_found: [out] Available element ranges. Default: SUBSET_IGNORE.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_load_begin(int group_id, int member_id, std::iostream** strm, int time_stamp = FENIX_DATA_SNAPSHOT_LATEST, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to load
   :param std::iostream** strm: [out] Read-only stream pointer for checkpoint data
   :param int time_stamp: [in] Snapshot timestamp. Default: FENIX_DATA_SNAPSHOT_LATEST.
   :param DataSubset data_found: [out] Available element ranges. Default: SUBSET_IGNORE.
   :returns: FENIX_SUCCESS if successful

.. note::
   As :c:func:`Fenix_Data_member_load`, but opens a file to read data from instead of directly
   loading the data. It is an error to call any staging, storing, loading, or restoring function
   involving this member before a corresponding call to :c:func:`Fenix_Data_member_load_end`.
   Returned file is read-only and must not be closed by the user. The value of any data outside
   the found_data subset is undefined.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - File opened successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - No data found at the specified timestamp
- :c:enumerator:`FENIX_WARNING_PARTIAL_RESTORE` - Only partial data is available

Example
-------

**C API: Reading checkpoint data with custom deserialization**

.. code-block:: c

   int group_id = 0;
   int member_id = 0;
   FILE* fp = NULL;
   Fenix_Data_subset found_data;
   int timestamp = 5;  // Load from checkpoint at timestamp 5

   // Open the checkpoint file for reading
   int ret = Fenix_Data_member_load_begin(group_id, member_id, &fp,
                                          timestamp, &found_data);

   if (ret == FENIX_SUCCESS) {
       // Read checkpoint data with custom deserialization
       double* local_array = malloc(local_size * sizeof(double));

       // Read array metadata
       int saved_size;
       fread(&saved_size, sizeof(int), 1, fp);

       // Read array data
       fread(local_array, sizeof(double), local_size, fp);

       // Read additional metadata
       int iteration_count;
       fread(&iteration_count, sizeof(int), 1, fp);

       // Close the checkpoint file
       Fenix_Data_member_load_end(group_id, member_id);

       printf("Restored %d elements from timestamp %d\n",
              saved_size, timestamp);

   } else if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
       // Partial data available - check found_data to see what's available
       printf("Warning: Only partial data available\n");
       printf("Available elements: start=%d, end=%d\n",
              found_data.start_offset, found_data.end_offset);

       // Still proceed with reading, but be aware some data may be missing
       double* local_array = malloc(local_size * sizeof(double));
       fread(local_array, sizeof(double), local_size, fp);

       Fenix_Data_member_load_end(group_id, member_id);

   } else if (ret == FENIX_ERROR_NODATA_FOUND) {
       fprintf(stderr, "No checkpoint found at timestamp %d\n", timestamp);
       // Initialize from scratch or use different timestamp
   }

**C++ API: Stream-based checkpoint reading**

.. code-block:: cpp

   using namespace fenix;

   int group_id = 0;
   int member_id = 0;
   std::iostream* strm = nullptr;
   DataSubset found_data;

   // Open most recent checkpoint as stream
   int ret = data::member_load_begin(group_id, member_id, &strm,
                                      FENIX_DATA_SNAPSHOT_LATEST,
                                      found_data);

   if (ret == FENIX_SUCCESS) {
       // Use stream operators for convenient deserialization
       std::vector<double> data(local_size);
       int metadata;

       // Read using stream operators
       for (size_t i = 0; i < local_size; i++) {
           *strm >> data[i];
       }
       *strm >> metadata;

       // Finish loading
       data::member_load_end(group_id, member_id);

       std::cout << "Loaded " << local_size << " elements with metadata="
                 << metadata << std::endl;
   } else {
       std::cerr << "Failed to open checkpoint: error " << ret << std::endl;
   }

.. seealso::
   :c:func:`Fenix_Data_member_load_end`, :c:func:`Fenix_Data_member_load`
