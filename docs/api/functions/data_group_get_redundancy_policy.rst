group_get_redundancy_policy
===========================

.. operation:: local

Get the storage policy of a data group.

.. c:function:: int Fenix_Data_group_get_redundancy_policy(int group_id, int* policy_name, void* policy_value, int* flag)

   :param int group_id: [in] The data group to query. Must be a valid group created with :c:func:`Fenix_Data_group_create`.
   :param int* policy_name: [out] The redundancy policy identifier for this group (e.g., FENIX_DATA_POLICY_IN_MEMORY_RAID).
   :param void* policy_value: [out] Location to store the policy-specific configuration value this group was created with. For IMR policy, this is the separation factor (cast to int*). May be NULL if not needed.
   :param int* flag: [out] Set to 1 if a policy value was successfully retrieved, 0 otherwise.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. seealso::
   :c:func:`Fenix_Data_group_create`
