group_get_redundancy_policy
===========================

.. operation:: local

Get the storage policy of a data group.

.. c:function:: int Fenix_Data_group_get_redundancy_policy(int group_id, int* policy_name, void* policy_value, int* flag)

   :param int group_id: The data group to query. Must be a valid group created with :c:func:`Fenix_Data_group_create`.
   :param int* policy_name: The redundancy policy identifier for this group (e.g., FENIX_DATA_POLICY_IN_MEMORY_RAID).
   :param void* policy_value: Location to store the policy-specific configuration value this group was created with. Caller responsible for freeing.
   :param int* flag: Set to 1 if a policy value was successfully retrieved, 0 otherwise.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. seealso::
   :c:func:`Fenix_Data_group_create`
