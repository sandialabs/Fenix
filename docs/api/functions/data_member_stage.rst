member_stage
============

.. operation:: local

Serialize a group member's data into the member's local store.

.. c:function:: int Fenix_Data_member_stage(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to serialize into local staging storage.
   :param Fenix_Data_subset subset_specifier: [in] Which element ranges to stage from the member's buffer. Use FENIX_DATA_SUBSET_FULL for all elements. Invalid: FENIX_DATA_SUBSET_PRESTAGED, or FENIX_DATA_SUBSET_ALL if member is resizable.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_stage(int group_id, int member_id, const DataSubset& subset = SUBSET_FULL)

   :param int group_id: [in] Group of the member to stage to
   :param int member_id: [in] Member to stage to
   :param DataSubset subset: [in] Element ranges to stage. Default: SUBSET_FULL (all elements).
   :returns: FENIX_SUCCESS if successful

.. note::
   A store operation can be broken into two parts: locally staging the data within Fenix, then
   policy-specific operations to make the data resilient to faults. This function performs ONLY
   the first part. Applications should subsequently make a store of this member to the
   FENIX_DATA_SUBSET_PRESTAGED data subset.

.. warning::
   FENIX_DATA_SUBSET_ALL is invalid if member size is FENIX_RESIZEABLE. FENIX_DATA_SUBSET_PRESTAGED
   is invalid. It is undefined behavior to commit staged-but-not-stored data.

**Usage Examples:**

Two-Phase Checkpointing
------------------------

The typical use case for ``member_stage`` is to separate the serialization step (staging) from the policy-specific resilience operations (storing to remote ranks or persistent storage).

.. code-block:: c

   // C example - Two-phase checkpointing for better control
   int group_id = 1;
   int member_id = 100;

   // Create member with fixed buffer
   static double simulation_state[10000];
   Fenix_Data_member_create(group_id, member_id, simulation_state, 10000, MPI_DOUBLE);

   // Update simulation state
   for (int i = 0; i < 10000; i++) {
       simulation_state[i] = compute_value(i);
   }

   // Phase 1: Stage the data locally (serialize into staging buffer)
   // This is a local operation - no communication with other ranks
   int ret = Fenix_Data_member_stage(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to stage member: %d\n", ret);
       return ret;
   }

   // At this point, data is serialized but not yet protected
   // Application can continue computation here if desired

   // Phase 2: Store the prestaged data to make it resilient
   // This operation may involve communication with buddy ranks or I/O
   ret = Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_PRESTAGED);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to store prestaged member: %d\n", ret);
       return ret;
   }

   // Make checkpoint durable
   int time_stamp;
   Fenix_Data_commit(group_id, &time_stamp);
   printf("Checkpoint %d created (two-phase)\n", time_stamp);

.. code-block:: cpp

   // C++ example
   std::array<double, 10000> state;

   fenix::data::member_create(group_id, member_id, state.data(), 10000, MPI_DOUBLE);

   // Update state
   compute(state);

   // Stage locally
   fenix::data::member_stage(group_id, member_id, FENIX_DATA_SUBSET_FULL);

   // Store prestaged data
   fenix::data::member_store(group_id, member_id, FENIX_DATA_SUBSET_PRESTAGED);

   // Commit
   int time_stamp;
   fenix::data::commit(group_id, &time_stamp);

Partial Data Staging
---------------------

Stage only a subset of data elements when only part of the data has changed:

.. code-block:: c

   // Stage only the active region of a sparse data structure
   static double sparse_data[100000];
   int active_start = 1000;
   int active_count = 5000;

   // Create subset for active region
   Fenix_Data_subset active_region;
   Fenix_Data_subset_create(active_start, active_count, 1, MPI_BYTE, &active_region);

   // Stage only the active region
   Fenix_Data_member_stage(group_id, member_id, active_region);

   // Store the prestaged subset
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_PRESTAGED);

   Fenix_Data_subset_free(&active_region);

Performance Benefits
--------------------

Separating staging from storing allows overlapping computation with checkpoint operations:

.. code-block:: c

   // Stage data quickly (local operation)
   Fenix_Data_member_stage(group_id, member_id, FENIX_DATA_SUBSET_FULL);

   // Data buffer can now be reused for computation
   // while the staged copy is stored in the background
   continue_computation(simulation_state);

   // Later, store the prestaged data (may involve communication)
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_PRESTAGED);

**Common Pitfalls:**

- **Not storing prestaged data**: After staging, you must store with ``FENIX_DATA_SUBSET_PRESTAGED`` to make the data resilient. Staged-but-not-stored data is not protected.
- **Committing without storing**: It is undefined behavior to commit staged data that has not been stored via ``member_store``.
- **Using wrong subset for store**: After staging with a subset, you must store with ``FENIX_DATA_SUBSET_PRESTAGED``, not the original subset.

**When to Use Two-Phase Checkpointing:**

- When you want to minimize the time application data is locked during checkpointing
- When you need to overlap computation with checkpoint I/O or communication
- When implementing custom checkpoint scheduling or throttling
- When profiling shows that serialization and resilience operations have different performance characteristics

**Performance Considerations:**

- Staging is typically faster than a combined store operation because it only serializes locally
- The staged data consumes additional memory until stored and committed
- Two-phase checkpointing allows better control over when expensive resilience operations occur
- Consider memory pressure when staging large datasets

**Return Codes:**

This function returns one of the following codes:

- :c:enumerator:`FENIX_SUCCESS` — Member data was successfully staged into local storage
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` — :c:func:`Fenix_Init` has not been called
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` — The specified ``group_id`` does not exist on this rank
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` — The specified ``member_id`` does not exist in the group
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` — A staging operation is already in progress for this member (e.g., from :c:func:`Fenix_Data_member_stage_begin` without corresponding :c:func:`Fenix_Data_member_stage_end`)
- :c:enumerator:`FENIX_ERROR_MEMBER_LOADING` — A loading operation is in progress for this member (e.g., from :c:func:`Fenix_data_member_load_begin` without corresponding :c:func:`Fenix_Data_member_load_end`)
- :c:enumerator:`FENIX_ERROR_INVALID_SUBSET` — The ``subset_specifier`` is ``FENIX_DATA_SUBSET_PRESTAGED`` (not valid for staging), or an unbounded subset (like ``FENIX_DATA_SUBSET_FULL``) was used on a member with size ``FENIX_RESIZEABLE`` that does not have a custom serializer function

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_stage_inplace`
