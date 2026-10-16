callback_pop
============

.. operation:: local

Remove the most recently registered callback.

.. c:function:: int Fenix_Callback_pop()

   Remove the most recently registered POST_RECOVERY callback from the callback stack. Callbacks are managed in LIFO (last-in, first-out) order.

   :returns: :c:enumerator:`FENIX_SUCCESS` if successful, or an error code

.. cpp:function:: int fenix::callback_pop(CallbackLocation location = POST_RECOVERY)

   :param CallbackLocation location: Which callback stack to pop from. Either PRE_RECOVERY or POST_RECOVERY. Default: POST_RECOVERY.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful, or an error code

.. seealso::
   :c:func:`Fenix_Callback_register`, :c:func:`Fenix_Callback_invoke_all`
