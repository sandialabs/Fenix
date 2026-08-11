Types
=====

This page documents all enums, structs, and typedefs used in Fenix.

These types are shared between the C and C++ APIs. The C++ API provides type aliases
in the ``fenix`` namespace that map to the C types.

Enums
-----

.. c:type:: Fenix_Rank_role

   All possible roles returned by Fenix_Init.

   Describes the current process's state in reference to process recovery.

   **C++ type alias:**

   .. cpp:type:: fenix::Role

      Type alias for Fenix_Rank_role.

   **Enumerators:**

   .. c:enumerator:: FENIX_ROLE_INITIAL_RANK
   .. cpp:enumerator:: fenix::INITIAL_RANK

      No failures have occurred yet (value: 0)

   .. c:enumerator:: FENIX_ROLE_RECOVERED_RANK
   .. cpp:enumerator:: fenix::RECOVERED_RANK

      This rank was a spare before the most recent failure, or was just spawned (value: 1)

   .. c:enumerator:: FENIX_ROLE_SURVIVOR_RANK
   .. cpp:enumerator:: fenix::SURVIVOR_RANK

      This rank was not a spare before the most recent failure (value: 2)

   .. c:enumerator:: FENIX_ROLE_SPARE_RANK
   .. cpp:enumerator:: fenix::SPARE_RANK

      This rank was a spare when Fenix finalized (value: 3)

.. c:type:: Fenix_Setting_name

   Global Fenix settings.

   .. c:enumerator:: FENIX_RECOVERY_MODE

      See :c:type:`Fenix_Recovery_mode`

   .. c:enumerator:: FENIX_RESUME_MODE

      See :c:type:`Fenix_Resume_mode`

   .. c:enumerator:: FENIX_UNHANDLED_MODE

      See :c:type:`Fenix_Unhandled_mode`

   .. c:enumerator:: FENIX_CALLBACK_EXCEPTION_MODE

      See :c:type:`Fenix_Callback_exception_mode`

   .. c:enumerator:: FENIX_MLOG_RECOVERY_MODE

      See :c:type:`Fenix_Mlog_recovery_mode`

   .. c:enumerator:: FENIX_SPARE_WAIT_MODE

      See :c:type:`Fenix_Spare_wait_mode`

   .. c:enumerator:: FENIX_SPARE_FINALIZE_MODE

      See :c:type:`Fenix_Spare_finalize_mode`

   .. c:enumerator:: FENIX_SETTING_NAME_MAXCODE

      Not a valid option

