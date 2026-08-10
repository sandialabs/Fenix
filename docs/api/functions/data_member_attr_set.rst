member_attr_set
===============

.. operation:: local

Set the value of a member's attribute.

.. c:function:: int Fenix_Data_member_attr_set(int group_id, int member_id, int attribute_name, void* attribute_value, int* flag)

.. cpp:function:: int fenix::data::member_attr_set(int group_id, int member_id, int attr, void* value, int* flag)

   :param int group_id: The group to update
   :param int member_id: The member to update
   :param int attr: The attribute to update
   :param void* value: The new value of the attribute
   :param int* flag: Set to true if the attribute was set, else false
   :returns: FENIX_SUCCESS if successful

.. note::
   Valid names are FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
   and FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE. The COUNT and DATATYPE attributes may only
   be set before the first store operation. Contrary to the Fenix specification, returning
   to Fenix_Init after a failure does not allow the user to set these attributes again.

.. seealso::
   :c:func:`Fenix_Data_member_attr_get`, :c:func:`Fenix_Data_member_create`
