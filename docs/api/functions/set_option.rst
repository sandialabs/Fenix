set_option
==========

.. operation:: local

Configure global Fenix behavior.

This function allows customization of how Fenix handles recovery, spare ranks,
callbacks, and error conditions. Settings can be configured before or after
:c:func:`Fenix_Init`, though some settings only take effect if set before initialization.

.. c:function:: int Fenix_set_option(Fenix_Setting_name setting, unsigned option)

   :param Fenix_Setting_name setting: [in] The setting to configure
   :param unsigned option: [in] The new value (must be a valid option for the setting)
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: void fenix::set_option(SettingName name, int value)

   :param SettingName name: [in] The setting to configure
   :param int value: [in] The new value

.. note::
   The C++ overload accepts the value directly instead of through a pointer.

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Setting configured successfully
- :c:enumerator:`FENIX_ERROR_INVALID_SETTING_NAME` - Unknown setting name
- :c:enumerator:`FENIX_ERROR_INVALID_SETTING_OPTION` - Invalid value for this setting

**Available Settings:**

Recovery Mode (FENIX_RECOVERY_MODE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls whether and how communicators are repaired after failures.

.. c:macro:: FENIX_RECOVERY_IGNORE

   Don't repair communicator, resume immediately.

.. c:macro:: FENIX_RECOVERY_NOOP

   Don't repair, but invoke callbacks normally (for testing/debugging).

.. c:macro:: FENIX_RECOVERY_REPAIR

   Repair communicator using spare ranks or by shrinking (default).

.. c:macro:: FENIX_RECOVERY_SPAWN

   Repair and attempt to respawn failed processes (UNIMPLEMENTED).

**Example:**

.. code-block:: c

   // Disable automatic recovery for testing
   Fenix_set_option(FENIX_RECOVERY_MODE, FENIX_RECOVERY_NOOP);

Resume Mode (FENIX_RESUME_MODE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls how control returns to the application after recovery.

.. c:macro:: FENIX_RESUME_JUMP

   Use longjmp to return to Fenix_Init (default). Fast but has undefined behavior
   with some C++ constructs and compiler optimizations.

.. c:macro:: FENIX_RESUME_RETURN

   Return error code inline (no jump). Application must check return codes and
   manually handle recovery.

.. c:macro:: FENIX_RESUME_THROW

   Throw :cpp:class:`fenix::CommException` (C++ only). Clean exception-based recovery.

**Example:**

.. code-block:: cpp

   // Use exceptions for clean C++ recovery
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

**Recommendation:** Use FENIX_RESUME_THROW for C++ applications, or FENIX_RESUME_RETURN
if you need fine-grained control over recovery handling.

Unhandled Error Mode (FENIX_UNHANDLED_MODE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls behavior when unhandled errors occur (e.g., invalid ranks).

.. c:macro:: FENIX_UNHANDLED_SILENT

   Silently ignore unhandled errors.

.. c:macro:: FENIX_UNHANDLED_PRINT

   Print error message and continue.

.. c:macro:: FENIX_UNHANDLED_ABORT

   Print error and abort (default). Safest for production.

**Example:**

.. code-block:: c

   // During development, print errors but don't abort
   Fenix_set_option(FENIX_UNHANDLED_MODE, FENIX_UNHANDLED_PRINT);

Callback Exception Mode (FENIX_CALLBACK_EXCEPTION_MODE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls whether CommExceptions from callbacks propagate.

.. c:macro:: FENIX_CALLBACK_EXCEPTION_RETHROW

   Allow exceptions to propagate out of callbacks (default).

.. c:macro:: FENIX_CALLBACK_EXCEPTION_SQUASH

   Catch and suppress exceptions from callbacks.

**Example:**

.. code-block:: cpp

   // Prevent callback exceptions from disrupting recovery
   fenix::set_option(fenix::CALLBACK_EXCEPTION_MODE, fenix::CALLBACK_EXCEPTION_SQUASH);

Message Logging Recovery Mode (FENIX_MLOG_RECOVERY_MODE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls how message logging recovery is performed.

.. c:macro:: FENIX_MLOG_RECOVERY_MANUAL

   All recovery is manual (application controls replay).

.. c:macro:: FENIX_MLOG_RECOVERY_INLINE

   Automatically replay failed logged MPI operations.

.. c:macro:: FENIX_MLOG_RECOVERY_INLINE_AUTOSYNC

   Automatic replay plus automatic log synchronization.

**Example:**

.. code-block:: c

   // Enable automatic message replay
   Fenix_set_option(FENIX_MLOG_RECOVERY_MODE, FENIX_MLOG_RECOVERY_INLINE);

Spare Wait Mode (FENIX_SPARE_WAIT_MODE) [Before Init Only]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls how spare ranks wait to be activated. **Must be set before Fenix_Init.**

.. c:macro:: FENIX_SPARE_WAIT_BUSY

   Busy-wait (fastest response, high CPU usage).

.. c:macro:: FENIX_SPARE_WAIT_YIELD

   Yield thread while waiting (balance of responsiveness and CPU).

.. c:macro:: FENIX_SPARE_WAIT_SLEEP

   Sleep 100ms between checks (lowest CPU, slower response).

**Example:**

.. code-block:: c

   // Reduce CPU usage of waiting spares
   Fenix_set_option(FENIX_SPARE_WAIT_MODE, FENIX_SPARE_WAIT_SLEEP);
   Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

Spare Finalize Mode (FENIX_SPARE_FINALIZE_MODE) [Before Init Only]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Controls what spare ranks do at finalization. **Must be set before Fenix_Init.**

.. c:macro:: FENIX_SPARE_FINALIZE_EXIT

   Spare ranks call MPI_Finalize and exit (default).

.. c:macro:: FENIX_SPARE_FINALIZE_RELEASE

   Spare ranks return from Fenix_Init with FENIX_ROLE_SPARE_RANK.

**Example:**

.. code-block:: c

   // Release spares for other use
   Fenix_set_option(FENIX_SPARE_FINALIZE_MODE, FENIX_SPARE_FINALIZE_RELEASE);
   Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

   if (role == FENIX_ROLE_SPARE_RANK) {
       printf("I was a spare, now available for other work\n");
       // Use this rank for something else
   }

**Complete Configuration Example:**

.. code-block:: cpp

   // Configure Fenix for C++ application
   int main(int argc, char** argv) {
       MPI_Init(&argc, &argv);

       // Must be before Init
       fenix::set_option(fenix::SPARE_WAIT_MODE, fenix::SPARE_WAIT_YIELD);
       fenix::set_option(fenix::SPARE_FINALIZE_MODE, fenix::SPARE_FINALIZE_EXIT);

       // Can be before or after Init
       fenix::set_option(fenix::RESUME_MODE, fenix::THROW);
       fenix::set_option(fenix::UNHANDLED_MODE, fenix::UNHANDLED_ABORT);

       int role, error;
       MPI_Comm fenix_comm;
       fenix::init({&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error});

       try {
           // Application code with automatic recovery
           run_simulation(fenix_comm);
       } catch (const fenix::CommException& e) {
           std::cerr << "Recovered from: " << e.what() << "\n";
       }

       fenix::finalize();
       MPI_Finalize();
       return 0;
   }

**Common Configurations:**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Configuration
     - Settings
   * - **C++ Exception-Based**
     - | FENIX_RESUME_MODE = FENIX_RESUME_THROW
       | Clean exception handling for C++ apps
   * - **Low CPU Overhead**
     - | FENIX_SPARE_WAIT_MODE = FENIX_SPARE_WAIT_SLEEP
       | Spare ranks sleep instead of busy-wait
   * - **Testing/Development**
     - | FENIX_RECOVERY_MODE = FENIX_RECOVERY_NOOP
       | FENIX_UNHANDLED_MODE = FENIX_UNHANDLED_PRINT
       | Test callbacks without actual recovery
   * - **Maximum Control**
     - | FENIX_RESUME_MODE = FENIX_RESUME_RETURN
       | FENIX_MLOG_RECOVERY_MODE = FENIX_MLOG_RECOVERY_MANUAL
       | Application fully controls recovery
   * - **Production Default**
     - | FENIX_RECOVERY_MODE = FENIX_RECOVERY_REPAIR
       | FENIX_RESUME_MODE = FENIX_RESUME_JUMP (C) or THROW (C++)
       | FENIX_UNHANDLED_MODE = FENIX_UNHANDLED_ABORT
       | Safe, automatic recovery

**Common Pitfalls:**

- **Setting spare modes after Init**: SPARE_WAIT_MODE and SPARE_FINALIZE_MODE must be set before Fenix_Init.
- **Using RESUME_JUMP with C++**: longjmp doesn't call destructors. Use RESUME_THROW for C++.
- **Forgetting to handle RESUME_RETURN**: If using RETURN mode, you must manually check and handle return codes.
- **Invalid option values**: Always use the defined macros, not arbitrary integers.

.. seealso::
   :c:func:`Fenix_get_option`, :c:func:`Fenix_Init`, :c:type:`Fenix_Setting_name`, :doc:`/guides/process-recovery`
