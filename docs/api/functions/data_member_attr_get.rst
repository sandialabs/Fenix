member_attr_get
===============

.. operation:: local

Get the value of a member's attribute.

.. c:function:: int Fenix_Data_member_attr_get(int group_id, int member_id, int attributename, void* attributevalue, int* flag, int source_rank)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member whose attribute to query. Must exist in the specified group.
   :param int attributename: [in] The attribute to retrieve. Valid values: FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER, FENIX_DATA_MEMBER_ATTRIBUTE_COUNT, or FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE.
   :param void* attributevalue: [out] Pointer to location where attribute value will be stored. Type depends on attribute being queried.
   :param int* flag: [out] Set to 1 if the attribute was successfully retrieved, 0 otherwise.
   :param int source_rank: [in] Rank (in the group's communicator) from which to retrieve the attribute value.
   :returns: FENIX_SUCCESS if successful

.. warning::
   **Implementation status:**

   This function is **not fully implemented**. It currently returns FENIX_SUCCESS
   without retrieving any attribute values. The ``attributevalue`` and ``flag``
   parameters are not modified.

   To query local member attributes, use :c:func:`Fenix_Data_member_attr_set` to
   track them in application code, or query the member structure directly in C++.

Example
-------

This example demonstrates the intended usage for querying member attributes during recovery:

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   int group_id, member_id;
   double* data;
   int count = 1000;

   // Create group and define member
   Fenix_Data_group_create(MPI_COMM_WORLD, 0, 0, 0, &group_id);
   Fenix_Data_member_create(group_id, data, count, MPI_DOUBLE, &member_id);

   // After recovery, query member attributes from a surviving rank
   int survived_rank = 0;  // A rank that did not fail
   void* buffer_ptr;
   int attr_count;
   MPI_Datatype attr_datatype;
   int flag;
   int ret;

   // Query the buffer pointer
   ret = Fenix_Data_member_attr_get(group_id, member_id,
                                     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                                     &buffer_ptr, &flag, survived_rank);
   if (ret == FENIX_SUCCESS && flag) {
       printf("Member buffer pointer: %p\n", buffer_ptr);
   }

   // Query the element count
   ret = Fenix_Data_member_attr_get(group_id, member_id,
                                     FENIX_DATA_MEMBER_ATTRIBUTE_COUNT,
                                     &attr_count, &flag, survived_rank);
   if (ret == FENIX_SUCCESS && flag) {
       printf("Member element count: %d\n", attr_count);
   }

   // Query the MPI datatype
   ret = Fenix_Data_member_attr_get(group_id, member_id,
                                     FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE,
                                     &attr_datatype, &flag, survived_rank);
   if (ret == FENIX_SUCCESS && flag) {
       printf("Member uses MPI datatype\n");
   }

.. note::
   Due to the current implementation status, this function always returns FENIX_SUCCESS
   but does not populate ``attributevalue`` or ``flag``. Use :c:func:`Fenix_Data_member_attr_set`
   to track attributes in your application until this function is fully implemented.

.. seealso::
   :c:func:`Fenix_Data_member_attr_set`
