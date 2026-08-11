Recovery with Callbacks
=======================

This guide shows you how to use callbacks for modern, maintainable fault tolerance. Callbacks work with **all resume modes** (THROW, RETURN, and JUMP) and execute before control returns to your application.

.. note::
   **Callbacks are universal**: They work the same way regardless of which resume mode you're using. This guide focuses on using callbacks with exception-based recovery (THROW mode), which is recommended for C++ applications, but the callback patterns apply to any resume mode.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Here's a minimal example with exception-based recovery and callbacks:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     // Enable exception-based resume mode
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     // Application state
     double my_data[1000];
     int iteration = 0;

     // Initialize data recovery
     const int GROUP_ID = 0, MEMBER_ID = 0;
     data::group_create(GROUP_ID);
     data::member_create(GROUP_ID, MEMBER_ID, my_data, 1000, MPI_DOUBLE);

     // Register recovery callback
     fenix::callback_register([&](MPI_Comm repaired, int err) {
       // Restore state after failure
       data::group_create(GROUP_ID);
       data::member_restore(GROUP_ID, MEMBER_ID, NULL, 0);
       printf("Rank %d recovered inline at iteration %d\n", rank, iteration);
     });

     // Main loop with exception handling
     for (int i = 0; i < 100; i++) {
       try {
         iteration = i;

         // Checkpoint periodically
         if (i % 10 == 0) {
           data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
           data::commit_barrier(GROUP_ID);
         }

         // MPI operations
         MPI_Allreduce(MPI_IN_PLACE, my_data, 1000,
                      MPI_DOUBLE, MPI_SUM, res_comm);

       } catch (fenix::CommException& e) {
         // Failure occurred, callback already restored state
         // Continue from current iteration
         printf("Rank %d continuing after recovery\n", rank);
         continue;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Why Use Non-Longjmp Resume Modes
---------------------------------

Advantages Over Longjmp-Based Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Type Safety and RAII**

Exception-based and return-based resume modes work correctly with C++ RAII patterns. Destructors are called properly, and resources are not leaked:

.. code-block:: cpp

   void process_data() {
     std::vector<double> buffer(1000000);  // RAII allocation
     std::unique_ptr<State> state(new State());
     std::lock_guard<std::mutex> lock(mtx);

     // If using longjmp, destructors won't be called - leak!
     // With THROW or RETURN resume modes, everything is cleaned up properly
     MPI_Allreduce(/* ... */);
   }

**No Undefined Behavior**

Variables don't mysteriously change values:

.. code-block:: cpp

   int counter = 0;
   // With longjmp, need: volatile int counter = 0;

   for (int i = 0; i < 100; i++) {
     counter++;  // Always increments correctly with THROW or RETURN modes
     // With JUMP mode (longjmp), counter may reset unpredictably
   }

**Cleaner Control Flow**

Recovery happens where the failure occurs, not at initialization:

.. code-block:: cpp

   // THROW or RETURN modes: continue from checkpoint
   for (int i = last_checkpoint; i < MAX_ITER; i++) {
     // ...
   }

   // JUMP mode (longjmp): restart entire loop from beginning
   for (int i = 0; i < MAX_ITER; i++) {
     // All work before checkpoint is lost
   }

Callback Basics
---------------

What Are Callbacks?
~~~~~~~~~~~~~~~~~~~

Recovery callbacks are functions you register with Fenix that are automatically called after a failure is detected and the communicator is repaired. They restore your application state regardless of which resume mode you're using (THROW, RETURN, or JUMP).

**Callback Signature:**

.. code-block:: cpp

   void callback(MPI_Comm repaired_comm, int mpi_error_code);

- ``repaired_comm``: The repaired resilient communicator
- ``mpi_error_code``: The MPI error that triggered recovery (e.g., ``MPI_ERR_PROC_FAILED``)

Registering a Callback
~~~~~~~~~~~~~~~~~~~~~~

Use ``fenix::callback_register`` with a lambda or function pointer:

.. code-block:: cpp

   // Lambda (most common)
   fenix::callback_register([&](MPI_Comm comm, int err) {
     // Recovery code here
   });

   // Function pointer
   void my_recovery(MPI_Comm comm, int err) {
     // Recovery code here
   }
   fenix::callback_register(my_recovery);

   // Member function
   class MyApp {
     void recover(MPI_Comm comm, int err) {
       // Recovery code
     }

     void setup() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         this->recover(comm, err);
       });
     }
   };

