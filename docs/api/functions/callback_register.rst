callback_register
=================

.. operation:: local

Register a callback function to execute during recovery.

Callbacks are invoked at specific points during the recovery process (before or after communicator repair).
They allow applications to perform custom recovery actions.

.. c:function:: int Fenix_Callback_register(void (*callback)(MPI_Comm, int, void*), void* callback_data)

   :param callback: Function to call during recovery
   :param void* callback_data: User data passed to callback
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: void fenix::callback_register(std::function<void(MPI_Comm, int)> callback, CallbackLocation location = POST_RECOVERY)

   :param callback: Function to call during recovery
   :param CallbackLocation location: When to invoke the callback (default: POST_RECOVERY)

.. note::
   The C++ overload uses std::function instead of function pointers, eliminating the need for a separate callback_data parameter.
   Callbacks can capture context via lambda closures.

.. code-block:: c

   // C example
   void my_callback(MPI_Comm comm, int error, void* data) {
       int* counter = (int*)data;
       (*counter)++;
   }

   int counter = 0;
   Fenix_Callback_register(my_callback, &counter);

.. code-block:: cpp

   // C++ example
   int counter = 0;
   fenix::callback_register(
       [&counter](MPI_Comm comm, int error) {
           counter++;
       },
       fenix::POST_RECOVERY
   );

.. seealso::
   :c:func:`Fenix_Callback_pop`, :doc:`/guides/process-recovery`
