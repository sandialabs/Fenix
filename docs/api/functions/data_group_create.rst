group_create
============

.. operation:: collective

Create a new data group for checkpointing application data.

A data group is a logical container for data members that share the same redundancy policy
and are committed together. Groups enable coordinated checkpoint/restore operations across
multiple pieces of application state.

.. c:function:: int Fenix_Data_group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

.. cpp:function:: int fenix::data::group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

   :param int group_id: User-defined unique identifier for this group (must be unique across the application)
   :param MPI_Comm comm: MPI communicator defining the scope of this data group (typically the Fenix resilient communicator)
   :param int start_time_stamp: Initial timestamp value for checkpoints in this group (typically 0)
   :param int depth: Number of older checkpoint versions to retain in addition to the most recent one. ``depth = 0`` keeps only the most recent snapshot.
   :param int policy_name: Redundancy policy identifier. Currently only FENIX_DATA_POLICY_IN_MEMORY_RAID is supported.
   :param void* policy_value: Policy-specific configuration (interpretation depends on policy_name).
   :param int* flag: Status flag indicating success (FENIX_SUCCESS) or error condition
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::group_create(int group_id, fenix::data::GroupCreateArgs args)

   :param int group_id: User-defined unique identifier for this group (must be unique across the application)
   :param GroupCreateArgs args: Struct containing the normal arguments of ``group_create`` with the same names and sane defaults.

It is permitted (and expected) for ranks to call this function multiple times
for the same group. After a failure, recovered ranks must create the group and
surviving ranks must participate to recreate internal group communicators and
state.

**Redundancy Policies:**

Currently, only :c:macro:`FENIX_DATA_POLICY_IN_MEMORY_RAID` is supported (see :doc:`/api/types` for details).

For this policy, ``policy_value`` should be cast from an ``int[2]`` or ``int[3]`` specifying the
mode (1 or 5), separation factor (number of ranks between redundant copies), and (for mode 5)
the redundancy group size.

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
       0,                   // Keep no extra checkpoints
       FENIX_DATA_POLICY_IN_MEMORY_RAID,
       (int[]){1, separation},
       &flag
   );

   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to create group: %d\n", ret);
   }

.. code-block:: cpp

   // C++ example
   int group_id = 1;
   fenix::data::group_create(group_id, { .comm = fenix_comm });

**Checkpoint Depth:**

The ``depth`` parameter controls how many checkpoint versions are retained:

- ``depth = 0``: Only the most recent checkpoint is kept
- ``depth > 0``: Multiple versions are kept for rollback to earlier states

When a new checkpoint is committed that would exceed the depth, the oldest checkpoint
is automatically deleted.

.. seealso::
   :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_group_delete`, :c:func:`Fenix_Data_group_created`, :c:func:`Fenix_Data_commit`, :doc:`/guides/data-recovery`