Callback Execution Order
~~~~~~~~~~~~~~~~~~~~~~~~

Callbacks are called in **reverse registration order** (LIFO - Last In, First Out):

.. code-block:: cpp

   fenix::callback_register([](MPI_Comm c, int e) {
     printf("First registered - runs THIRD\n");
   });

   fenix::callback_register([](MPI_Comm c, int e) {
     printf("Second registered - runs SECOND\n");
   });

   fenix::callback_register([](MPI_Comm c, int e) {
     printf("Third registered - runs FIRST\n");
   });

This order allows you to register foundational recovery first, then add higher-level recovery on top.

Lambda Capture Patterns
------------------------

Capture by Reference
~~~~~~~~~~~~~~~~~~~~

Most common pattern - capture application state by reference:

.. code-block:: cpp

   int iteration = 0;
   double data[1000];
   const int GROUP_ID = 0;

   fenix::callback_register([&](MPI_Comm comm, int err) {
     // Can access iteration and data
     data::member_restore(GROUP_ID, 0, data, 1000);
     printf("Recovered to iteration %d\n", iteration);
   });

**Warning:** The captured variables must still be in scope when the callback runs. Don't capture local variables from a function that will exit.

Capture by Value
~~~~~~~~~~~~~~~~

For small, copyable data:

.. code-block:: cpp

   const int GROUP_ID = 0;
   const int MEMBER_ID = 0;

   fenix::callback_register([=](MPI_Comm comm, int err) {
     // GROUP_ID and MEMBER_ID are copied
     data::member_restore(GROUP_ID, MEMBER_ID, NULL, 0);
   });

Capture Specific Variables
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Mix capture modes for flexibility:

.. code-block:: cpp

   int iteration = 0;
   double* data = new double[1000];
   const int GROUP_ID = 0;

   // Capture iteration by reference, GROUP_ID by value
   fenix::callback_register([&iteration, GROUP_ID](MPI_Comm comm, int err) {
     data::member_restore(GROUP_ID, 0, NULL, 0);
     printf("At iteration %d\n", iteration);
   });

Capture Class Members
~~~~~~~~~~~~~~~~~~~~~~

Use ``this`` to capture member variables:

.. code-block:: cpp

   class Simulation {
     int iteration_;
     std::vector<double> state_;
     const int group_id_;

   public:
     void register_recovery() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         // Access member variables via this->
         data::member_restore(group_id_, 0, state_.data(), state_.size());
         printf("Recovered to iteration %d\n", iteration_);
       });
     }
   };

