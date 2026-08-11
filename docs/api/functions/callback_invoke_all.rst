callback_invoke_all
===================

.. operation:: local

Invoke all registered callbacks with information from the last recovered fault.

.. c:function:: int Fenix_Callback_invoke_all()

   Invoke all registered POST_RECOVERY callbacks manually. Useful for testing callback behavior without triggering actual recovery.

   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::callback_invoke_all(CallbackLocation loc = POST_RECOVERY)

   :param CallbackLocation loc: [in] Which set of callbacks to invoke. Either PRE_RECOVERY or POST_RECOVERY. Default: POST_RECOVERY.
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Callback_register`, :c:func:`Fenix_Callback_pop`
