Tutorial 3: Resume Modes and Recovery Control Flow
===================================================

**Time:** 20-30 minutes | **Difficulty:** Intermediate

**Prerequisites:** :doc:`01-first-program`, :doc:`02-data-recovery`

In the previous tutorials, you learned exception-based recovery where failures throw a ``fenix::CommException``. But exceptions are just one of **three ways** Fenix can return control to your application after a failure. This tutorial teaches you about all three **resume modes** and when to use each one.

.. contents:: In This Tutorial
   :local:
   :depth: 2

Learning Objectives
-------------------

By completing this tutorial, you will:

✓ Understand the three resume modes: THROW, RETURN, and JUMP

✓ Know when each mode is appropriate for your application

✓ Understand that resume modes can be switched dynamically

✓ Understand that callbacks work with any resume mode

✓ Understand how resume modes interact with message logging

What Are Resume Modes?
-----------------------

The Problem: How Should Failures Be Communicated?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When Fenix detects a rank failure, it repairs the communicator and needs to tell your application "a failure occurred, but it's been handled." There are three ways to do this:

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Resume Mode
     - How It Works
     - When You See It
   * - **THROW**
     - Throw ``fenix::CommException``
     - At the ``catch`` block
   * - **RETURN**
     - Return error code from function
     - Check return value: ``FENIX_ERROR_PROCESS_FAILURE``
   * - **JUMP**
     - Longjmp back to ``Fenix_Init``
     - Back at ``Fenix_Init`` (after ``setjmp``)

These are **not** different recovery strategies - they're just different ways to **communicate** that recovery happened. The actual recovery process (communicator repair, callbacks, data restoration) is the same regardless of resume mode.

Key Concepts
^^^^^^^^^^^^

**Important Distinctions:**

1. **Resume mode** (this tutorial): Controls how your app learns about failures

   - ``RESUME_THROW``: via exceptions
   - ``RESUME_RETURN``: via error codes
   - ``RESUME_JUMP``: via longjmp

2. **Message logging** (Tutorial 4): Controls if MPI messages replay automatically

   - This is completely separate!
   - Any resume mode can be combined with any message logging mode
   - Successful automatic replay will prevent resume modes from triggering (MPI function returns success after replay)

3. **Callbacks** (covered here): Work with **all** resume modes

   - Not tied to any specific resume mode
   - Execute before control returns, regardless of how it returns

Part 1: Exception-Based Recovery (THROW Mode)
----------------------------------------------

How It Works
^^^^^^^^^^^^

You've already used this mode in Tutorial 1. Here's a quick reminder:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     // THROW is the default mode for C++ API
     fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

     bool keep_running = true;
     while (keep_running) {
       try {
         // Do work with MPI calls
         MPI_Barrier(res_comm);
         keep_running = false;

       } catch (const fenix::CommException& e) {
         // Failure occurred and was repaired
         // res_comm is automatically updated via the pointer
         // Loop continues with repaired communicator
       }
     }

     fenix::finalize();
     MPI_Finalize();
   }

**When a Failure Occurs:**

1. MPI operation fails due to rank failure
2. Fenix repairs communicator and updates ``res_comm`` automatically
3. Fenix throws ``fenix::CommException``
4. Your ``catch`` block handles it
5. Execution continues from the catch block

**Trade-offs:**

.. list-table::
   :widths: 50 50
   :header-rows: 1

   * - Advantages
     - Disadvantages
   * - Clean, idiomatic C++
     - C++ only
   * - RAII-safe (destructors run)
     - Exception overhead
   * - Explicit control flow
     - Requires exception handling

Part 2: Return-Based Recovery (RETURN Mode)
--------------------------------------------

How It Works
^^^^^^^^^^^^

``RESUME_RETURN`` works in both C and C++. Instead of throwing exceptions or jumping, functions return an error code to indicate a failure occurred and was handled.

.. code-block:: cpp

   // Set RETURN mode (works in C and C++)
   fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);

With this mode set:

- MPI operations that encounter failures return MPI error codes
- Fenix communication functions (like ``checkpoint``, ``group_create``) may return ``FENIX_ERROR_PROCESS_FAILURE``
- Fenix C++ API functions won't throw ``fenix::CommException`` (but may still throw other ``fenix::RuntimeException`` types for non-communicator errors)
- You must check return codes to know if recovery occurred

What Needs Error Checking
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Fenix functions that communicate:**

- ``fenix::data::checkpoint()`` - stores and commits data
- ``fenix::data::group_create()`` - coordinates metadata across ranks
- ``fenix::data::commit_barrier()`` - collective commit operation
- ``fenix::data::member_store()`` - store operations
- Other collective Fenix operations

