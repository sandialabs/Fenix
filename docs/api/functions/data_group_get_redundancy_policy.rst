group_get_redundancy_policy
===========================

.. operation:: local

Get the storage policy of a data group.

.. c:function:: int Fenix_Data_group_get_redundancy_policy(int group_id, int* policy_name, void* policy_value, int* flag)

   :param int group_id: Identifier to the data group to query
   :param int* policy_name: The identifier of the policy name of the data group
   :param void* policy_value: Location to store the policy_values this group's policy was configured with
   :param int* flag: Set to true if a policy value was extracted, else false
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_group_create`