Complete Example with Capture
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct AppState {
     int iteration;
     int rank;
     double convergence;
     std::vector<double> solution;
   };

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     AppState state;
     state.rank = rank;
     state.iteration = 0;
     state.solution.resize(10000, 0.0);

     const int GROUP_ID = 0;
     const int SOLUTION_ID = 0;
     const int STATE_ID = 1;

     data::group_create(GROUP_ID);
     data::member_create(GROUP_ID, SOLUTION_ID,
                        state.solution.data(), state.solution.size(), MPI_DOUBLE);
     data::member_create(GROUP_ID, STATE_ID,
                        &state, sizeof(AppState), MPI_BYTE);

     // Capture state by reference
     fenix::callback_register([&state, GROUP_ID, SOLUTION_ID, STATE_ID]
                              (MPI_Comm comm, int err) {
       data::group_create(GROUP_ID);
       data::member_restore(GROUP_ID, SOLUTION_ID, NULL, 0);
       data::member_restore(GROUP_ID, STATE_ID, NULL, 0);

       printf("Rank %d recovered to iteration %d, convergence=%e\n",
              state.rank, state.iteration, state.convergence);
     });

     // Main loop
     for (int i = state.iteration; i < 100; i++) {
       try {
         state.iteration = i;

         // Computation...
         MPI_Allreduce(MPI_IN_PLACE, state.solution.data(),
                      state.solution.size(), MPI_DOUBLE, MPI_SUM, res_comm);

         // Checkpoint
         if (i % 10 == 0) {
           data::member_store(GROUP_ID, SOLUTION_ID, SUBSET_FULL);
           data::member_store(GROUP_ID, STATE_ID, SUBSET_FULL);
           data::commit_barrier(GROUP_ID);
         }

       } catch (fenix::CommException& e) {
         continue;  // State restored by callback
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Multiple Callbacks
------------------

When to Use Multiple Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use multiple callbacks when you have:

- **Layered state**: Foundation data, then derived data
- **Multiple subsystems**: Each with its own state
- **Staged recovery**: Low-level then high-level operations

Example: Layered Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Level 1: Core data structures
   fenix::callback_register([&](MPI_Comm comm, int err) {
     printf("Restoring core data structures...\n");
     data::group_create(CORE_GROUP);
     data::member_restore(CORE_GROUP, ARRAY_MEMBER);
     data::member_restore(CORE_GROUP, MATRIX_MEMBER);
   });

   // Level 2: Derived state (depends on Level 1)
   fenix::callback_register([&](MPI_Comm comm, int err) {
     printf("Recomputing derived state...\n");
     compute_indices();  // Uses restored arrays
     update_metadata();  // Uses restored matrices
   });

   // Level 3: Communication setup (depends on Level 2)
   fenix::callback_register([&](MPI_Comm comm, int err) {
     printf("Re-establishing communication patterns...\n");
     setup_neighbor_ranks(comm);  // Uses updated metadata
   });

   // Execution order on failure:
   // 1. Level 3 callback runs first
   // 2. Level 2 callback runs second
   // 3. Level 1 callback runs third

Example: Multiple Subsystems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class Solver {
     void register_recovery() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         this->restore_solver_state();
       });
     }
   };

   class IOManager {
     void register_recovery() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         this->reopen_files();
       });
     }
   };

   class Profiler {
     void register_recovery() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         this->reset_timers();
       });
     }
   };

   int main(int argc, char** argv) {
     // ...
     Solver solver;
     IOManager io;
     Profiler prof;

     solver.register_recovery();
     io.register_recovery();
     prof.register_recovery();

     // All three callbacks will be called on failure
   }

Removing Callbacks
~~~~~~~~~~~~~~~~~~

Pop the most recently registered callback:

.. code-block:: cpp

   int callback_id = fenix::callback_register(my_callback);

   // Later, remove it
   fenix::callback_pop();

Pre-Recovery vs Post-Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Callbacks can run before or after communicator repair:

.. code-block:: cpp

   // Post-recovery (default): runs after communicator is repaired
   fenix::callback_register(my_callback, fenix::POST_RECOVERY);

   // Pre-recovery: runs before repair, comm may be broken
   fenix::callback_register(cleanup_callback, fenix::PRE_RECOVERY);

**Use PRE_RECOVERY for:**

- Logging failure information
- Cleaning up resources tied to failed ranks
- Recording diagnostics

**Use POST_RECOVERY for:**

- Restoring application state (most common)
- MPI operations (communicator is repaired)

Callback Lifecycle and Integration
-----------------------------------

Callback Registration Timing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Register callbacks after initial setup is complete:**

.. code-block:: cpp

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);
     fenix::init({.out_comm = &res_comm, .spares = 2});

     // Initialize data structures
     setup_data_groups();
     initialize_state();

     // Create initial checkpoint
     checkpoint_all();

     // NOW register callbacks (after state is initialized)
     fenix::callback_register([&](MPI_Comm comm, int err) {
       restore_state();
     });

     // Main application loop
     // ...
   }

**For recovered ranks:**