**MPI operations:**

- All MPI communication functions return normal MPI error codes after Fenix repairs
- Check return values: ``MPI_SUCCESS``, ``MPI_ERR_REVOKED``, etc.

**Important complexity:** Not all ranks will detect the error at the same operation. Some ranks may successfully complete an operation only to get an error at the next one, while others may get the error at the first. This makes writing correct recovery code with RETURN mode non-trivial.

Example: Basic Error Checking
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);

   // Checkpoint operation
   int ret = fenix::data::checkpoint(group_id);
   if (ret == FENIX_ERROR_PROCESS_FAILURE) {
     printf("Checkpoint interrupted by failure\n");
     // Recovery actions here
   }

   // Group creation
   ret = fenix::data::group_create(group_id);
   if (ret == FENIX_ERROR_PROCESS_FAILURE) {
     printf("Group creation interrupted\n");
     // Recovery actions here
   }

   // MPI operation
   ret = MPI_Barrier(res_comm);
   if (ret != MPI_SUCCESS) {
     printf("MPI operation failed: %d\n", ret);
     // Recovery actions here
   }

**Trade-offs:**

.. list-table::
   :widths: 50 50
   :header-rows: 1

   * - Advantages
     - Disadvantages
   * - Works in both C and C++
     - Requires intrusive changes to large applications
   * - Explicit error handling at each call
     - Complex: ranks detect errors at different times
   * - Fine-grained control
     - Significant code rewrites needed

**When to Use RETURN Mode:**

- Small sections of code that need careful error handling
- Third-party C libraries that need library-specific error handling before allowing the application to handle failures
- When you need fine-grained control over specific operations

**Not Recommended For:**

- Large-scale applications (too many intrusive changes)
- General application-level recovery (use THROW or JUMP instead)

Part 3: Longjmp-Based Recovery (JUMP Mode)
-------------------------------------------

How It Works
^^^^^^^^^^^^

``RESUME_JUMP`` uses C's ``setjmp``/``longjmp`` mechanism to return control back to ``Fenix_Init`` after a failure. This is practical for large-scale C applications where adding error checking throughout the codebase would be infeasible.

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     int role, error;

     // Fenix_Init macro includes setjmp
     Fenix_Init(&role, MPI_COMM_WORLD, &res_comm, &argc, &argv, 1, &error);

     // JUMP is the default mode for C API
     Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_JUMP);

     // If a failure occurs anywhere below, execution jumps back to Fenix_Init
     volatile int iteration = 0;
     for (iteration = 0; iteration < 100; iteration++) {
       // Do work
       MPI_Barrier(res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**When a Failure Occurs:**

1. MPI operation encounters failure
2. Fenix repairs communicator and updates ``res_comm`` automatically
3. Callbacks execute
4. ``longjmp`` returns control to the ``Fenix_Init`` point
5. Code after ``Fenix_Init`` re-executes from the beginning

Handling Variables Safely
^^^^^^^^^^^^^^^^^^^^^^^^^^

Stack variables need to be ``volatile`` to preserve values across longjmp:

.. code-block:: c

   Fenix_Init(...);

   int counter = 0;              // Value undefined after longjmp!
   volatile int safe_counter = 0;  // Value preserved

   for (int i = 0; i < 100; i++) {
     counter++;         // Unreliable after recovery
     safe_counter++;    // Reliable after recovery
     MPI_Barrier(comm);
   }

Cleaning Up Application State
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Applications can register callbacks to clean up allocated resources. Callbacks can be pushed/popped as memory is allocated/freed:

.. code-block:: c

   void* allocate_work_buffer(size_t size) {
     void* buffer = malloc(size);

     // Register cleanup callback for this buffer
     Fenix_Callback_register(cleanup_buffer, buffer);

     return buffer;
   }

   void free_work_buffer(void* buffer) {
     free(buffer);

     // Remove the cleanup callback
     Fenix_Callback_pop();
   }

   void cleanup_buffer(MPI_Comm comm, int error, void* buffer) {
     // This will be called during recovery to free the buffer
     free(buffer);
   }

**Trade-offs:**

.. list-table::
   :widths: 50 50
   :header-rows: 1

   * - Advantages
     - Disadvantages
   * - No error checking needed
     - Non-volatile variables have undefined values
   * - Practical for large C codebases
     - C++ destructors may not be called
   * - Simple recovery model
     - Potential resource leaks
   * - Callbacks can manage cleanup
     - Requires careful state management

**When to Use JUMP Mode:**

- Large C applications where adding return-code checking throughout would be impractical
- Applications with simple state that can restart from ``Fenix_Init``
- Legacy Fenix applications already using this pattern
- When callback-based cleanup is sufficient for resource management

Part 4: Dynamic Mode Switching
-------------------------------

Resume modes can be changed at any time during execution using ``fenix::set_option()`` or ``Fenix_set_option()``. This allows using different modes for different application phases (e.g., RETURN for initialization, THROW for main computation).

Part 5: Callbacks Work With All Resume Modes
---------------------------------------------

Callbacks Are Independent
^^^^^^^^^^^^^^^^^^^^^^^^^^

Callbacks execute after communicator repair but **before** control returns to your application, regardless of the resume mode.

**Callback Execution Order:**

.. code-block:: text

   1. Failure detected
   2. Communicator repaired (res_comm updated automatically)
   3. → Callbacks execute (all of them, in registration order)
   4. Control returns via configured RESUME_MODE:
      - THROW: exception thrown
      - RETURN: function returns with error code
      - JUMP: longjmp back to Init

Same Callback, Different Modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Register callback once
   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     printf("Callback: Recovery occurred\n");

     // Recreate data structures
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_define(GROUP_ID, DATA_ID, data, count, MPI_DOUBLE);

     if (fenix::role() == fenix::RECOVERED_RANK) {
       fenix::data::member_restore(GROUP_ID, DATA_ID);
     }
   });

   // This same callback works with THROW, RETURN, or JUMP mode

