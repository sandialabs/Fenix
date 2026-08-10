wait
====

.. operation:: local

Wait for a non-blocking data operation to complete.

.. c:function:: int Fenix_Data_wait(Fenix_Request request)

.. cpp:function:: int fenix::data::wait(Fenix_Request request)

   :param Fenix_Request request: The request to wait on
   :returns: FENIX_SUCCESS when the operation completes

.. seealso::
   :c:func:`Fenix_Data_test`, :c:func:`Fenix_Data_member_istore`
