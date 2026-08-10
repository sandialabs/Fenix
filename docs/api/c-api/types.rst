Types
=====

This page documents all enums, structs, and typedefs in the Fenix C API.

Enums
-----

.. c:type:: Fenix_Rank_role

   All possible roles returned by Fenix_Init.

   Describes the current process's state in reference to process recovery.

   .. c:enumerator:: FENIX_ROLE_INITIAL_RANK

      No failures have occurred yet (value: 0)

   .. c:enumerator:: FENIX_ROLE_RECOVERED_RANK

      This rank was a spare before the most recent failure, or was just spawned (value: 1)

   .. c:enumerator:: FENIX_ROLE_SURVIVOR_RANK

      This rank was not a spare before the most recent failure (value: 2)

   .. c:enumerator:: FENIX_ROLE_SPARE_RANK

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

   .. c:enumerator:: FENIX_RECOVERY_NOOP

      Do not repair communicator, otherwise behave normally.

      This includes calling the PRE_RECOVERY and POST_RECOVERY callbacks.

   .. c:enumerator:: FENIX_RECOVERY_REPAIR

      Repair the communicator with spares or by shrinking

   .. c:enumerator:: FENIX_RECOVERY_SPAWN

      As REPAIR, but attempt to respawn failed processes

      .. warning::
         **UNIMPLEMENTED** - This feature is not yet available

   .. c:enumerator:: FENIX_RECOVERY_MODE_MAXCODE

      Not a valid option

.. c:type:: Fenix_Resume_mode

   Options for passing control back to application after recovery.

   .. c:enumerator:: FENIX_RESUME_JUMP

      Return to Fenix_Init via longjmp (default)

      The value of variables set before the longjmp are subject to undefined
      behavior from compiler optimizations. To ensure expected behavior, it is
      recommended that any variables that will be used across the longjmp are
      declared as volatile, are heap allocated, or are global in scope.

      For C++ applications, whether stack variables are automatically
      destructed when leaving stack frames via longjmp is undefined. For this
      reason and the above, it is highly recommended to instead use
      FENIX_RESUME_THROW for C++ applications.

   .. c:enumerator:: FENIX_RESUME_RETURN

      Return the error code inline

   .. c:enumerator:: FENIX_RESUME_THROW

      Throw a fenix::CommException

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

   Describes a subset of data for partial store/restore operations.

.. c:type:: Fenix_Request

   Request handle for non-blocking Fenix operations.

.. c:type:: Fenix_Serialize_file_fn

   Function pointer type for custom serialization functions.
