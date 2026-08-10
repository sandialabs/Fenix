Data Recovery
=============

See :doc:`/guides/data-recovery` for conceptual overview.

Data Group Management
---------------------

.. c:function:: int Fenix_Data_group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

   Create a data group for managing related data members.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param comm: [in] Communicator for this group
   :param start_time_stamp: [in] Starting timestamp
   :param depth: [in] Checkpoint depth (number of checkpoints to retain)
   :param policy_name: [in] Redundancy policy to use (e.g. FENIX_DATA_POLICY_IN_MEMORY_RAID)
   :param policy_value: [in] Policy-specific configuration
   :param flag: [out] Result flag

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

   .. seealso::
      - :doc:`/guides/imr-policy`

.. c:function:: int Fenix_Data_group_created(int group_id)

   Query if a data group exists on this rank.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier

   :returns: A truthy value if the group exists

.. c:function:: int Fenix_Data_group_get_redundancy_policy(int group_id, int* policy_name, void* policy_value, int* flag)

   Get the redundancy policy of a data group.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param policy_name: [out] Policy name
   :param policy_value: [out] Policy configuration
   :param flag: [out] Result flag

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_group_get_number_of_members(int group_id, int* number_of_members)

   Get the number of members in a data group.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param number_of_members: [out] Number of members

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_group_get_member_at_position(int group_id, int position, int* member_id)

   Get the member ID at a specific position in the group.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param position: [in] Position index
   :param member_id: [out] Member ID at that position

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_group_get_number_of_snapshots(int group_id, int* number_of_snapshots)

   Get the number of snapshots stored for a group.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param number_of_snapshots: [out] Number of snapshots

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_group_get_snapshot_at_position(int group_id, int position, int* time_stamp)

   Get the timestamp of a snapshot at a specific position.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param position: [in] Position index
   :param time_stamp: [out] Timestamp of snapshot

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_group_delete(int group_id)

   Delete a data group and free its resources.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Data Member Management
----------------------

.. c:function:: int Fenix_Data_member_create(int group_id, int member_id, void* source_buffer, int count, MPI_Datatype datatype)

   Create a data member for store/restore operations.

   .. rubric:: Collective Operation

   All calling ranks in the group's communicator must pass the same values for
   member_id, datatype, group_id, and count.

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier (unique within group)
   :param source_buffer: [in] Pointer to application data
   :param count: [in] Number of elements
   :param datatype: [in] MPI datatype

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_fcreate(int group_id, int member_id, Fenix_Serialize_file_fn serialize_fn, void* serialize_param, FILE* file)

   Create a data member with custom serialization.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param serialize_fn: [in] Custom serialization function
   :param serialize_param: [in] Parameter for serialization function
   :param file: [in] File handle for serialization

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_define(int group_id, int member_id, void* source_buffer, int count, MPI_Datatype datatype)

   Define a data member without creating local storage.

   .. rubric:: Collective Operation

   Similar to :c:func:`Fenix_Data_member_create` but defers storage allocation.

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param source_buffer: [in] Pointer to application data
   :param count: [in] Number of elements
   :param datatype: [in] MPI datatype

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_fdefine(int group_id, int member_id, Fenix_Serialize_file_fn serialize_fn, void* serialize_param, FILE* file)

   Define a data member with custom serialization, without creating storage.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param serialize_fn: [in] Custom serialization function
   :param serialize_param: [in] Parameter for serialization function
   :param file: [in] File handle for serialization

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_created(int group_id, int member_id)

   Query if a data member exists.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier

   :returns: A truthy value if the member exists

.. c:function:: int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename, void* attributevalue, int* flag)

   Get an attribute of a data member.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param attributename: [in] Attribute name
   :param attributevalue: [out] Attribute value
   :param flag: [out] Result flag

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_attr_set(int group_id, int member_id, int attributename, void* attributevalue, int* flag)

   Set an attribute of a data member.

   .. rubric:: Local Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param attributename: [in] Attribute name
   :param attributevalue: [in] Attribute value
   :param flag: [out] Result flag

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Staging Operations
------------------

.. c:function:: int Fenix_Data_member_stage(int group_id, int member_id, const Fenix_Data_subset subset)

   Stage data for checkpointing.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param subset: [in] Data subset to stage

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_stage_inplace(int group_id, int member_id, void* source_buffer, const Fenix_Data_subset subset)

   Stage data in-place for checkpointing.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param source_buffer: [in] Buffer to stage from
   :param subset: [in] Data subset to stage

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_stage_begin(int group_id, int member_id, FILE** fpp)

   Begin staging with custom serialization.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param fpp: [out] File pointer for staging

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_stage_end(int group_id, int member_id)

   End staging with custom serialization.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Store Operations
----------------