.. code-block:: cpp

   if (fenix::role() == fenix::RECOVERED_RANK) {
     // Restore state manually first time
     restore_state();

     // Then register callback for future failures
     fenix::callback_register([&](MPI_Comm comm, int err) {
       restore_state();
     });
   }

Integration with Application State
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Pattern: State Manager Class

.. code-block:: cpp

   class StateManager {
     int iteration_;
     std::vector<double> data_;
     const int group_id_;
     const int member_id_;

   public:
     StateManager(int group_id, int member_id)
       : group_id_(group_id), member_id_(member_id), iteration_(0) {
       data_.resize(10000);
     }

     void initialize() {
       data::group_create(group_id_);
       data::member_create(group_id_, member_id_,
                          data_.data(), data_.size(), MPI_DOUBLE);
       checkpoint();
     }

     void checkpoint() {
       data::member_store(group_id_, member_id_, SUBSET_FULL);
       data::commit_barrier(group_id_);
     }

     void restore() {
       data::group_create(group_id_);
       data::member_restore(group_id_, member_id_, NULL, 0);
     }

     void register_recovery_callback() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         this->restore();
         printf("Restored to iteration %d\n", iteration_);
       });
     }

     void advance() { iteration_++; }
     int iteration() const { return iteration_; }
     std::vector<double>& data() { return data_; }
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     StateManager state(0, 0);

     if (fenix::role() == fenix::INITIAL_RANK) {
       state.initialize();
     } else {
       state.restore();
     }

     state.register_recovery_callback();

     // Main loop
     for (int i = state.iteration(); i < 100; i++) {
       try {
         // Use state.data() for computations
         MPI_Allreduce(MPI_IN_PLACE, state.data().data(),
                      state.data().size(), MPI_DOUBLE, MPI_SUM, res_comm);

         state.advance();

         if (i % 10 == 0) {
           state.checkpoint();
         }

       } catch (fenix::CommException& e) {
         continue;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Complete Working Example
-------------------------

Iterative Stencil Solver
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <cmath>
   #include <signal.h>

   constexpr int N = 10000;
   constexpr int MAX_ITER = 100;
   constexpr int CHECKPOINT_FREQ = 10;

   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;
   constexpr int SOLUTION_ID = 1;

   struct State {
     int iteration;
     int rank;
     double convergence;
   };

   void inject_failure(int iteration) {
     int global_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

     // Inject failures at specific iterations
     if (global_rank == 2 && iteration == 25) {
       printf("Injecting failure on rank %d\n", global_rank);
       raise(SIGKILL);
     }
   }

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     // Initialize with THROW or RETURN resume mode
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Application state
     State state;
     std::vector<double> solution(N, 0.0);
     std::vector<double> residual(N, 1.0);

     // Initialize or recover
     if (fenix::role() == fenix::INITIAL_RANK) {
       state.rank = rank;
       state.iteration = 0;
       state.convergence = 1.0;

       // Create data group and members
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, STATE_ID,
                          &state, sizeof(State), MPI_BYTE);
       data::member_create(GROUP_ID, SOLUTION_ID,
                          solution.data(), N, MPI_DOUBLE);

       // Initial checkpoint
       data::member_store(GROUP_ID, STATE_ID, SUBSET_FULL);
       data::member_store(GROUP_ID, SOLUTION_ID, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);

       printf("Rank %d initialized\n", rank);

     } else {
       // Recovered rank - restore from checkpoint
       data::group_create(GROUP_ID);
       data::member_define(GROUP_ID, STATE_ID,
                          &state, sizeof(State), MPI_BYTE);
       data::member_define(GROUP_ID, SOLUTION_ID,
                          solution.data(), N, MPI_DOUBLE);

       data::member_restore(GROUP_ID, STATE_ID);
       data::member_restore(GROUP_ID, SOLUTION_ID);

       printf("Rank %d recovered to iteration %d\n", rank, state.iteration);
     }

     // Register recovery callback for inline failures
     fenix::callback_register(
       [&state, &solution, &residual](MPI_Comm comm, int err) {
         data::group_create(GROUP_ID);
         data::member_restore(GROUP_ID, STATE_ID, NULL, 0);
         data::member_restore(GROUP_ID, SOLUTION_ID, NULL, 0);

         printf("Rank %d continuing inline at iteration %d\n",
                state.rank, state.iteration);
       }
     );

     // Main solver loop
     for (int i = state.iteration; i < MAX_ITER; i++) {
       try {
         inject_failure(i);

         state.iteration = i;

         // Solver iteration
         for (int j = 0; j < N; j++) {
           double old = solution[j];
           solution[j] = solution[j] + 0.1 * residual[j];
           residual[j] = residual[j] - 0.1 * (solution[j] - old);
         }

         // Check convergence
         double local_norm = 0.0;
         for (int j = 0; j < N; j++) {
           local_norm += residual[j] * residual[j];
         }

         MPI_Allreduce(&local_norm, &state.convergence, 1,
                      MPI_DOUBLE, MPI_SUM, res_comm);
         state.convergence = std::sqrt(state.convergence);

         if (rank == 0 && i % 10 == 0) {
           printf("Iteration %d: convergence = %e\n", i, state.convergence);
         }

         // Checkpoint periodically
         if (i % CHECKPOINT_FREQ == 0) {
           data::member_store(GROUP_ID, STATE_ID, SUBSET_FULL);
           data::member_store(GROUP_ID, SOLUTION_ID, SUBSET_FULL);
           data::commit_barrier(GROUP_ID);
         }

         // Check convergence
         if (state.convergence < 1e-6) {
           if (rank == 0) {
             printf("Converged at iteration %d\n", i);
           }
           break;
         }

       } catch (fenix::CommException& e) {
         // Failure occurred, callback already restored state
         printf("Rank %d recovered from failure, continuing\n", rank);
         continue;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Troubleshooting
---------------

**Problem: Segfault in callback**

- Ensure captured variables are still in scope
- Check that pointers are valid when callback runs
- Verify data structures are initialized before callback registration

**Problem: Callback doesn't restore all state**

- Add debug prints to verify callback is called
- Check that all necessary members are restored
- Ensure ``member_restore`` calls match checkpoint members

**Problem: Exception not caught**

- Verify ``RESUME_MODE`` is set to ``RESUME_THROW``
- Check that try-catch blocks surround MPI operations
- Ensure ``fenix::CommException`` is being caught (not base ``std::exception``)

**Problem: Wrong values after recovery**

- Verify checkpoint was committed before failure
- Check that correct snapshot timestamp is used in restore
- Ensure callbacks use ``member_restore`` with ``NULL, 0`` to use the buffer from ``member_define`` or ``member_create``

**Problem: Callback called multiple times**

- Each failure triggers all registered callbacks
- Remove old callbacks with ``callback_pop`` if no longer needed
- Check for cascading failures (see :doc:`handle-cascading-failures`)

**Problem: Deadlock in callback**

- Avoid collective operations in callbacks unless all ranks participate
- Use ``NULL, 0`` for buffer/count in ``member_restore`` to use the buffer from ``member_define`` or ``member_create``
- Check that callback doesn't wait for failed ranks

**Problem: When should I use member_define vs member_create?**

- **Initial ranks**: Use ``member_create`` for fail-fast creation. Callbacks can use INPLACE restore.
- **Recovered ranks**: Use ``member_define`` to specify buffers before first restore (idempotent, retry-safe).
- **Custom serializers**: Always use ``member_fdefine`` or ``member_define`` + ``member_attribute_set`` to specify serializer.
- **Both functions**: Save buffer pointers for use with INPLACE restore (``NULL, 0``).

See :doc:`/api/data-recovery` for detailed comparison.

See Also
--------

- :doc:`choose-recovery-pattern` - Choosing between inline and longjmp
- :doc:`handle-cascading-failures` - Handle failures during recovery
- :doc:`checkpoint-data` - How to checkpoint application state
- :doc:`test-locally` - Testing your callbacks
- :doc:`/api/process-recovery` - API reference for callbacks
- :doc:`/guides/process-recovery` - Conceptual guide to recovery
