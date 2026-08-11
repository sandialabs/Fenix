member_stage_end
================

.. operation:: local

Conclude a member_stage_begin operation.

.. c:function:: int Fenix_Data_member_stage_end(int group_id, int member_id)

   :param int group_id: [in] The data group containing the member. Must match the group_id from the corresponding :c:func:`Fenix_Data_member_stage_begin` call.
   :param int member_id: [in] The member to finish staging for. Must match the member_id from the corresponding :c:func:`Fenix_Data_member_stage_begin` call. Closes the file/stream and finalizes staging.
   :returns: Status code (see Return Codes below)

.. cpp:function:: int fenix::data::member_stage_end(int group_id, int member_id)

   :param int group_id: [in] Group of the member to end staging for
   :param int member_id: [in] Member to end staging for
   :returns: Status code (see Return Codes below)

.. note::
   This function is equivalent to performing a :c:func:`Fenix_Data_member_stage_inplace` with
   buf pointing to the written data and a subset of FENIX_DATA_SUBSET_FULL. For resizable members,
   the subset is instead of the range [0, staging_file_size/element_size] and it is an error if
   the staging file's size is not divisible by the element size.

.. note::
   Throws (or returns) FENIX_ERROR_INVALID_LOGIC_CALL if there has not been a corresponding
   :c:func:`Fenix_Data_member_stage_begin`.

Return Codes
------------

- :c:enumerator:`FENIX_SUCCESS` - The staging was successfully finalized.
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized. Call :c:func:`Fenix_Init` before using data recovery functions.
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - The specified ``group_id`` does not correspond to an existing data group. Create the group with :c:func:`Fenix_Data_group_create` first.
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - The specified ``member_id`` does not exist in the given data group. Create the member with :c:func:`Fenix_Data_member_create` or :c:func:`Fenix_Data_member_define` first.
- :c:enumerator:`FENIX_ERROR_INVALID_LOGIC_CALL` - No corresponding :c:func:`Fenix_Data_member_stage_begin` was called for this member, or the staging operation was already ended.
- :c:enumerator:`FENIX_ERROR_MEMBER_LOADING` - A :c:func:`Fenix_Data_member_load_begin` operation is currently active for this member instead of a staging operation. Call :c:func:`Fenix_Data_member_load_end` to close the load operation first.

.. seealso::
   :c:func:`Fenix_Data_member_stage_begin`, :c:func:`Fenix_Data_member_stage_inplace`

Example
-------

Manual staging with begin/end is useful for complex data structures or when using custom serialization:

.. code-block:: c

   #include <fenix.h>
   #include <stdio.h>

   int group_id = 0;
   int member_id = 0;
   FILE* fp;
   int retval;

   // Vector data to checkpoint
   int* data = malloc(data_size * sizeof(int));
   for (int i = 0; i < data_size; i++) {
       data[i] = compute_value(i);
   }

   // Begin staging: opens a file for writing
   retval = Fenix_Data_member_stage_begin(group_id, member_id, &fp);
   if (retval != FENIX_SUCCESS) {
       fprintf(stderr, "Error beginning staging: %d\n", retval);
       return retval;
   }

   // Write data manually to the staging file
   // This allows custom serialization or incremental writes
   fwrite(&data_size, sizeof(int), 1, fp);  // Write size first
   fwrite(data, sizeof(int), data_size, fp); // Write data

   // End staging: closes the file and finalizes the stage operation
   retval = Fenix_Data_member_stage_end(group_id, member_id);
   if (retval != FENIX_SUCCESS) {
       fprintf(stderr, "Error ending staging: %d\n", retval);
       return retval;
   }

   // Do NOT close fp manually - stage_end handles that
   // Now the staged data can be stored with member_store or member_storev
   retval = Fenix_Data_member_storev(group_id, member_id, FENIX_DATA_SUBSET_PRESTAGED);

C++ example with iostream:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <iostream>
   #include <vector>

   using fenix::data::member_stage_begin;
   using fenix::data::member_stage_end;

   int group_id = 0;
   int member_id = 0;
   std::vector<double> data(1000);

   // Populate data
   for (size_t i = 0; i < data.size(); i++) {
       data[i] = compute_value(i);
   }

   // Begin staging with C++ iostream
   std::iostream* stream;
   int retval = member_stage_begin(group_id, member_id, &stream);
   if (retval != FENIX_SUCCESS) {
       std::cerr << "Error beginning staging: " << retval << std::endl;
       return retval;
   }

   // Write data using stream operations
   size_t data_size = data.size();
   stream->write(reinterpret_cast<char*>(&data_size), sizeof(size_t));
   stream->write(reinterpret_cast<char*>(data.data()),
                 data_size * sizeof(double));

   // End staging: finalizes the operation
   retval = member_stage_end(group_id, member_id);
   if (retval != FENIX_SUCCESS) {
       std::cerr << "Error ending staging: " << retval << std::endl;
       return retval;
   }

   // Store the prestaged data
   retval = fenix::data::member_storev(group_id, member_id,
                                       FENIX_DATA_SUBSET_PRESTAGED);
