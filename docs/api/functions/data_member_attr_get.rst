member_attr_get
===============

.. operation:: local

Get the value of a member's attribute.

.. c:function:: int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename, void* attributevalue, int* flag, int source_rank)

   :param int group_id: The group to query
   :param int member_id: The member to query
   :param int attributename: The attribute to query
   :param void* attributevalue: The value of the attribute
   :param int* flag: Set to true if the attribute was retrieved, else false
   :param int source_rank: Rank to retrieve attribute from
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is currently unimplemented.

.. seealso::
   :c:func:`Fenix_Data_member_attr_set`
