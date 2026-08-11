group_get_member_at_position
============================

.. operation:: local

Get member ID based on member index within a group.

.. c:function:: int Fenix_Data_group_get_member_at_position(int group_id, int* member_id, int position)

   :param int group_id: [in] The data group to query. Must be a valid existing group.
   :param int* member_id: [out] Pointer to store the member ID at the specified position in the group. Members are ordered by creation time.
   :param int position: [in] The 0-based index of the member to retrieve. Must be in range [0, number_of_members).
   :returns: FENIX_SUCCESS if successful, error code if position out of range or group invalid

Usage
-----

This function is typically used in combination with :c:func:`Fenix_Data_group_get_number_of_members` to iterate over all data members in a group. This is useful for:

- Performing operations on all members (e.g., storing or restoring all data)
- Discovering member IDs after recovery when IDs may be dynamic
- Implementing generic checkpoint/restart logic that works with any group composition

Example
-------

Iterate over all members in a group and checkpoint each one:

.. code-block:: c

   int group_id = 0;
   int num_members;
   int ret;

   // First, get the total number of members
   ret = Fenix_Data_group_get_number_of_members(group_id, &num_members);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to get member count\n");
       return ret;
   }

   printf("Checkpointing %d members from group %d\n", num_members, group_id);

   // Iterate through each member by position
   for (int i = 0; i < num_members; i++) {
       int member_id;

       // Get the member ID at this position
       ret = Fenix_Data_group_get_member_at_position(group_id, &member_id, i);
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to get member at position %d\n", i);
           continue;
       }

       // Store this member's data
       ret = Fenix_Data_member_store(member_id, FENIX_DATA_POLICY_IN_MEMORY_RAID);
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to store member %d\n", member_id);
       } else {
           printf("Stored member %d (position %d)\n", member_id, i);
       }
   }

   // Commit the checkpoint
   ret = Fenix_Data_commit(group_id, NULL);

.. seealso::
   :c:func:`Fenix_Data_group_get_number_of_members`
