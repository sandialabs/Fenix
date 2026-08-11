THROW Resume Mode
=================

The THROW resume mode (FENIX_RESUME_THROW) uses C++ exceptions for clean, RAII-safe recovery.

.. _throw-resume-mode:

Call Stack with Exception Unwinding
------------------------------------

.. graphviz::
   :caption: THROW Resume Mode - Clean Unwinding

   digraph throw_stack {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       normal [label="try block:\napplication_loop()\n→ MPI_Allreduce()", fillcolor=lightgreen];

       error [label="Error handler:\n• Revoke comm\n• Repair comm\n• Call callbacks\n• throw CommException", fillcolor=orange];

       unwind [label="Stack Unwinding:\n• Destructors called\n• RAII cleanup\n• Resources freed", fillcolor=lightyellow];

       catch [label="catch (CommException&)\nHandle recovery", fillcolor=lightgreen];

       normal -> error [label="Failure"];
       error -> unwind [label="throw"];
       unwind -> catch [label="Caught"];
   }

Key Characteristics
-------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Aspect
     - Behavior
   * - **Stack unwinding**
     - ✅ Clean unwinding
   * - **Destructors**
     - ✅ All called automatically
   * - **Local variables**
     - ✅ Properly cleaned up
   * - **Resource cleanup**
     - ✅ Automatic (RAII)
   * - **RAII compatibility**
     - ✅ Perfect
   * - **Language**
     - C++ only
   * - **Best for**
     - Modern C++ applications

RAII Safety Example
-------------------

.. code-block:: cpp

   void application_loop(MPI_Comm comm) {
     std::unique_ptr<Data> ptr(new Data(1000));  // Allocated
     std::lock_guard<std::mutex> lock(mtx);      // Lock acquired
     std::vector<double> buffer(1000);           // Vector allocated

     try {
       MPI_Allreduce(buffer.data(), /*...*/, comm);  // May throw!

     } catch (fenix::CommException& e) {
       // Exception caught
       // Resources automatically cleaned:
       // 1. lock_guard releases mutex  ✅
       // 2. vector frees buffer        ✅
       // 3. unique_ptr deletes Data    ✅
     }
   }

Complete C++ Example
--------------------

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <memory>

   struct AppState {
     int iteration = 0;
     std::vector<double> data;
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix with exception mode
     MPI_Comm comm;
     fenix::init({
       .out_comm = &comm,
       .spares = 2,
       .resume_mode = fenix::RESUME_THROW
     });

     AppState state;
     state.data.resize(1000);

     // Register recovery callback
     fenix::callback_register([&](MPI_Comm c, int err) {
       // Restore state from checkpoint
       fenix::data::member_restore(GROUP_ID, MEMBER_ID);
       std::cout << "Restored to iteration " 
                 << state.iteration << std::endl;
     });

     // Initial setup
     if (fenix::role() == FENIX_ROLE_INITIAL_RANK) {
       state.iteration = 0;
       // Setup data recovery...
     }

     // Application loop with exception handling
     while (state.iteration < 100) {
       try {
         for (; state.iteration < 100; state.iteration++) {
           // Work...
           for (auto& val : state.data) {
             val = val * 2.0;
           }

           // MPI operation - may throw CommException
           MPI_Allreduce(MPI_IN_PLACE, state.data.data(), 1000,
                        MPI_DOUBLE, MPI_SUM, comm);

           // Checkpoint
           if (state.iteration % 10 == 0) {
             fenix::data::member_store(GROUP_ID, MEMBER_ID,
                                      fenix::data::SUBSET_FULL);
             fenix::data::commit(GROUP_ID);
           }
         }
         break;  // Completed successfully

       } catch (fenix::CommException& e) {
         // Recovery happened, continue from checkpoint
         std::cout << "Recovered from failure at iteration "
                   << state.iteration << std::endl;
         // Loop continues from current state.iteration
       }
     }

     fenix::finalize();
     MPI_Finalize();
     return 0;
   }

Advantages
----------

✅ **Benefits:**

- Perfect RAII safety
- Clean exception semantics
- Automatic resource cleanup
- Modern C++ idioms
- Clear error handling
- No manual cleanup needed

When to Use
-----------

✅ **Strongly recommended for:**

- Modern C++ applications
- Code using smart pointers
- Code using RAII containers
- Code with complex resource management
- Production C++ systems

❌ **Cannot use for:**

- C applications (C++ only)

.. seealso::

   * :doc:`03-inline-recovery` - RETURN resume mode (C++ alternative without exceptions)
   * :doc:`02-longjmp-recovery` - JUMP resume mode (why not to use in C++)
   * :doc:`12-decision-recovery-pattern` - Resume mode selection guide