.. c:type:: Fenix_Recovery_mode

   Options for recovering after a failed rank is detected.

   .. c:enumerator:: FENIX_RECOVERY_IGNORE

      Do not repair communicator, immediately resume per :c:macro:`FENIX_RESUME_MODE`

      Fenix will not automatically repair the communicator when a failure is detected.
      The error is passed directly to the application without any recovery actions.
      Use this for custom recovery implementations or when you want to handle failures
      manually.

   .. c:enumerator:: FENIX_RECOVERY_NOOP

      Do not repair communicator, otherwise behave normally.

      This includes calling the PRE_RECOVERY and POST_RECOVERY callbacks.

   .. c:enumerator:: FENIX_RECOVERY_REPAIR

      Repair the communicator with spares or by shrinking (default)

      When a failure is detected, Fenix automatically repairs the communicator.
      If spare ranks are available, they replace failed ranks (maintaining
      communicator size and rank IDs). If spares are exhausted, the communicator
      shrinks to exclude failed ranks (ranks may be renumbered).

   .. c:enumerator:: FENIX_RECOVERY_SPAWN

      As REPAIR, but attempt to respawn failed processes

      .. warning::
         **UNIMPLEMENTED** - This feature is not yet available

   .. c:enumerator:: FENIX_RECOVERY_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Resume_mode

   Options for passing control back to application after recovery.

   .. c:enumerator:: FENIX_RESUME_JUMP

      Return to Fenix_Init via longjmp (default, but not recommended for C++)

      After communicator recovery, Fenix uses the C library function ``longjmp``
      to jump back to ``Fenix_Init``, causing execution to restart from that point.
      This mimics traditional checkpoint/restart behavior.

      **Warning**: The value of variables set before the longjmp are subject to
      undefined behavior from compiler optimizations. To ensure expected behavior,
      any variables that will be used across the longjmp should be declared as
      ``volatile``, heap allocated, or global in scope.

      **C++ Warning**: Whether stack variables are automatically destructed when
      leaving stack frames via longjmp is undefined behavior in C++. RAII objects
      (smart pointers, locks, etc.) may leak. For this reason and the above, it is
      highly recommended to instead use FENIX_RESUME_THROW for C++ applications.

   .. c:enumerator:: FENIX_RESUME_RETURN

      Return the error code inline (no jump)

      After communicator recovery, the failing MPI function returns with an error
      code (such as ``MPI_ERR_PROC_FAILED``). Execution continues inline without
      jumping. The application must check return codes from MPI functions to detect
      and handle failures. This provides more predictable behavior than longjmp and
      allows for fine-grained recovery logic.

   .. c:enumerator:: FENIX_RESUME_THROW

      Throw a fenix::CommException (recommended for C++)

      After communicator recovery, Fenix throws a ``fenix::CommException`` which
      can be caught using standard C++ exception handling. The exception contains
      the repaired communicator and error information. This provides clean error
      handling with proper C++ semantics, including automatic destructor calls.
      This is the recommended approach for C++ applications.

   .. c:enumerator:: FENIX_RESUME_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Mlog_recovery_mode

   Message logging recovery modes.

   .. c:enumerator:: FENIX_MLOG_RECOVERY_MANUAL

      All message logging recovery is manual

   .. c:enumerator:: FENIX_MLOG_RECOVERY_INLINE

      Automatically repeats failed, logged MPI operations without
      disrupting normal application control flow.

      User is responsible for handling any recovery steps in Fenix callbacks.

   .. c:enumerator:: FENIX_MLOG_RECOVERY_INLINE_AUTOSYNC

      As INLINE, but automatically sync logs with FENIX_MLOG_CONTINUE.

      Invoked after post-recovery callbacks, immediately before resuming.
      Invoked regardless of the logged-or-not status of the failing message.

   .. c:enumerator:: FENIX_MLOG_RECOVERY_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Unhandled_mode

   Options for dealing with 'unhandled' errors, e.g. invalid rank IDs.

   .. c:enumerator:: FENIX_UNHANDLED_SILENT

      Ignore unhandled errors

   .. c:enumerator:: FENIX_UNHANDLED_PRINT

      Print error and continue without handling

   .. c:enumerator:: FENIX_UNHANDLED_ABORT

      Print error and abort Fenix's world (default)

   .. c:enumerator:: FENIX_UNHANDLED_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Spare_wait_mode

   Options for how spare ranks wait to be needed. Must be set before Fenix_Init to take effect.

   .. c:enumerator:: FENIX_SPARE_WAIT_BUSY

      Busy wait, consuming CPU time in exchange for faster response

   .. c:enumerator:: FENIX_SPARE_WAIT_YIELD

      Tell MPI to yield this thread while waiting (if supported, else busy wait)

   .. c:enumerator:: FENIX_SPARE_WAIT_SLEEP

      Sleep 100ms between checks to see if this thread is needed for recovery

   .. c:enumerator:: FENIX_SPARE_WAIT_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Callback_exception_mode

   Options for dealing with CommExceptions generated in callbacks.

   .. c:enumerator:: FENIX_CALLBACK_EXCEPTION_RETHROW

      CommExceptions are allowed to propagate out of callbacks

   .. c:enumerator:: FENIX_CALLBACK_EXCEPTION_SQUASH

      CommExceptions from callbacks are squashed

   .. c:enumerator:: FENIX_CALLBACK_EXCEPTION_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Spare_finalize_mode

   Options for what spare ranks should do when Fenix_Finalize is called.
   Must be set before Fenix_Init to take effect.

   .. c:enumerator:: FENIX_SPARE_FINALIZE_RELEASE

      Continue from Fenix_Init with Fenix_Rank_role FENIX_RANK_ROLE_SPARE

   .. c:enumerator:: FENIX_SPARE_FINALIZE_EXIT

      Finalize MPI and exit (default)

   .. c:enumerator:: FENIX_SPARE_FINALIZE_MODE_MAXCODE

      Not a valid option

