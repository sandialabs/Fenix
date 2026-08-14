group_members
=============

.. operation:: local

Query the members in a data group. The C API provides functions to get the count and iterate by position

C API
-----

.. c:function:: int Fenix_Data_group_get_number_of_members(int group_id, int* number_of_members)

   Get the number of members in a data group.

   :param int group_id: [in] The data group to query. Must be a valid existing group.
   :param int* number_of_members: [out] Pointer to store the total count of data members in this group.
   :returns: FENIX_SUCCESS if successful, error code if group invalid

.. c:function:: int Fenix_Data_group_get_member_at_position(int group_id, int* member_id, int position)

   :param int group_id: [in] The data group to query. Must be a valid existing group.
   :param int* member_id: [out] Pointer to store the member ID at the specified position in the group. Members are ordered by creation time.
   :param int position: [in] The 0-based index of the member to retrieve. Must be in range [0, number_of_members).
   :returns: FENIX_SUCCESS if successful

C++ API
-------

.. cpp:function:: std::optional<std::vector<int>> fenix::data::group_members(int group_id)

   Get all member IDs in a data group.

   :param int group_id: [in] The data group to query
   :returns: std::optional containing vector of member IDs if group exists, std::nullopt otherwise

   Returns a vector of all member IDs in the group, ordered by creation time (first created member appears first). If the group does not exist, returns ``std::nullopt``.

Usage
-----

These functions are used to iterate over all data members in a group. This is useful for:

- Performing operations on all members (e.g., storing or restoring all data)
- Discovering member IDs after recovery when IDs may be dynamic
- Implementing generic checkpoint/restart logic that works with any group composition
- Checking if a given member was created before failure

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
       ret = Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
       if (ret != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to store member %d\n", member_id);
       } else {
           printf("Stored member %d (position %d)\n", member_id, i);
       }
   }

   // Commit the checkpoint
   ret = Fenix_Data_commit(group_id, FENIX_TIME_STAMP_IGNORE);

.. seealso::
   :c:func:`Fenix_Data_group_get_number_of_members`
