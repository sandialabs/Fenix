callback_register
=================

.. operation:: local

Register a callback function to execute during recovery.

Callbacks are invoked at specific points during the recovery process (before or after communicator repair).
They allow applications to perform custom recovery actions.

.. c:function:: int Fenix_Callback_register(void (*callback)(MPI_Comm, int, void*), void* callback_data)

   :param callback: [in] Function pointer to call during recovery. The function receives the repaired communicator, error code, and user data.
   :param void* callback_data: [in] Optional user data pointer passed to callback function. May be NULL if no context needed.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: void fenix::callback_register(std::function<void(MPI_Comm, int)> callback, CallbackLocation location = POST_RECOVERY)

   :param callback: [in] std::function or lambda to call during recovery. Receives the repaired communicator and error code.
   :param CallbackLocation location: [in] When to invoke the callback. Either PRE_RECOVERY (before repair) or POST_RECOVERY (after repair). Default: POST_RECOVERY.

.. note::
   The C++ overload uses std::function instead of function pointers, eliminating the need for a separate callback_data parameter.
   Callbacks can capture context via lambda closures.

.. important::
   Callbacks will be invoked by all survivor ranks. Callbacks will also be
   invoked by recovered ranks (former spares), but recovered ranks will have
   no callbacks registered since they were not executing application code before
   the failure.

**Callback Locations:**

Callbacks can be registered for different points in the recovery process:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - PRE_RECOVERY
     - POST_RECOVERY
   * - Timing
     - Before repair
     - After repair
   * - Communicator
     - May be revoked
     - Newly repaired
   * - Use case
     - Cleanup, logging, revoking subcomms
     - State restoration
   * - MPI operations
     - Unsafe (comm may be bad)
     - Safe (comm is repaired)

**Callback Execution Order:**

Multiple callbacks can be registered and are executed in LIFO (stack) order:

.. code-block:: c

   void callback1(MPI_Comm comm, int error, void* data) {
       printf("Callback 1\n");
   }

   void callback2(MPI_Comm comm, int error, void* data) {
       printf("Callback 2\n");
   }

   void callback3(MPI_Comm comm, int error, void* data) {
       printf("Callback 3\n");
   }

   Fenix_Callback_register(callback1, NULL);
   Fenix_Callback_register(callback2, NULL);
   Fenix_Callback_register(callback3, NULL);

   // On recovery, prints:
   // Callback 3
   // Callback 2
   // Callback 1

**Common Use Cases:**

1. **State restoration**: Restore checkpointed data after recovery
2. **Reinitialization**: Reset algorithm state after rank replacement
3. **Logging**: Record when and how many failures occur
4. **Resource management**: Clean up or reallocate resources
5. **User notification**: Alert user to failure and recovery

**Usage Examples:**

.. code-block:: c

   // C example - Track recovery events
   void my_callback(MPI_Comm comm, int error, void* data) {
       int* counter = (int*)data;
       (*counter)++;
       printf("Recovery occurred, count = %d\n", *counter);
   }

   int recovery_count = 0;
   Fenix_Callback_register(my_callback, &recovery_count);

.. code-block:: cpp

   // C++ example with lambda capture
   int recovery_count = 0;
   std::vector<double> simulation_data(1000);

   fenix::callback_register(
       [&recovery_count, &simulation_data](MPI_Comm comm, int error) {
           recovery_count++;
           std::cout << "Recovery #" << recovery_count << std::endl;

           // Restore data
           fenix::data::member_restore(
               group_id, member_id,
               simulation_data.data(),
               simulation_data.size()
           );
       },
       fenix::POST_RECOVERY
   );

.. seealso::
   :c:func:`Fenix_Callback_pop`, :c:func:`Fenix_Callback_invoke_all`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Init`, :doc:`/guides/process-recovery`