Structs and Typedefs
---------------------

.. c:type:: Fenix_Data_subset

   Specifies which elements of a data member to checkpoint or restore.

   A **subset** defines element ranges within an array. Instead of checkpointing an entire array, you can checkpoint only the portions that changed or that are critical for recovery. This reduces checkpoint time and storage overhead.

   **Example use case:** If only elements 0-99 and 500-599 of a 1000-element array changed, create a subset for just those ranges rather than checkpointing all 1000 elements.

   **Common Predefined Subsets:**

   .. c:var:: const Fenix_Data_subset FENIX_DATA_SUBSET_FULL

      Checkpoint **all elements** of the data member.

      This is the most common option. Use when you need to checkpoint the entire array.

      **Example:**

      .. code-block:: c

         Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);

   .. c:var:: const Fenix_Data_subset FENIX_DATA_SUBSET_EMPTY

      Checkpoint **no elements** (empty subset).

      Use as a placeholder when you want to skip checkpointing a particular member during this iteration, but still participate in the collective operation.

   .. c:var:: const Fenix_Data_subset FENIX_DATA_SUBSET_PRESTAGED

      Checkpoint the element ranges that were previously staged via :c:func:`Fenix_Data_member_stage`.

      Use this when you've already called ``member_stage`` to prepare data, and now want to commit just those pre-staged elements.

   .. c:var:: Fenix_Data_subset* FENIX_DATA_SUBSET_IGNORE

      Special value meaning "don't report subset information."

      Pass this when calling restore functions if you don't need to know what subset
      was actually restored.

      **Example:**

      .. code-block:: c

         // Don't care about subset info
         Fenix_Data_member_restore(group_id, member_id, buffer, count, timestamp,
                                   FENIX_DATA_SUBSET_IGNORE);

   **Creating Custom Subsets:**

   Create custom subsets to checkpoint only specific element ranges:

   .. code-block:: c

      // Example: checkpoint elements 0-99 and 200-299 (skip 100-199)
      Fenix_Data_subset my_subset;
      int starts[] = {0, 200};
      int ends[] = {99, 299};
      Fenix_Data_subset_createv(2, starts, ends, &my_subset);

      Fenix_Data_member_store(group_id, member_id, my_subset);
      Fenix_Data_subset_delete(&my_subset);  // Clean up

   **Functions for creating subsets:**

   - :c:func:`Fenix_Data_subset_create` - For regular stride patterns (e.g., every Nth element)
   - :c:func:`Fenix_Data_subset_createv` - For arbitrary element ranges

   See :doc:`/howto/partial-checkpoints` for detailed examples and patterns.

.. c:type:: Fenix_Request

   Request handle for non-blocking Fenix operations.

   Analogous to MPI_Request, this handle tracks asynchronous Fenix operations
   and can be used with :c:func:`Fenix_Data_wait` or :c:func:`Fenix_Data_test`.

   .. warning::
      **Implementation status:** The asynchronous data operations using this type
      are currently **unimplemented**. This includes:

      - :c:func:`Fenix_Data_member_istore`
      - :c:func:`Fenix_Data_member_istorev`
      - :c:func:`Fenix_Data_wait`
      - :c:func:`Fenix_Data_test`

      Use the synchronous alternatives :c:func:`Fenix_Data_member_store` and
      :c:func:`Fenix_Data_member_storev` instead.

   **Example (currently unimplemented):**

   .. code-block:: c

      Fenix_Request request;
      // WARNING: This will fail - istore is unimplemented
      Fenix_Data_member_istore(group_id, member_id, FENIX_DATA_SUBSET_FULL, &request);

      // ... do other work ...

      Fenix_Data_wait(&request);  // Wait for store to complete

