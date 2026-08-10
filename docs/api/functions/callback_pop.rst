callback_pop
============

.. operation:: local

Remove the most recently registered callback.

.. c:function:: int Fenix_Callback_pop()

   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::callback_pop(CallbackLocation location = POST_RECOVERY)

   :param CallbackLocation location: Which callback stack to pop from (default: POST_RECOVERY)
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Callback_register`
