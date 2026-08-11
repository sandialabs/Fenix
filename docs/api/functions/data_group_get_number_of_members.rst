group_get_number_of_members
===========================

.. operation:: local

Query the members in a data group. The C API provides functions to get the count and iterate by position, while the C++ API returns all member IDs directly.

C API
-----

.. c:function:: int Fenix_Data_group_get_number_of_members(int group_id, int* number_of_members)

   Get the number of members in a data group.

   :param int group_id: [in] The data group to query. Must be a valid existing group.
   :param int* number_of_members: [out] Pointer to store the total count of data members in this group.
   :returns: FENIX_SUCCESS if successful, error code if group invalid

C++ API
-------

.. cpp:function:: std::optional<std::vector<int>> fenix::data::group_members(int group_id)

   Get all member IDs in a data group.

   :param int group_id: [in] The data group to query
   :returns: std::optional containing vector of member IDs if group exists, std::nullopt otherwise

   Returns a vector of all member IDs in the group, ordered by creation time (first created member appears first). If the group does not exist, returns ``std::nullopt``.

Member Ordering
---------------

Members are returned in the order they were created using :c:func:`Fenix_Data_member_create` or :cpp:func:`fenix::data::member_create`. This ordering is preserved across all ranks and remains stable until members are deleted.

When to Use
-----------

This function is useful for:

- Iterating over all members in a group to perform batch operations
- Discovering which members exist after recovery when member IDs are dynamic
- Validating that all expected members have been created
- Debugging data recovery state

Example Usage
-------------

**C++ API:**

.. code-block:: cpp

   // Create a group and add members
   fenix::data::group_create(0);
   fenix::data::member_create(0, 100, data1, count1, MPI_DOUBLE);
   fenix::data::member_create(0, 200, data2, count2, MPI_INT);
   fenix::data::member_create(0, 300, data3, count3, MPI_FLOAT);

   // Query all member IDs
   auto members_opt = fenix::data::group_members(0);
   if (members_opt) {
       std::vector<int>& members = *members_opt;
       std::cout << "Group has " << members.size() << " members: ";
       for (int member_id : members) {
           std::cout << member_id << " ";
       }
       // Output: "Group has 3 members: 100 200 300 "
   }

   // Check if group exists
   if (!fenix::data::group_members(999)) {
       std::cout << "Group 999 does not exist\n";
   }

**C API:**

.. code-block:: c

   int group_id = 0;
   int num_members;

   // Get the count
   int ret = Fenix_Data_group_get_number_of_members(group_id, &num_members);
   if (ret == FENIX_SUCCESS) {
       printf("Group has %d members\n", num_members);

       // Iterate over all members
       for (int i = 0; i < num_members; i++) {
           int member_id;
           Fenix_Data_group_get_member_at_position(group_id, &member_id, i);
           printf("Position %d: member ID %d\n", i, member_id);
       }
   }

Notes
-----

- This is a local query operation; no communication occurs
- Member IDs can be any integer values; they do not need to be sequential
- After a member is deleted with :c:func:`Fenix_Data_member_delete`, it no longer appears in the list
- The returned order is consistent across all ranks in the group's communicator

.. seealso::
   :c:func:`Fenix_Data_group_get_member_at_position`, :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_created`, :cpp:func:`fenix::data::group_snapshots`
