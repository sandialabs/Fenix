member_attr_set
===============

.. operation:: local

Set the value of a member's attribute.

.. c:function:: int Fenix_Data_member_attr_set(int group_id, int member_id, int attribute_name, void* attribute_value, int* flag)

.. cpp:function:: int fenix::data::member_attr_set(int group_id, int member_id, int attr, void* value, int* flag)

   :param int group_id: The data group to update
   :param int member_id: The member to update
   :param int attr: The attribute to set. Valid: BUFFER, COUNT, or DATATYPE. COUNT and DATATYPE only settable before first store.
   :param void* value: Pointer to new attribute value
   :param int* flag: Set to 1 if successful, 0 otherwise
   :returns: FENIX_SUCCESS if successful

.. note::
   Valid names are FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
   and FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE. The COUNT and DATATYPE attributes may only
   be set before the first store operation. Contrary to the Fenix specification, returning
   to Fenix_Init after a failure does not allow the user to set these attributes again.

**Common Use Cases:**

- **Dynamic memory reallocation**: Update BUFFER attribute when using realloc() or std::vector::resize()
- **Pointer updates**: Keep Fenix in sync when application data structures are reorganized in memory

.. seealso::
   :c:func:`Fenix_Data_member_attr_get`, :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_define`
