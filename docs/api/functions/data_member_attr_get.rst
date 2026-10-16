member_attr_get
===============

.. operation:: local

Get the value of a member's attribute.

.. c:function:: int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename, void* attributevalue, int* flag, int source_rank)

   :param int group_id: The data group containing the member. Must be a valid group ID.
   :param int member_id: The member whose attribute to query. Must exist in the specified group.
   :param int attributename: The attribute to retrieve. Valid values: FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, or FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE.
   :param void* attributevalue: Pointer to location where attribute value will be stored. Type depends on attribute being queried.
   :param int* flag: Set to 1 if the attribute was successfully retrieved, 0 otherwise.
   :param int source_rank: Rank (in the group's communicator) from which to retrieve the attribute value.
   :returns: FENIX_SUCCESS if successful

.. warning::
   **Implementation status:**

   This function is **not fully implemented**. It currently returns FENIX_SUCCESS
   without retrieving any attribute values. The ``attributevalue`` and ``flag``
   parameters are not modified.

.. seealso::
   :c:func:`Fenix_Data_member_attr_set`