Part 6: Choosing the Right Resume Mode
---------------------------------------

Decision Matrix
^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 15 30 30 25

   * - Mode
     - Best For
     - Advantages
     - Disadvantages
   * - **THROW**
     - C++ applications
     - Clean, RAII-safe
     - C++ only
   * - **RETURN**
     - Small code sections, third-party C libraries
     - Fine-grained control
     - Complex, intrusive changes
   * - **JUMP**
     - Large C codebases
     - No error checking needed
     - Careful state management may be required

Common Patterns
^^^^^^^^^^^^^^^

**Pattern 1: Pure THROW (C++ applications)**

.. code-block:: cpp

   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   while (keep_running) {
     try {
       do_work();
       keep_running = false;
     } catch (const fenix::CommException& e) {
       // Handle recovery
     }
   }

**Pattern 2: Pure RETURN (small sections needing careful control)**

.. code-block:: cpp

   fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);

   int ret = critical_operation();
   if (ret == FENIX_ERROR_PROCESS_FAILURE) {
     // Handle recovery for this specific operation
   }

**Pattern 3: Pure JUMP (large C codebases)**

.. code-block:: c

   Fenix_Init(...);
   Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_JUMP);

   volatile int iteration = 0;
   for (iteration = 0; iteration < N; iteration++) {
     do_work();  // No error checking needed
   }

Resume Mode and Message Logging
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Resume mode is completely separate from message logging configuration. Any combination is valid:

- ``RESUME_THROW`` + any message logging mode
- ``RESUME_RETURN`` + any message logging mode
- ``RESUME_JUMP`` + any message logging mode

**Important:** If automatic message replay (``MLOG_RECOVERY_INLINE``) is enabled and succeeds, the MPI function will return success after the replay completes. The resume mode will not be triggered because no error occurred from the application's perspective (though callbacks will be invoked normally).

Next Steps
----------

📚 **Next Tutorial:**

- :doc:`04-message-logging` - Automatic MPI message replay (independent of resume mode)

🔗 **Related How-To Guides:**

- :doc:`/howto/choose-recovery-pattern` - Complete decision matrix

📖 **API Reference:**

- :cpp:func:`fenix::set_option` - Configure RESUME_MODE
- :c:macro:`Fenix_Init` - C API initialization with setjmp
- :cpp:class:`fenix::CommException` - Exception thrown in THROW mode

Summary
-------

**Key Concepts:**

- **Resume mode** controls how Fenix communicates failures to your application
- **RESUME_THROW**: Throws exceptions (C++, RAII-safe, clean control flow)
- **RESUME_RETURN**: Returns error codes (C/C++, for small sections needing fine-grained control, complex to use correctly)
- **RESUME_JUMP**: Uses longjmp (practical for large C codebases, requires volatile variables and callback-based cleanup)
- **Independent setting**: Resume mode is separate from message logging
- **Callbacks are universal**: Work the same way regardless of resume mode
- **Automatic updates**: The communicator pointer passed to ``fenix::init()`` is automatically updated after repair

The actual recovery process (communicator repair, callback execution, data restoration) is identical regardless of which resume mode you choose.
