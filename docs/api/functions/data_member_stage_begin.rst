member_stage_begin
==================

.. operation:: local

Open a file for manually staging a member into.

.. c:function:: int Fenix_Data_member_stage_begin(int group_id, int member_id, FILE** fpp)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to stage. Opens a writable file for manual staging.
   :param FILE** fpp: [out] Pointer to FILE* that will be set to a writable file pointer for staging data. Do not close this file manually. Must call :c:func:`Fenix_Data_member_stage_end` when done writing.
   :returns: Return code indicating success or failure

Return Codes
^^^^^^^^^^^^

- :c:enumerator:`FENIX_SUCCESS` - Operation completed successfully. The file pointer is ready for writing.
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized. Must call ``Fenix_Init`` before using data recovery functions.
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - The specified ``group_id`` does not correspond to an existing data group. Create the group first with :c:func:`Fenix_Data_group_create`.
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - The specified ``member_id`` does not exist in the data group. Create the member first with :c:func:`Fenix_Data_member_create` or :c:func:`Fenix_Data_member_define`.
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` - A staging operation is already in progress for this member. Must call :c:func:`Fenix_Data_member_stage_end` before starting a new staging operation.
- :c:enumerator:`FENIX_ERROR_MEMBER_LOADING` - A loading operation is currently in progress for this member. Must call :c:func:`Fenix_Data_member_load_end` before staging.

.. cpp:function:: int fenix::data::member_stage_begin(int group_id, int member_id, FILE** fp)

   :param int group_id: [in] Group of the member to stage to
   :param int member_id: [in] Member to stage to
   :param FILE** fp: [out] Writable file pointer for staging
   :returns: Return code indicating success or failure
   :throws fenix::RuntimeException: If preconditions are not met (invalid group/member, uninitialized, or conflicting operation in progress)

.. cpp:function:: int fenix::data::member_stage_begin(int group_id, int member_id, std::iostream** stream)

   :param int group_id: [in] Group of the member to stage to
   :param int member_id: [in] Member to stage to
   :param std::iostream** stream: [out] Writable stream pointer for staging
   :returns: Return code indicating success or failure
   :throws fenix::RuntimeException: If preconditions are not met (invalid group/member, uninitialized, or conflicting operation in progress)

.. note::
   It is an error to call any staging, storing, loading, or restoring function involving this
   member before a corresponding call to :c:func:`Fenix_Data_member_stage_end`. File must not
   be closed by the user. It is an error to use this file after the corresponding
   :c:func:`Fenix_Data_member_stage_end`.

Example
-------

This example demonstrates using ``member_stage_begin`` to manually serialize a ``std::vector``
into Fenix's staging storage. This pattern is useful when you need fine-grained control over
serialization or when working with complex data structures.

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <vector>

   int main(int argc, char** argv) {
     // Initialize Fenix
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     // Create data group
     int errflag;
     int group_id = 0;
     Fenix_Data_group_create(
       group_id, res_comm, 0, 1,
       FENIX_DATA_POLICY_IMR, NULL, &errflag
     );

     // Create resizable member with custom serializer
     std::vector<int> data;
     int member_id = 0;

     auto data_serializer = [&data](FILE* fp, int dir, void* b, int offset, int count) {
       if (dir == FENIX_SERIALIZE) {
         // Write vector size and data to staging file
         fwrite(data.data(), sizeof(int), data.size(), fp);
       } else {
         // Read data back during restore
         data.resize(count);
         fread(data.data(), sizeof(int), count, fp);
       }
     };

     fenix::data::member_create(
       group_id, member_id, nullptr, FENIX_RESIZEABLE, MPI_INT, data_serializer
     );

     // Populate data
     data.resize(100 + rank);
     for (int& val : data) val = rank * 1000;

     // Manual staging: open file, write data, close file
     FILE* fp;
     fenix::data::member_stage_begin(group_id, member_id, &fp);

     // Write custom data format to staging file
     fwrite(data.data(), sizeof(int), data.size(), fp);

     // Finalize staging (DO NOT fclose the file pointer yourself)
     fenix::data::member_stage_end(group_id, member_id);
     fp = nullptr;  // File is now closed by stage_end

     // Store the prestaged data to make it resilient
     fenix::data::member_storev(group_id, member_id, SUBSET_PRESTAGED);
     Fenix_Data_commit_barrier(group_id, NULL);

     // Continue with application...
     Fenix_Finalize();
     return 0;
   }

**Key Points:**

- The file pointer returned by ``member_stage_begin`` is **writable** and managed by Fenix
- You **must not** call ``fclose()`` on the file pointer
- Always call ``member_stage_end`` when done writing
- After staging, store with ``FENIX_DATA_SUBSET_PRESTAGED`` to make data resilient
- This pattern gives you full control over serialization format

**C++ Stream Alternative:**

The C++ API also supports ``std::iostream`` for more idiomatic C++ code:

.. code-block:: cpp

   std::iostream* stream;
   fenix::data::member_stage_begin(group_id, member_id, &stream);

   // Write using stream operations
   stream->write(reinterpret_cast<char*>(data.data()), data.size() * sizeof(int));

   fenix::data::member_stage_end(group_id, member_id);

.. seealso::
   :c:func:`Fenix_Data_member_stage_end`, :c:func:`Fenix_Data_member_stage`
