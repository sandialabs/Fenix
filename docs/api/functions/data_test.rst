test
====

.. operation:: local

Test for completion of a non-blocking data operation.

.. c:function:: int Fenix_Data_test(Fenix_Request request, int* flag)

   :param Fenix_Request request: Request handle from a non-blocking operation
   :param int* flag: Set to true if the operation has completed
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is currently unimplemented. Query completion of the store operation specified
   by the request.

.. seealso::
   :c:func:`Fenix_Data_wait`, :c:func:`Fenix_Data_member_istore`
