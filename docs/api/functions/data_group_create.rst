group_create
============

.. operation:: collective

Create a new data group for checkpointing application data.

A data group is a logical container for data members that share the same redundancy policy
and are committed together. Groups enable coordinated checkpoint/restore operations across
multiple pieces of application state.

.. c:function:: int Fenix_Data_group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

.. cpp:function:: int fenix::data::group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

   :param int group_id: [in] User-defined unique identifier for this group (must be unique across the application)
   :param MPI_Comm comm: [in] MPI communicator defining the scope of this data group (typically the Fenix resilient communicator)
   :param int start_time_stamp: [in] Initial timestamp value for checkpoints in this group (typically 0)
   :param int depth: [in] Number of checkpoint versions to retain. Older checkpoints are automatically garbage collected. Must be >= 1.
   :param int policy_name: [in] Redundancy policy identifier. Currently only FENIX_DATA_POLICY_IN_MEMORY_RAID is supported.
   :param void* policy_value: [in] Policy-specific configuration (interpretation depends on policy_name). For IMR, this specifies separation policy.
   :param int* flag: [out] Status flag indicating success (FENIX_SUCCESS) or error condition
   :returns: FENIX_SUCCESS if successful, error code otherwise

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Group created successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group ID is invalid or already exists
- :c:enumerator:`FENIX_ERROR_GROUP_CREATE` - Failed to create group (internal error)
- :c:enumerator:`FENIX_ERROR_INVALID_DEPTH` - Depth must be >= 1
- :c:enumerator:`FENIX_ERROR_INVALID_POLICY_NAME` - Unknown policy name

**Redundancy Policies:**

Currently, only :c:macro:`FENIX_DATA_POLICY_IN_MEMORY_RAID` is supported (see :doc:`/api/types` for details).

For this policy, ``policy_value`` should be cast from an int specifying the separation
factor (number of ranks between redundant copies).

**Usage Examples:**

.. code-block:: c

   // C example - Create a group with IMR policy
   int group_id = 1;
   int flag;
   int separation = 1;  // Separation between redundant data

   int ret = Fenix_Data_group_create(
       group_id,
       fenix_comm,          // Use Fenix resilient communicator
       0,                   // Start timestamp at 0
       5,                   // Keep 5 checkpoint versions
       FENIX_DATA_POLICY_IN_MEMORY_RAID,
       &separation,
       &flag
   );

   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to create group: %d\n", ret);
   }

.. code-block:: cpp

   // C++ example
   int group_id = 1;
   int flag;
   int separation = 1;

   int ret = fenix::data::group_create(
       group_id, fenix_comm, 0, 5,
       FENIX_DATA_POLICY_IN_MEMORY_RAID,
       &separation, &flag
   );

**Checkpoint Depth:**

The ``depth`` parameter controls how many checkpoint versions are retained:

- ``depth = 1``: Only the most recent checkpoint is kept
- ``depth > 1``: Multiple versions are kept for rollback to earlier states

When a new checkpoint is committed that would exceed the depth, the oldest checkpoint
is automatically deleted.

**Common Pitfalls:**

- **Non-unique group IDs**: Each group in the application must have a unique ID. Using duplicate IDs will fail.
- **Wrong communicator**: Use the Fenix resilient communicator (from Fenix_Init), not MPI_COMM_WORLD.
- **Depth too small**: If you need to restore from older checkpoints, ensure depth is large enough.
- **Forgetting to delete**: Call :c:func:`Fenix_Data_group_delete` when done to free resources.

**Performance Considerations:**

- Larger depth values consume more memory to store multiple checkpoint versions
- IMR policy distributes data across ranks, so memory overhead depends on rank count and separation factor
- Creating groups is relatively lightweight; most overhead comes from storing/committing data

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_group_delete`, :c:func:`Fenix_Data_group_created`, :c:func:`Fenix_Data_commit`, :doc:`/guides/data-recovery`, :doc:`/tutorials/02-data-recovery`
