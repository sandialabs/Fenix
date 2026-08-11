Data Recovery Component
=======================

Functions for managing data checkpointing and recovery.

Data recovery in Fenix provides high-performance in-memory checkpoint/restart capabilities.
Applications can create data groups, define data members, and store/restore application state
to recover from process failures.

.. toctree::
   :titlesonly:
   :hidden:
   :maxdepth: 1

   data-recovery/group-management
   data-recovery/member-management
   data-recovery/staging
   data-recovery/checkpointing
   data-recovery/recovery
   data-recovery/data-subsets

Quick Reference
---------------

**Typical Workflow:**

1. :c:func:`Fenix_Data_group_create` - Create a group with redundancy policy
2. :c:func:`Fenix_Data_member_create` or :c:func:`Fenix_Data_member_define` - Add data members
3. :c:func:`Fenix_Data_member_store` or :c:func:`Fenix_Data_member_storev` - Stage data
4. :c:func:`Fenix_Data_commit` - Make checkpoint durable
5. :c:func:`Fenix_Data_member_restore` - Restore after recovery

**Key Function Comparisons:**

member_create vs member_define
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Feature
     - :c:func:`Fenix_Data_member_create`
     - :c:func:`Fenix_Data_member_define`
   * - Buffer pointer
     - Saved at creation
     - Saved at creation/update
   * - Idempotent
     - No (fails if exists)
     - Yes (updates buffer/count/datatype if exists)
   * - Store functions
     - Both :c:func:`Fenix_Data_member_store` and :c:func:`Fenix_Data_member_storev`
     - Both :c:func:`Fenix_Data_member_store` and :c:func:`Fenix_Data_member_storev`
   * - Best for
     - Static/fixed buffers that never move
     - Buffers that may move (realloc, vector resize)
   * - Example
     - ``static double data[1000]``
     - ``std::vector<double> data`` (call again after resize)

store vs storev
^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Function
     - When to Use
     - Key Feature
   * - :c:func:`Fenix_Data_member_store`
     - All ranks store same subset
     - Uniform subsets across ranks
   * - :c:func:`Fenix_Data_member_storev`
     - Ranks store different subsets
     - Subsets may vary rank-to-rank

Blocking vs Non-blocking
^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Operation
     - Blocking
     - Non-blocking
   * - Store (uniform subsets)
     - :c:func:`Fenix_Data_member_store`
     - :c:func:`Fenix_Data_member_istore` †
   * - Store (varying subsets)
     - :c:func:`Fenix_Data_member_storev` ‡
     - :c:func:`Fenix_Data_member_istorev` †
   * - Wait for completion
     - N/A
     - :c:func:`Fenix_Data_wait` †

† **Unimplemented** - these asynchronous operations are not yet available.

‡ **Mode 1 only** - :c:func:`Fenix_Data_member_storev` is only supported in IMR Mode 1 (Buddy), not Mode 5 (Parity).

Checkpoint Shortcuts
^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Function
     - Description
   * - :c:func:`Fenix_Data_checkpoint`
     - Store all members + commit in one call
   * - :c:func:`Fenix_Data_commit_barrier`
     - Commit + wait for all ranks (barrier)

Data Subset Options
^^^^^^^^^^^^^^^^^^^

A **data subset** specifies which portion of a data member to checkpoint or restore. Subsets enable partial checkpointing to reduce overhead when only some elements of an array change between checkpoints.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Subset
     - Description
   * - ``FENIX_DATA_SUBSET_FULL``
     - All elements of the data member (use for full checkpoints)
   * - ``FENIX_DATA_SUBSET_EMPTY``
     - No elements (use as placeholder when skipping a checkpoint)
   * - ``FENIX_DATA_SUBSET_PRESTAGED``
     - Previously staged elements via :c:func:`Fenix_Data_member_stage`
   * - Custom subset
     - Specify exact element ranges to checkpoint (see :doc:`data-recovery/data-subsets`)

**Creating custom subsets:**

- :c:func:`Fenix_Data_subset_create` - Regular stride patterns (all ranks use same subset)
- :c:func:`Fenix_Data_subset_createv` - Arbitrary ranges (can vary per rank with storev)

For detailed examples and patterns, see :doc:`/howto/partial-checkpoints`.

Common Patterns
---------------

**Pattern 1: Simple Checkpointing**

.. code-block:: c

   // Setup
   Fenix_Data_group_create(group_id, comm, 0, 5, policy, policy_val, &flag);
   Fenix_Data_member_create(group_id, member_id, data, count, MPI_DOUBLE);

   // In main loop
   for (int iter = 0; iter < max_iter; iter++) {
       compute(data);

       if (iter % 10 == 0) {
           Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
           Fenix_Data_commit(group_id, &timestamp);
       }
   }

**Pattern 2: Multiple Members**

.. code-block:: c

   // Checkpoint multiple related data structures together
   Fenix_Data_member_store(group_id, member_1, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_member_store(group_id, member_2, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_member_store(group_id, member_3, FENIX_DATA_SUBSET_FULL);

   // Atomic commit - all at same timestamp
   Fenix_Data_commit(group_id, &timestamp);

**Pattern 3: Recovery with Callbacks**

.. code-block:: c

   void recovery_callback(MPI_Comm comm, int error, void* ctx) {
       CheckpointContext* cp = (CheckpointContext*)ctx;

       Fenix_Data_member_restore(cp->group_id, cp->member_id,
                                 cp->buffer, cp->count,
                                 cp->last_timestamp, NULL);
   }

   CheckpointContext ctx = {group_id, member_id, data, count, 0};
   Fenix_Callback_register(recovery_callback, &ctx);

**Pattern 4: Dynamic Data (std::vector)**

.. code-block:: cpp

   std::vector<double> data(1000);

   // Use member_define for dynamic containers - can update buffer after resize
   fenix::data::member_define(group_id, member_id, data.data(), 1000, MPI_DOUBLE);

   // Store using the saved buffer - use store (uniform subsets) or storev (varying subsets)
   fenix::data::member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   fenix::data::commit(group_id, &timestamp);

   // Vector may resize and move in memory
   data.resize(2000);

   // Update member with new buffer location after resize
   fenix::data::member_define(group_id, member_id, data.data(), 2000, MPI_DOUBLE);

   // Restore to updated buffer location
   fenix::data::member_restore(group_id, member_id, data.data(),
                                data.size(), timestamp);

See :doc:`/guides/data-recovery` for conceptual overview and :doc:`/tutorials/02-data-recovery` for step-by-step examples.
