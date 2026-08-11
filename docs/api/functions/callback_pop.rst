callback_pop
============

.. operation:: local

Remove the most recently registered callback.

.. c:function:: int Fenix_Callback_pop()

   Remove the most recently registered POST_RECOVERY callback from the callback stack. Callbacks are managed in LIFO (last-in, first-out) order.

   :returns: :c:enumerator:`FENIX_SUCCESS` if successful, or an error code

   **Return Codes:**

   :c:enumerator:`FENIX_SUCCESS`
      The callback was successfully removed from the stack.

   :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
      Fenix has not been initialized via :c:func:`Fenix_Init`.

   :c:enumerator:`FENIX_ERROR_CALLBACK_NOT_REGISTERED`
      The callback stack is empty; no callbacks are registered to pop.

.. cpp:function:: int fenix::callback_pop(CallbackLocation location = POST_RECOVERY)

   :param CallbackLocation location: [in] Which callback stack to pop from. Either PRE_RECOVERY or POST_RECOVERY. Default: POST_RECOVERY.
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful, or an error code

   **Return Codes:**

   :c:enumerator:`FENIX_SUCCESS`
      The callback was successfully removed from the stack.

   :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
      Fenix has not been initialized via :cpp:func:`fenix::init`.

   :c:enumerator:`FENIX_ERROR_CALLBACK_NOT_REGISTERED`
      The specified callback stack is empty; no callbacks are registered to pop.

.. seealso::
   :c:func:`Fenix_Callback_register`, :c:func:`Fenix_Callback_invoke_all`
