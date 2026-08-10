group_create
============

.. operation:: collective

Create a new data group for checkpointing.

A data group is a collection of data members that are checkpointed together using a specified
redundancy policy.

.. c:function:: int Fenix_Data_group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

.. cpp:function:: int fenix::data::group_create(int group_id, MPI_Comm comm, int start_time_stamp, int depth, int policy_name, void* policy_value, int* flag)

   :param int group_id: Unique identifier for this group
   :param MPI_Comm comm: MPI communicator for this group
   :param int start_time_stamp: Initial timestamp
   :param int depth: Number of checkpoint versions to maintain
   :param int policy_name: Redundancy policy to use
   :param void* policy_value: Policy-specific configuration
   :param int* flag: Result flag
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_member_create`, :doc:`/guides/data-recovery`
