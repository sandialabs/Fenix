JUMP Resume Mode
================

The JUMP resume mode (FENIX_RESUME_JUMP) is the default recovery mode that uses setjmp/longjmp to return control to Fenix_Init after a failure.

.. _jump-resume-mode:

.. warning::
   **NOT recommended for C++!** Longjmp bypasses destructors and causes undefined behavior with RAII objects.

Call Stack Behavior
-------------------

.. graphviz::
   :caption: JUMP Resume Mode Stack Jump

   digraph jump_stack {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       normal [label="Normal Execution Stack:\nmain() → loop() → MPI_Allreduce()\n→ error_handler()", fillcolor=lightgreen];

       jump [label="longjmp() DESTROYS stack!", shape=diamond, fillcolor=red, fontcolor=white];

       reset [label="Stack Reset:\nmain() → Fenix_Init()\n(back to setjmp point)", fillcolor=lightcoral];

       normal -> jump [label="Failure"];
       jump -> reset [label="Jump!"];
   }

Key Characteristics
-------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Aspect
     - Behavior
   * - **Stack unwinding**
     - ❌ NO - stack is destroyed
   * - **Destructors**
     - ❌ NOT called
   * - **Local variables**
     - ⚠️ Undefined (use volatile)
   * - **Resource cleanup**
     - ❌ Manual cleanup required
   * - **RAII compatibility**
     - ❌ NO - undefined behavior
   * - **Ease of use**
     - ✅ Easy (minimal code changes)
   * - **Performance**
     - ⚠️ Poor (restart from scratch)

Code Example
------------

.. code-block:: c

   int main(int argc, char** argv) {
     // Variables that survive longjmp: use volatile!
     volatile int num_failures = 0;
     volatile int checkpoint_iteration = 0;

     MPI_Init(&argc, &argv);

     int role, error;
     MPI_Comm comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &comm,  // ◄── setjmp() here
                &argc, &argv, 2, &error);

     // ◄── longjmp() returns here after failure

     if (role == FENIX_ROLE_RECOVERED_RANK || 
         role == FENIX_ROLE_SURVIVOR_RANK) {
       num_failures++;
       printf("Recovery #%d\n", num_failures);
     }

     // Must reinitialize everything
     int current_iteration = checkpoint_iteration;

     // Application loop
     for (int i = current_iteration; i < 100; i++) {
       current_iteration = i;

       if (i % 10 == 0) {
         checkpoint_iteration = i;  // volatile survives
       }

       MPI_Allreduce(/*...*/, comm);  // May jump back!
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Warnings
--------

.. danger::
   **C++ Issues:**
   
   - ``std::vector``, ``std::string`` → Memory leaks
   - ``std::unique_ptr`` → Resource leaks
   - ``std::lock_guard`` → Deadlocks (mutex not released)
   - Destructors NOT called → Undefined behavior

When to Use
-----------

✅ **Good for:**

- Simple C applications
- Prototypes and testing
- Legacy code with minimal changes
- Applications that can restart cheaply

❌ **Avoid when:**

- Using C++ (use inline or exception instead)
- Using RAII patterns
- Application state is expensive to reinitialize
- Need to preserve work across failures

.. seealso::

   * :doc:`03-inline-recovery` - RETURN resume mode (better for C)
   * :doc:`04-exception-recovery` - THROW resume mode (best for C++)
   * :doc:`12-decision-recovery-pattern` - Choosing resume modes