.. c:type:: Fenix_Serialize_file_fn

   Function pointer type for custom serialization functions.

   Use this when data structures can't be represented with MPI datatypes alone
   (e.g., linked lists, trees, complex nested structures).

   **Function Signature:**

   .. code-block:: c

      typedef void (*Fenix_Serialize_file_fn)(void* buffer, FILE* fp, void* ctx);

   **Parameters:**

   - ``buffer``: Pointer to the data to serialize/deserialize
   - ``fp``: File handle to read from or write to
   - ``ctx``: User-defined context passed during member creation

   **Example - Serializing a Linked List:**

   .. code-block:: c

      typedef struct Node {
          int value;
          struct Node* next;
      } Node;

      void serialize_list(void* buffer, FILE* fp, void* ctx) {
          Node* head = (Node*)buffer;
          int* is_write = (int*)ctx;

          if (*is_write) {
              // Serialize (write to fp)
              int count = 0;
              for (Node* n = head; n != NULL; n = n->next) count++;
              fwrite(&count, sizeof(int), 1, fp);

              for (Node* n = head; n != NULL; n = n->next) {
                  fwrite(&n->value, sizeof(int), 1, fp);
              }
          } else {
              // Deserialize (read from fp)
              int count;
              fread(&count, sizeof(int), 1, fp);

              Node** tail_ptr = (Node**)buffer;
              *tail_ptr = NULL;

              for (int i = 0; i < count; i++) {
                  Node* node = malloc(sizeof(Node));
                  fread(&node->value, sizeof(int), 1, fp);
                  node->next = NULL;

                  if (*tail_ptr == NULL) {
                      *tail_ptr = node;
                  } else {
                      (*tail_ptr)->next = node;
                  }
                  tail_ptr = &node->next;
              }
          }
      }

      // Use with member creation
      int is_write = 1;
      Node* list = create_linked_list();
      Fenix_Data_member_fcreate(group_id, member_id, list, 1, MPI_BYTE,
                                 serialize_list, &is_write);

Data Policy Constants
---------------------

.. c:macro:: FENIX_DATA_POLICY_IN_MEMORY_RAID

   In-Memory RAID (IMR) redundancy policy.

   Stores checkpoint data redundantly across surviving ranks using RAID-like parity
   encoding. This provides resilience without requiring external storage (like
   parallel file systems).

   **How It Works:**

   - Data is distributed and replicated across multiple ranks
   - Parity information allows reconstruction after rank failures
   - No external I/O required - all data stays in memory
   - Automatic reconstruction when ranks fail

   **Policy Value:**

   When creating a group with this policy, pass a pointer to an ``int`` specifying
   the separation factor (number of ranks between redundant copies):

   .. code-block:: c

      int separation = 1;  // Place redundant copies 1 rank apart
      Fenix_Data_group_create(group_id, comm, 0, depth,
                              FENIX_DATA_POLICY_IN_MEMORY_RAID,
                              &separation, &flag);

   **Shorter Alias:**

   .. c:macro:: FENIX_DATA_POLICY_IMR

      Alias for :c:macro:`FENIX_DATA_POLICY_IN_MEMORY_RAID`.

   **Memory Overhead:**

   IMR policy adds memory overhead for redundancy. The exact overhead depends on
   the number of ranks and separation factor, but typically ranges from 50-100%
   additional memory per rank.

   **Performance:**

   - Checkpointing: O(data size) communication to distribute redundant copies
   - Recovery: O(data size) communication to reconstruct lost data
   - No disk I/O overhead
   - Scales well with rank count

   See :doc:`/guides/data-recovery` for more details on redundancy policies.
