Options
=======

Fenix setting options control how the library handles recovery, callback
behavior, message-logging recovery, and spare-rank lifecycle.

Use :c:func:`Fenix_set_option` or :cpp:func:`fenix::set_option` to configure an
option. Unless otherwise noted, options should be configured consistently across
ranks.

Process Recovery (core) Options
-------------------------------

.. c:enumerator:: FENIX_RECOVERY_MODE

   Whether and how Fenix repairs communicators after rank failure.

   - :c:enumerator:`FENIX_RECOVERY_IGNORE` -- immediately resume the application without performing any recovery operations
   - :c:enumerator:`FENIX_RECOVERY_NOOP` -- do not repair communicators, but otherwise behave normally (e.g., invoke callbacks)
   - :c:enumerator:`FENIX_RECOVERY_REPAIR` -- repair communicators with spares or by shrinking
   - :c:enumerator:`FENIX_RECOVERY_SPAWN` -- unimplemented respawn-based recovery mode

.. c:enumerator:: FENIX_RESUME_MODE

   How control returns to the application after recovery.

   - :c:enumerator:`FENIX_RESUME_JUMP` -- return to :c:func:`Fenix_Init` via ``longjmp``
   - :c:enumerator:`FENIX_RESUME_RETURN` -- return inline with an error code
   - :c:enumerator:`FENIX_RESUME_THROW` -- throw :cpp:class:`fenix::CommException`

.. c:enumerator:: FENIX_UNHANDLED_MODE

   How unhandled errors (MPI errors other than fault tolerance errors) are reported.

   - :c:enumerator:`FENIX_UNHANDLED_SILENT` -- ignore unhandled errors - the error will be returned by the MPI function.
   - :c:enumerator:`FENIX_UNHANDLED_PRINT` -- print a warning and continue - the error will be returned by the MPI function.
   - :c:enumerator:`FENIX_UNHANDLED_ABORT` -- print a warning and abort Fenix's world

.. c:enumerator:: FENIX_CALLBACK_EXCEPTION_MODE

   What happens when callbacks throw ``CommException``.

   - :c:enumerator:`FENIX_CALLBACK_EXCEPTION_RETHROW` -- let callback exceptions propagate out of error handlers
   - :c:enumerator:`FENIX_CALLBACK_EXCEPTION_SQUASH` -- suppress callback exceptions

Spare Rank Options
__________________

.. note::
   Spare rank options must be set before initialization to take effect.

.. c:enumerator:: FENIX_SPARE_WAIT_MODE

   How spare ranks wait to be needed. It may be useful to configure this setting if you intend to launch
   your application with spare ranks colocated with application ranks (as opposed to allocating spare nodes for spare ranks).

   - :c:enumerator:`FENIX_SPARE_WAIT_BUSY` -- busy wait
   - :c:enumerator:`FENIX_SPARE_WAIT_YIELD` -- immediately yield the waiting thread between checks (when supported, else busy wait)
   - :c:enumerator:`FENIX_SPARE_WAIT_SLEEP` -- sleep 100ms between checks

.. c:enumerator:: FENIX_SPARE_FINALIZE_MODE

   What spare ranks do when :c:func:`Fenix_Finalize` is called.

   Available settings:

   - :c:enumerator:`FENIX_SPARE_FINALIZE_RELEASE` -- return from :c:func:`Fenix_Init` with role :c:enumerator:`FENIX_ROLE_SPARE_RANK`
   - :c:enumerator:`FENIX_SPARE_FINALIZE_EXIT` -- finalize MPI and exit - application never sees them again.

Message Recovery Options
------------------------

.. c:enumerator:: FENIX_MLOG_RECOVERY_MODE

   How message-logging recovery is handled. Note that options other than manual
   typically require the application to perform recovery operations in a Fenix
   callback (e.g., to repair data members after rank failures).

   - :c:enumerator:`FENIX_MLOG_RECOVERY_MANUAL` -- all message-log recovery is manual
   - :c:enumerator:`FENIX_MLOG_RECOVERY_INLINE` -- automatically replay failed logged MPI operations
   - :c:enumerator:`FENIX_MLOG_RECOVERY_INLINE_AUTOSYNC` -- as inline recovery, but also synchronize logs with :c:macro:`FENIX_MLOG_CONTINUE`

.. seealso::
   :c:func:`Fenix_set_option`, :c:func:`Fenix_get_option`, :c:type:`Fenix_Setting_name`