.. c:function:: int Fenix_Data_member_store(int group_id, int member_id, const Fenix_Data_subset subset)

   Store data member to redundant storage.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param subset: [in] Data subset to store

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_storev(int group_id, int memberv_count, int* memberv_id, Fenix_Data_subset* subsetv)

   Store multiple data members in one operation.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param memberv_count: [in] Number of members to store
   :param memberv_id: [in] Array of member IDs
   :param subsetv: [in] Array of subsets

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_istore(int group_id, int member_id, const Fenix_Data_subset subset, Fenix_Request* request)

   Non-blocking store of data member.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param subset: [in] Data subset to store
   :param request: [out] Request handle

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_istorev(int group_id, int memberv_count, int* memberv_id, Fenix_Data_subset* subsetv, Fenix_Request* request)

   Non-blocking store of multiple data members.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param memberv_count: [in] Number of members to store
   :param memberv_id: [in] Array of member IDs
   :param subsetv: [in] Array of subsets
   :param request: [out] Request handle

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Commit Operations
-----------------

.. c:function:: int Fenix_Data_commit(int group_id, int* time_stamp)

   Commit staged/stored data and create a snapshot.

   .. rubric:: Collective Operation

   Finalizes all preceding store operations and assigns a timestamp.

   :param group_id: [in] Group identifier
   :param time_stamp: [out] Timestamp of created snapshot

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_commit_barrier(int group_id, int* time_stamp)

   Commit with barrier synchronization.

   .. rubric:: Collective Operation

   Like :c:func:`Fenix_Data_commit` but includes a barrier.

   :param group_id: [in] Group identifier
   :param time_stamp: [out] Timestamp of created snapshot

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_checkpoint(int group_id, int depth, int* time_stamp)

   Combined stage, store, and commit operation.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param depth: [in] Checkpoint depth
   :param time_stamp: [out] Timestamp of created snapshot

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_barrier(int group_id)

   Barrier synchronization for a data group.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Restore Operations
------------------

.. c:function:: int Fenix_Data_member_repair(int group_id, int member_id)

   Repair a data member after failure.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_load(int group_id, int member_id, int time_stamp, const Fenix_Data_subset subset)

   Load data from redundant storage (deprecated, use restore).

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param time_stamp: [in] Snapshot timestamp to restore from
   :param subset: [in] Data subset to load

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_load_to(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, const Fenix_Data_subset subset)

   Load data to a specific buffer.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param target_buffer: [in] Buffer to load into
   :param max_count: [in] Maximum elements to load
   :param time_stamp: [in] Snapshot timestamp
   :param subset: [in] Data subset to load

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_load_end(int group_id, int member_id)

   End custom deserialization load.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_restore(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, const Fenix_Data_subset subset)

   Restore data member from a snapshot.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param target_buffer: [in] Buffer to restore into
   :param max_count: [in] Maximum elements to restore
   :param time_stamp: [in] Snapshot timestamp
   :param subset: [in] Data subset to restore

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_lrestore(int group_id, int member_id, void* target_buffer, int max_count, const Fenix_Data_subset subset)

   Restore data member from latest snapshot.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param target_buffer: [in] Buffer to restore into
   :param max_count: [in] Maximum elements to restore
   :param subset: [in] Data subset to restore

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_member_restore_from_rank(int group_id, int member_id, void* target_buffer, int max_count, int time_stamp, int source_rank)

   Restore data from a specific rank.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param member_id: [in] Member identifier
   :param target_buffer: [in] Buffer to restore into
   :param max_count: [in] Maximum elements to restore
   :param time_stamp: [in] Snapshot timestamp
   :param source_rank: [in] Rank to restore from

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Subset Operations
-----------------

.. c:function:: int Fenix_Data_subset_create(int num_blocks, int start_offset, int end_offset, int stride, Fenix_Data_subset* subset_specifier)

   Create a data subset specification.

   .. rubric:: Local Operation

   :param num_blocks: [in] Number of blocks
   :param start_offset: [in] Starting offset
   :param end_offset: [in] Ending offset
   :param stride: [in] Stride between blocks
   :param subset_specifier: [out] Created subset

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_subset_createv(int num_blocks, int* array_start_offsets, int* array_end_offsets, Fenix_Data_subset* subset_specifier)

   Create a data subset from arrays of offsets.

   .. rubric:: Local Operation

   :param num_blocks: [in] Number of blocks
   :param array_start_offsets: [in] Array of start offsets
   :param array_end_offsets: [in] Array of end offsets
   :param subset_specifier: [out] Created subset

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_subset_delete(Fenix_Data_subset* subset_specifier)

   Delete a data subset specification.

   .. rubric:: Local Operation

   :param subset_specifier: [in] Subset to delete

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Asynchronous Operations
------------------------

.. c:function:: int Fenix_Data_wait(Fenix_Request request)

   Wait for a non-blocking data operation to complete.

   .. rubric:: Local Operation

   :param request: [in] Request handle

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Data_test(Fenix_Request request, int* flag)

   Test if a non-blocking data operation has completed.

   .. rubric:: Local Operation

   :param request: [in] Request handle
   :param flag: [out] Completion flag

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Snapshot Management
-------------------

.. c:function:: int Fenix_Data_snapshot_delete(int group_id, int time_stamp)

   Delete a specific snapshot.

   .. rubric:: Collective Operation

   :param group_id: [in] Group identifier
   :param time_stamp: [in] Timestamp of snapshot to delete

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise
