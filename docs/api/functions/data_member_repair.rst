member_repair
=============

.. operation:: collective

Repair the resilient storage of committed data for a member.

.. c:function:: int Fenix_Data_member_repair(int group_id, int member_id)

   :param int group_id: [in] The data group containing the member. All ranks in the group must provide the same value.
   :param int member_id: [in] The member whose redundant storage to repair/reconstruct after failures. All ranks must provide the same value. Member does not need to exist locally (will be created with NULL buffer if missing).
   :returns: FENIX_SUCCESS if successful, FENIX_ERROR_NODATA_FOUND if unrecoverable failure pattern

.. cpp:function:: int fenix::data::member_repair(int group_id, int member_id)

   :param int group_id: [in] The group of the member to repair
   :param int member_id: [in] The member to repair
   :returns: FENIX_SUCCESS if successful

.. note::
   All ranks in this group must call with the same group_id and member_id. May also be matched
   by a call to :c:func:`Fenix_Data_member_restore`. Member does not have to exist locally. If
   member does not exist locally, this function is equivalent to calling :c:func:`Fenix_Data_member_create`
   with a null buffer before this function's normal behavior. If the group's policy is unable to
   rebuild this member (i.e., in the case of an unrecoverable failure pattern), raises
   FENIX_ERROR_NODATA_FOUND.

.. warning::
   Behavior is currently undefined if this group's comm has a different size than it had when
   it committed any of the group's snapshots.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Member repaired successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_INVALID_MEMBERID` - Member does not exist in group
- :c:enumerator:`FENIX_ERROR_NODATA_FOUND` - Unable to rebuild member (unrecoverable failure pattern)

Example
-------

This example shows repairing redundant storage after process failure is detected:

.. code-block:: c

   int group_id = 0;
   int member_id = 0;
   int error_code;
   double *simulation_data;
   int data_size = 1000;

   // Detect and recover from process failure
   int flag;
   Fenix_Detect_failures(&flag);

   if (flag) {
       // Get repaired communicator
       MPI_Comm world_comm;
       Fenix_Get_repaired_comm(&world_comm);

       // Re-create data group with the repaired communicator
       int policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID;
       int policy_value = 1;
       Fenix_Data_group_create(group_id, world_comm, 0, policy_name,
                               &policy_value, 1, &error_code);

       // Repair the redundant storage for this member
       // This rebuilds the lost copies from surviving copies
       int ret = Fenix_Data_member_repair(group_id, member_id);

       if (ret == FENIX_SUCCESS) {
           // Now restore the data into local buffer
           simulation_data = (double *)malloc(data_size * sizeof(double));
           Fenix_Data_member_restore(group_id, member_id, simulation_data,
                                     data_size, 0);

           printf("Data member repaired and restored successfully\n");
       } else if (ret == FENIX_ERROR_NODATA_FOUND) {
           // Too many failures - unrecoverable
           printf("Cannot repair: unrecoverable failure pattern\n");
           // Application must handle this case (e.g., restart from disk)
       }
   }

.. seealso::
   :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_lrestore`
