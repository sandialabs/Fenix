member_load_end
===============

.. operation:: local

Conclude a member_load_begin operation.

.. c:function:: int Fenix_Data_member_load_end(int group_id, int member_id)

   :param int group_id: [in] The data group containing the member. Must match the group_id from the corresponding :c:func:`Fenix_Data_member_load_begin` call.
   :param int member_id: [in] The member to finish loading. Must match the member_id from the corresponding :c:func:`Fenix_Data_member_load_begin` call. Closes the file/stream opened by load_begin.
   :returns: FENIX_SUCCESS if successful, FENIX_ERROR_INVALID_LOGIC_CALL if load_begin was not called first

.. cpp:function:: int fenix::data::member_load_end(int group_id, int member_id)

   :param int group_id: [in] Group of the member to end loading for
   :param int member_id: [in] Member to end loading for
   :returns: FENIX_SUCCESS if successful

.. note::
   Throws (or returns) FENIX_ERROR_INVALID_LOGIC_CALL if there has not been a corresponding
   :c:func:`Fenix_Data_member_load_begin`.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Load operation completed successfully and file/stream closed
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized via :c:func:`Fenix_Init`
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - No data group exists with the specified group_id
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - No data member with member_id exists in the specified group
- :c:enumerator:`FENIX_ERROR_INVALID_LOGIC_CALL` - No corresponding :c:func:`Fenix_Data_member_load_begin` call was made for this member, or the load operation was already ended
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` - Internal error: the open file/stream is in staging mode instead of loading mode. This should not occur in normal usage

**Usage Examples:**

.. code-block:: c

   // C example - Reading checkpoint data incrementally
   int group_id = 1;
   int member_id = 100;
   int time_stamp = last_checkpoint;

   FILE* fp = NULL;
   Fenix_Data_subset found;
   int ret = Fenix_Data_member_load_begin(group_id, member_id, &fp,
                                          time_stamp, &found);

   if (ret == FENIX_SUCCESS) {
       // Read checkpoint data incrementally using the FILE pointer
       double buffer[100];
       size_t elements_read = fread(buffer, sizeof(double), 100, fp);

       if (elements_read > 0) {
           printf("Read %zu elements from checkpoint\n", elements_read);
           // Process the data...
       }

       // Always close the load operation
       ret = Fenix_Data_member_load_end(group_id, member_id);
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to end load operation: %d\n", ret);
       }
   }

.. code-block:: cpp

   // C++ example - Reading checkpoint with iostream
   int group_id = 1;
   int member_id = 200;

   std::iostream* strm = nullptr;
   fenix::DataSubset found;

   int ret = fenix::data::member_load_begin(
       group_id, member_id, &strm,
       FENIX_DATA_SNAPSHOT_LATEST, found
   );

   if (ret == FENIX_SUCCESS) {
       // Read structured data from checkpoint
       int count;
       strm->read(reinterpret_cast<char*>(&count), sizeof(count));

       std::vector<double> data(count);
       strm->read(reinterpret_cast<char*>(data.data()),
                  count * sizeof(double));

       std::cout << "Loaded " << count << " elements\n";

       // End the load operation
       ret = fenix::data::member_load_end(group_id, member_id);
   }

**Reading Large Checkpoints in Chunks:**

For large datasets, read incrementally rather than loading all at once:

.. code-block:: c

   FILE* fp = NULL;
   int ret = Fenix_Data_member_load_begin(group_id, member_id, &fp,
                                          timestamp, NULL);

   if (ret == FENIX_SUCCESS) {
       // Read checkpoint in chunks
       double chunk[1000];
       size_t total_read = 0;

       while (!feof(fp)) {
           size_t n = fread(chunk, sizeof(double), 1000, fp);
           if (n > 0) {
               // Process chunk
               process_data(chunk, n);
               total_read += n;
           }
       }

       printf("Total elements loaded: %zu\n", total_read);

       // Must call load_end when done
       Fenix_Data_member_load_end(group_id, member_id);
   }

**Error Handling:**

Always ensure load_end is called even if errors occur during reading:

.. code-block:: c

   FILE* fp = NULL;
   int ret = Fenix_Data_member_load_begin(group_id, member_id, &fp,
                                          timestamp, NULL);

   if (ret == FENIX_SUCCESS) {
       // Try to read data
       double data[1000];
       size_t n = fread(data, sizeof(double), 1000, fp);

       if (ferror(fp)) {
           fprintf(stderr, "Error reading checkpoint data\n");
       } else {
           printf("Successfully read %zu elements\n", n);
       }

       // Always end the load operation, even on error
       ret = Fenix_Data_member_load_end(group_id, member_id);
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to close load operation: %d\n", ret);
       }
   }

**Common Pitfalls:**

- **Forgetting to call load_end**: The file/stream remains open and prevents other operations on this member. Always call load_end when done reading.
- **Closing FILE manually**: Do not call fclose() on the FILE pointer. Fenix manages the file handle and closes it in load_end.
- **Mismatched IDs**: The group_id and member_id passed to load_end must exactly match those passed to load_begin.
- **Calling operations mid-load**: You cannot stage, store, or load this member until load_end completes.
- **Multiple load_begin calls**: Must call load_end before another load_begin on the same member.

**When to Use:**

Use the load_begin/load_end pair instead of :c:func:`Fenix_Data_member_load` when:

- Reading large checkpoints that don't fit in memory at once
- Need to parse or transform data during loading
- Want to read only specific portions of a checkpoint
- Using custom serialization formats that require streaming reads

For simple cases where you want to load all data directly into a buffer, use
:c:func:`Fenix_Data_member_load` or :c:func:`Fenix_Data_member_restore` instead.

.. seealso::
   :c:func:`Fenix_Data_member_load_begin`, :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_restore`
