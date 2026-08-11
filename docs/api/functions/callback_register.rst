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

.. c:macro:: FENIX_CALLBACK_PRE_RECOVERY

   Invoked before communicator repair begins. Use for cleanup or preparation.

.. c:macro:: FENIX_CALLBACK_POST_RECOVERY

   Invoked after communicator repair completes. Use for state restoration. (Default)

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

.. code-block:: c

   // C example - Restore checkpoint in callback
   void restore_callback(MPI_Comm comm, int error, void* ctx) {
       CheckpointData* cp_data = (CheckpointData*)ctx;

       // Restore application state
       Fenix_Data_member_restore(
           cp_data->group_id,
           cp_data->member_id,
           cp_data->buffer,
           cp_data->count,
           cp_data->last_timestamp,
           NULL
       );

       printf("Restored from checkpoint %d\n", cp_data->last_timestamp);
   }

   typedef struct {
       int group_id;
       int member_id;
       void* buffer;
       int count;
       int last_timestamp;
   } CheckpointData;

   CheckpointData cp_data = {1, 100, my_data, 1000, 0};
   Fenix_Callback_register(restore_callback, &cp_data);

   // Update timestamp after each checkpoint
   Fenix_Data_commit(cp_data.group_id, &cp_data.last_timestamp);

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
               simulation_data.size(),
               last_checkpoint_timestamp
           );
       },
       fenix::POST_RECOVERY
   );

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

**Pre-Recovery vs Post-Recovery:**

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
     - Cleanup, logging
     - State restoration
   * - MPI operations
     - Unsafe (comm may be bad)
     - Safe (comm is repaired)

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Callback registered successfully

**Common Use Cases:**

1. **State restoration**: Restore checkpointed data after recovery
2. **Reinitialization**: Reset algorithm state after rank replacement
3. **Logging**: Record when and how many failures occur
4. **Resource management**: Clean up or reallocate resources
5. **User notification**: Alert user to failure and recovery

**Example: Complete Checkpoint/Restore Pattern:**

.. code-block:: c

   // Global state
   typedef struct {
       int group_id;
       int member_id;
       double* data;
       int data_size;
       int current_timestamp;
   } AppState;

   AppState state;

   // Restore callback
   void restore_state(MPI_Comm comm, int error, void* ctx) {
       AppState* s = (AppState*)ctx;

       printf("Recovering from failure, restoring checkpoint %d\n", s->current_timestamp);

       int ret = Fenix_Data_member_restore(
           s->group_id, s->member_id,
           s->data, s->data_size,
           s->current_timestamp,
           NULL
       );

       if (ret == FENIX_SUCCESS) {
           printf("Restoration successful\n");
       } else if (ret == FENIX_ERROR_NODATA_FOUND) {
           printf("No checkpoint found, initializing from scratch\n");
           initialize_data(s->data, s->data_size);
       } else {
           fprintf(stderr, "Restoration failed: %d\n", ret);
       }
   }

   int main(int argc, char** argv) {
       MPI_Init(&argc, &argv);

       int role, error;
       MPI_Comm fenix_comm;
       Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

       // Initialize state
       state.group_id = 1;
       state.member_id = 100;
       state.data_size = 1000;
       state.data = malloc(state.data_size * sizeof(double));
       state.current_timestamp = 0;

       // Create data group and member
       int flag;
       int separation = 1;
       Fenix_Data_group_create(state.group_id, fenix_comm, 0, 5,
                               FENIX_DATA_POLICY_IN_MEMORY_RAID,
                               &separation, &flag);
       Fenix_Data_member_create(state.group_id, state.member_id,
                                state.data, state.data_size, MPI_DOUBLE);

       // Register restore callback
       Fenix_Callback_register(restore_state, &state);

       // Main computation loop
       for (int iter = 0; iter < 1000; iter++) {
           // Compute
           for (int i = 0; i < state.data_size; i++) {
               state.data[i] = compute(state.data[i], iter);
           }

           // Checkpoint every 10 iterations
           if (iter % 10 == 0) {
               Fenix_Data_member_store(state.group_id, state.member_id,
                                       FENIX_DATA_SUBSET_FULL);
               Fenix_Data_commit(state.group_id, &state.current_timestamp);
               printf("Iter %d: Created checkpoint %d\n", iter, state.current_timestamp);
           }
       }

       free(state.data);
       Fenix_Finalize();
       MPI_Finalize();
       return 0;
   }

**Common Pitfalls:**

- **Capturing stack variables**: Don't pass pointers to local variables that will go out of scope.
- **Unsafe MPI in pre-recovery**: The communicator may be revoked in PRE_RECOVERY callbacks.
- **Forgetting to update context**: If callback data changes (like checkpoint timestamp), ensure the pointer points to updated data.
- **Heavy computation in callbacks**: Keep callbacks fast - they block the recovery process.
- **Not handling NODATA**: First run has no checkpoints. Handle FENIX_ERROR_NODATA_FOUND gracefully.

**Performance Considerations:**

- Callbacks add latency to recovery process
- Keep callbacks short and focused
- Avoid unnecessary synchronization in callbacks
- Consider lazy restoration (restore only when needed, not in callback)

.. seealso::
   :c:func:`Fenix_Callback_pop`, :c:func:`Fenix_Callback_invoke_all`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Init`, :doc:`/guides/process-recovery`
