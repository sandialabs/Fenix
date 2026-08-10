callback_invoke_all
===================

.. operation:: local

Invoke all registered callbacks with information from the last recovered fault.

.. c:function:: int Fenix_Callback_invoke_all()

   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::callback_invoke_all(CallbackLocation loc = POST_RECOVERY)

   :param CallbackLocation loc: Location of callbacks to invoke
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Callback_register`, :c:func:`Fenix_Callback_pop`
