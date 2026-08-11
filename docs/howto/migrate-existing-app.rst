How to Migrate Your MPI Application to Fenix
=============================================

This guide shows you how to add fault tolerance to an existing MPI application using Fenix. We'll cover both quick minimal migrations and comprehensive fault-tolerant implementations.

.. tip::
   **Time Required:** 30 minutes for basic migration, 2-4 hours for full data recovery integration

.. contents:: On This Page
   :local:
   :depth: 2

----

Is Migration Right for Your Application?
-----------------------------------------

Before you start, consider:

**Good Candidates for Fenix:**

* Long-running scientific simulations (hours to days)
* Applications with iterative algorithms
* Programs running on unreliable hardware
* Jobs using large resource allocations (high cost of failure)
* Applications with manageable state that can be checkpointed

**May Not Need Fenix:**

* Short-running jobs (minutes)
* Embarrassingly parallel workloads with independent tasks
* Applications already using system-level checkpointing
* Programs with minimal communication patterns

**Key Question:** Can your application benefit from continuing through rank failures without full restart?

----

Migration Decision Tree
-----------------------

Choose your migration path:

.. code-block:: text

   Do you need to preserve application state across failures?
   │
   ├─ NO: Minimal Migration (Process Recovery Only)
   │      → 15-30 minutes, minimal code changes
   │      → Your app restarts from beginning after recovery
   │
   └─ YES: Full Migration (Process + Data Recovery)
          │
          ├─ Simple state: Full Migration with Checkpointing
          │  → 2-4 hours, moderate code changes
          │  → Checkpoint/restore application state
          │
          └─ Complex communication: Add Message Logging
             → Additional 1-2 hours
             → Automatic message replay for localized recovery

**This Guide Covers:** Both minimal and full migration paths with concrete examples.

----

Quick Migration: Process Recovery Only
---------------------------------------

This is the fastest path to fault tolerance. Your application will automatically recover from failures, but will restart its computation from the beginning.

**Time:** 15-30 minutes | **Difficulty:** Easy | **Code Changes:** Minimal

Step 1: Add Fenix Headers
~~~~~~~~~~~~~~~~~~~~~~~~~~

Replace your standard MPI include with Fenix:

**C++ Applications (Recommended):**

.. code-block:: cpp

   // Before
   #include <mpi.h>

   // After
   #include <mpi.h>
   #include <fenix.hpp>  // Modern C++ API

**C Applications:**

.. code-block:: c

   // Before
   #include <mpi.h>

   // After
   #include <mpi.h>
   #include <fenix.h>  // C API

Step 2: Initialize Fenix
~~~~~~~~~~~~~~~~~~~~~~~~~

Add Fenix initialization right after MPI_Init:

**Modern C++ API (Recommended):**

.. code-block:: cpp

   int main(int argc, char** argv) {
     // Keep MPI_Init
     MPI_Init(&argc, &argv);

     // Add Fenix initialization
     MPI_Comm fenix_comm;
     fenix::init({
       .out_comm = &fenix_comm,  // resilient communicator
       .spares = 2               // Number of spare ranks
     });

     // Check for errors
     if (fenix::error() != FENIX_SUCCESS) {
       fprintf(stderr, "Fenix initialization failed\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
     }

     // Your application code...
   }

**C API:**

.. code-block:: c

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     int fenix_role;
     MPI_Comm fenix_comm;
     int error;

     Fenix_Init(&fenix_role, MPI_COMM_WORLD, &fenix_comm,
                &argc, &argv, 2, &error);

     if (error != FENIX_SUCCESS) {
       fprintf(stderr, "Fenix initialization failed\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
     }

     // Your application code...
   }

.. important::
   **Spare Ranks:** You need to request spare ranks for recovery. A good rule of thumb is 10-20% of your active ranks, minimum 1-2 spares.

Step 3: Replace MPI_COMM_WORLD
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Replace all uses of ``MPI_COMM_WORLD`` with your resilient communicator:

.. code-block:: cpp

   // Before
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   MPI_Send(buffer, count, MPI_INT, dest, tag, MPI_COMM_WORLD);

   // After
   MPI_Comm_rank(fenix_comm, &rank);
   MPI_Comm_size(fenix_comm, &size);
   MPI_Send(buffer, count, MPI_INT, dest, tag, fenix_comm);

.. tip::
   **Quick Find & Replace:** Search for ``MPI_COMM_WORLD`` and replace with your resilient communicator name throughout your codebase.

Step 4: Add Fenix Finalization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before MPI_Finalize, add Fenix cleanup:

.. code-block:: cpp

   // Before
   MPI_Finalize();
   return 0;

   // After
   Fenix_Finalize();
   MPI_Finalize();
   return 0;

Step 5: Update Build System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add Fenix to your compilation:

**Manual Compilation:**

.. code-block:: bash

   # Before
   mpicxx -o myapp myapp.cpp

   # After
   mpicxx -o myapp myapp.cpp \
     -I/path/to/fenix/include \
     -L/path/to/fenix/lib -lfenix

**CMake:**

.. code-block:: cmake

   # Add to CMakeLists.txt
   find_package(fenix REQUIRED)
   target_link_libraries(myapp fenix)

Step 6: Run with Fault Tolerance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Enable fault tolerance when running:

.. code-block:: bash

   # Before
   mpiexec -n 16 ./myapp

   # After - launch with 2 spare ranks
   mpiexec --with-ft mpi -n 18 ./myapp

.. important::
   The ``--with-ft mpi`` flag is **required** for fault tolerance. Without it, failures will still abort your application.

Complete Minimal Example
~~~~~~~~~~~~~~~~~~~~~~~~~

Here's a complete before/after comparison:

**Before (Standard MPI):**

.. code-block:: cpp

   #include <mpi.h>
   #include <stdio.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     int rank, size;
     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
     MPI_Comm_size(MPI_COMM_WORLD, &size);

     // Ring communication
     int msg = rank;
     if (rank == 0) {
       MPI_Send(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
       MPI_Recv(&msg, 1, MPI_INT, size-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     } else {
       MPI_Recv(&msg, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
       msg++;
       MPI_Send(&msg, 1, MPI_INT, (rank+1)%size, 0, MPI_COMM_WORLD);
     }

     printf("Rank %d final message: %d\n", rank, msg);

     MPI_Finalize();
     return 0;
   }

**After (With Fenix - Minimal Changes):**

.. code-block:: cpp

   #include <mpi.h>
   #include <fenix.hpp>  // Added
   #include <stdio.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Added Fenix initialization
     MPI_Comm fenix_comm;
     fenix::init({.out_comm = &fenix_comm, .spares = 2});
     if (fenix::error() != FENIX_SUCCESS) {
       fprintf(stderr, "Fenix init failed\n");
       return 1;
     }

     int rank, size;
     MPI_Comm_rank(fenix_comm, &rank);      // resilient communicator
     MPI_Comm_size(fenix_comm, &size);      // resilient communicator

     // Ring communication - use resilient communicator
     int msg = rank;
     if (rank == 0) {
       MPI_Send(&msg, 1, MPI_INT, 1, 0, fenix_comm);
       MPI_Recv(&msg, 1, MPI_INT, size-1, 0, fenix_comm, MPI_STATUS_IGNORE);
     } else {
       MPI_Recv(&msg, 1, MPI_INT, rank-1, 0, fenix_comm, MPI_STATUS_IGNORE);
       msg++;
       MPI_Send(&msg, 1, MPI_INT, (rank+1)%size, 0, fenix_comm);
     }

     printf("Rank %d final message: %d\n", rank, msg);

     Fenix_Finalize();  // Added
     MPI_Finalize();
     return 0;
   }

That's it for minimal migration! Your application now survives rank failures and automatically restarts.

**Limitations of Minimal Migration:**

* Application restarts computation from beginning after recovery
* All state is lost when ranks fail
* Good for stateless or short-iteration applications

For preserving state across failures, continue to the next section.

----

Full Migration: Adding Data Recovery
-------------------------------------

This section adds checkpoint/restore capabilities so your application can continue from where it left off after a failure.

**Time:** 2-4 hours | **Difficulty:** Moderate | **Prerequisites:** Completed minimal migration above

Step 1: Choose Recovery Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix supports two recovery patterns:

**Pattern A: Exception-Based (Recommended for C++)**

* Modern, clean control flow
* Uses try/catch for recovery
* No undefined behavior
* Easier to reason about

**Pattern B: Longjmp-Based (Default, Simpler)**

* Automatically jumps back to Fenix_Init
* Mimics traditional restart
* May have undefined behavior with some compilers
* Works with C and C++

**We recommend Pattern A for new C++ applications.** This guide shows both.

Step 2: Initialize with Data Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Pattern A: Exception-Based (C++):**

.. code-block:: cpp

   #include <mpi.h>
   #include <fenix.hpp>
   #include <stdio.h>

   // Your application state
   struct AppState {
     int iteration;
     double* grid;
     int grid_size;
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm fenix_comm;
     fenix::init({.out_comm = &fenix_comm, .spares = 2});

     // Enable exception-based recovery
     fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

     int rank;
     MPI_Comm_rank(fenix_comm, &rank);

     // Allocate application state
     AppState state;
     state.grid_size = 1000;
     state.grid = new double[state.grid_size];
     state.iteration = 0;

     // Wrap application in try-catch for recovery
     try {
       run_application(fenix_comm, state);
     } catch (fenix::CommException& e) {
       // Recovery happened, application will continue
       printf("Rank %d recovered from failure\n", rank);
     }

     delete[] state.grid;
     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Pattern B: Longjmp-Based (C or C++):**

.. code-block:: cpp

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm fenix_comm;
     int fenix_role;

     // Fenix_Init returns here after recovery
     fenix::init({
       .role = &fenix_role,
       .out_comm = &fenix_comm,
       .spares = 2
     });

     int rank;
     MPI_Comm_rank(fenix_comm, &rank);

     // Enable exception-based recovery
     fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

     // Check if this is initial run or recovery
     if (fenix_role == fenix::INITIAL_RANK) {
       // First time - initialize state
       initialize_state();
     } else {
       // Recovered from failure - restore state
       restore_state();
       printf("Rank %d recovered from failure\n", rank);
     }

     // Application runs normally
     run_application(fenix_comm);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Step 3: Create Data Group and Members
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A **data group** contains **data members** to checkpoint together:

.. code-block:: cpp

   using namespace fenix::data;

   const int GROUP_ID = 0;
   const int STATE_MEMBER = 0;
   const int GRID_MEMBER = 1;

   void setup_checkpointing(MPI_Comm comm, AppState& state) {
     // Create group for this communicator
     group_create(GROUP_ID, {.comm = comm});

     // Register state variables to checkpoint
     member_create(GROUP_ID, STATE_MEMBER,
                   &state.iteration, 1, MPI_INT);

     member_create(GROUP_ID, GRID_MEMBER,
                   state.grid, state.grid_size, MPI_DOUBLE);
   }

.. note::
   **Data Members:** Can be any MPI-serializable data: scalars, arrays, structs with MPI datatypes.
   **Groups:** Organize related data. All members in a group are checkpointed atomically.

Step 4: Checkpoint Your Data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Store data at regular intervals:

.. code-block:: cpp

   void checkpoint(int iteration) {
     using namespace fenix::data;

     if (iteration % CHECKPOINT_FREQUENCY == 0) {
       // Store all data members
       member_store(GROUP_ID, STATE_MEMBER, SUBSET_FULL);
       member_store(GROUP_ID, GRID_MEMBER, SUBSET_FULL);

       // Commit checkpoint - collective operation
       commit_barrier(GROUP_ID);
     }
   }

.. tip::
   **Checkpoint Frequency:** Balance overhead vs recovery time.

   * More frequent: Faster recovery, higher overhead
   * Less frequent: Lower overhead, more work lost
   * Start with every 10-20 iterations and tune based on profiling

Step 5: Restore Data After Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Pattern A: Exception-Based:**

.. code-block:: cpp

   void run_application(MPI_Comm comm, AppState& state) {
     using namespace fenix::data;

     // Set up checkpointing
     if (fenix::role() == fenix::INITIAL_RANK) {
       setup_checkpointing(comm, state);
       checkpoint(0);  // Initial checkpoint
     } else {
       // Recovered rank - restore from checkpoint
       group_create(GROUP_ID, {.comm = comm});
       member_define(GROUP_ID, STATE_MEMBER, &state.iteration, 1, MPI_INT);
       member_define(GROUP_ID, GRID_MEMBER, state.grid, state.grid_size, MPI_DOUBLE);

       member_restore(GROUP_ID, STATE_MEMBER);
       member_restore(GROUP_ID, GRID_MEMBER);
     }

     // Register callback for inline recovery
     fenix::callback_register([&](MPI_Comm repaired, int err) {
       // This runs when another failure happens
       group_create(GROUP_ID, {.comm = repaired});
       member_restore(GROUP_ID, STATE_MEMBER);
       member_restore(GROUP_ID, GRID_MEMBER);
     });

     // Main application loop
     for (int i = state.iteration; i < MAX_ITERATIONS; i++) {
       state.iteration = i;

       // Do computation
       compute_iteration(comm, state);

       // Checkpoint periodically
       checkpoint(i);
     }
   }

**Pattern B: Longjmp-Based:**

.. code-block:: cpp

   void restore_state() {
     using namespace fenix::data;

     // Recreate group
     group_create(GROUP_ID, {.comm = fenix_comm});

     // Define member locations
     member_define(GROUP_ID, STATE_MEMBER, &state.iteration, 1, MPI_INT);
     member_define(GROUP_ID, GRID_MEMBER, state.grid, state.grid_size, MPI_DOUBLE);

     // Restore from last checkpoint
     member_restore(GROUP_ID, STATE_MEMBER);
     member_restore(GROUP_ID, GRID_MEMBER);
   }

Step 6: Update Application Loop
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Modify your main loop to support resumption:

.. code-block:: cpp

   // Before
   for (int i = 0; i < MAX_ITERATIONS; i++) {
     compute_iteration(comm, i);
   }

   // After - resume from checkpoint
   for (int i = state.iteration; i < MAX_ITERATIONS; i++) {
     state.iteration = i;
     compute_iteration(comm, state);

     if (i % CHECKPOINT_FREQUENCY == 0) {
       checkpoint(i);
     }
   }

Complete Example: Stencil Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here's a complete example migrating a 1D stencil computation:

.. code-block:: cpp

   #include <mpi.h>
   #include <fenix.hpp>
   #include <vector>
   #include <algorithm>

   const int GROUP_ID = 0;
   const int ITERATION_MEMBER = 0;
   const int GRID_MEMBER = 1;
   const int CHECKPOINT_FREQ = 10;

   struct State {
     int iteration = 0;
     std::vector<double> grid;
     int local_size;
   };

   void stencil_update(MPI_Comm comm, State& state) {
     int rank, size;
     MPI_Comm_rank(comm, &rank);
     MPI_Comm_size(comm, &size);

     int left = (rank - 1 + size) % size;
     int right = (rank + 1) % size;

     // Exchange boundaries
     double left_boundary, right_boundary;
     MPI_Sendrecv(&state.grid[0], 1, MPI_DOUBLE, left, 0,
                  &right_boundary, 1, MPI_DOUBLE, right, 0,
                  comm, MPI_STATUS_IGNORE);
     MPI_Sendrecv(&state.grid[state.local_size-1], 1, MPI_DOUBLE, right, 1,
                  &left_boundary, 1, MPI_DOUBLE, left, 1,
                  comm, MPI_STATUS_IGNORE);

     // Update interior points
     std::vector<double> new_grid = state.grid;
     for (int i = 1; i < state.local_size - 1; i++) {
       new_grid[i] = 0.25 * (state.grid[i-1] + 2*state.grid[i] + state.grid[i+1]);
     }

     // Update boundaries
     new_grid[0] = 0.25 * (left_boundary + 2*state.grid[0] + state.grid[1]);
     new_grid[state.local_size-1] = 0.25 *
       (state.grid[state.local_size-2] + 2*state.grid[state.local_size-1] + right_boundary);

     state.grid = new_grid;
   }

   int main(int argc, char** argv) {
     using namespace fenix::data;

     MPI_Init(&argc, &argv);

     MPI_Comm fenix_comm;
     fenix::init({.out_comm = &fenix_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

     int rank, size;
     MPI_Comm_rank(fenix_comm, &rank);
     MPI_Comm_size(fenix_comm, &size);

     State state;
     state.local_size = 100;
     state.grid.resize(state.local_size, rank * 1.0);

     try {
       // Initialize or restore
       if (fenix::role() == fenix::INITIAL_RANK) {
         group_create(GROUP_ID, {.comm = fenix_comm});
         member_create(GROUP_ID, ITERATION_MEMBER, &state.iteration, 1, MPI_INT);
         member_create(GROUP_ID, GRID_MEMBER,
                      state.grid.data(), state.local_size, MPI_DOUBLE);

         member_store(GROUP_ID, ITERATION_MEMBER, SUBSET_FULL);
         member_store(GROUP_ID, GRID_MEMBER, SUBSET_FULL);
         commit_barrier(GROUP_ID);
       } else {
         group_create(GROUP_ID, {.comm = fenix_comm});
         member_define(GROUP_ID, ITERATION_MEMBER, &state.iteration, 1, MPI_INT);
         member_define(GROUP_ID, GRID_MEMBER,
                      state.grid.data(), state.local_size, MPI_DOUBLE);

         member_restore(GROUP_ID, ITERATION_MEMBER);
         member_restore(GROUP_ID, GRID_MEMBER);

         printf("Rank %d restored to iteration %d\n", rank, state.iteration);
       }

       // Recovery callback for inline recovery
       fenix::callback_register([&](MPI_Comm repaired, int err) {
         group_create(GROUP_ID, {.comm = repaired});
         // member_define to update buffer pointers (vector may have reallocated)
         member_define(GROUP_ID, ITERATION_MEMBER, &state.iteration, 1, MPI_INT);
         member_define(GROUP_ID, GRID_MEMBER,
                      state.grid.data(), state.local_size, MPI_DOUBLE);
         member_restore(GROUP_ID, ITERATION_MEMBER);
         member_restore(GROUP_ID, GRID_MEMBER);
         printf("Rank %d continuing at iteration %d\n", rank, state.iteration);
       });

       // Main computation loop
       const int MAX_ITER = 100;
       for (int i = state.iteration; i < MAX_ITER; i++) {
         state.iteration = i;

         stencil_update(fenix_comm, state);

         // Checkpoint periodically
         if (i % CHECKPOINT_FREQ == 0) {
           member_store(GROUP_ID, ITERATION_MEMBER, SUBSET_FULL);
           member_store(GROUP_ID, GRID_MEMBER, SUBSET_FULL);
           commit_barrier(GROUP_ID);
         }
       }

     } catch (fenix::CommException& e) {
       printf("Rank %d caught exception, recovered\n", rank);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

----

Advanced: Adding Message Logging
---------------------------------

For applications with complex communication patterns, message logging can provide automatic message replay during recovery.

**When to Use Message Logging:**

* Complex point-to-point communication patterns
* Non-deterministic message ordering
* Want to minimize rollback after failures

**Time:** +1-2 hours | **Difficulty:** Advanced

Step 1: Create Message Log
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.hpp>

   const int MLOG_ID = 0;
   const int MLOG_GROUP = 0;
   const int MLOG_MEMBER = 2;
   const int MLOG_REGIONS = 10;  // Keep last 10 regions

   // After Fenix init
   fenix::mlog::create(MLOG_ID, fenix_comm, MLOG_REGIONS);

Step 2: Define Message Log Regions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   for (int i = state.iteration; i < MAX_ITERATIONS; i++) {
     // Start message log region
     fenix::mlog::begin_region(MLOG_ID, i);

     // All MPI communication in this region is logged
     compute_iteration(fenix_comm, state);

     if (i % CHECKPOINT_FREQ == 0) {
       checkpoint(i);
     }
   }

Step 3: Checkpoint Message Log State
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Add message log member to checkpoint
   fenix::mlog::create_data_member(MLOG_ID, MLOG_GROUP, MLOG_MEMBER);

   // Store with other members
   fenix::data::member_store(MLOG_GROUP, SUBSET_FULL);

Step 4: Enable Automatic Replay
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Activate message logging
   fenix::mlog::activate(MLOG_ID);

   // Enable automatic replay after recovery
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

.. note::
   With automatic replay enabled, surviving ranks will automatically resend messages to recovered ranks, and recovered ranks will skip ahead to the current iteration.

----

Before/After Comparison: Common Patterns
----------------------------------------

Point-to-Point Communication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Before:**

.. code-block:: cpp

   MPI_Send(buffer, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
   MPI_Recv(buffer, count, MPI_INT, source, tag, MPI_COMM_WORLD, &status);

**After:**

.. code-block:: cpp

   // Use resilient communicator - Fenix handles failures automatically
   MPI_Send(buffer, count, MPI_INT, dest, tag, fenix_comm);
   MPI_Recv(buffer, count, MPI_INT, source, tag, fenix_comm, &status);

Collective Operations
~~~~~~~~~~~~~~~~~~~~~

**Before:**

.. code-block:: cpp

   MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

**After:**

.. code-block:: cpp

   // Use resilient communicator
   MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, fenix_comm);

   // Fenix automatically restarts collective if a rank fails

Non-Blocking Operations
~~~~~~~~~~~~~~~~~~~~~~~

**Before:**

.. code-block:: cpp

   MPI_Request requests[2];
   MPI_Isend(send_buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD, &requests[0]);
   MPI_Irecv(recv_buf, count, MPI_INT, source, tag, MPI_COMM_WORLD, &requests[1]);
   MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);

**After:**

.. code-block:: cpp

   MPI_Request requests[2];
   MPI_Isend(send_buf, count, MPI_INT, dest, tag, fenix_comm, &requests[0]);
   MPI_Irecv(recv_buf, count, MPI_INT, source, tag, fenix_comm, &requests[1]);

   // Wait may return error if failure occurs
   int err = MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);
   // Fenix handles recovery automatically

Iterative Algorithms
~~~~~~~~~~~~~~~~~~~~

**Before:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iterations; iter++) {
     // Computation
     compute_step(MPI_COMM_WORLD, data);

     // Check convergence
     MPI_Allreduce(&local_error, &global_error, 1, MPI_DOUBLE,
                   MPI_MAX, MPI_COMM_WORLD);
     if (global_error < tolerance) break;
   }

**After:**

.. code-block:: cpp

   for (int iter = state.iteration; iter < max_iterations; iter++) {
     state.iteration = iter;

     // Computation
     compute_step(fenix_comm, data);

     // Checkpoint periodically
     if (iter % 10 == 0) {
       checkpoint_state(state);
     }

     // Check convergence
     MPI_Allreduce(&local_error, &global_error, 1, MPI_DOUBLE,
                   MPI_MAX, fenix_comm);
     if (global_error < tolerance) break;
   }

----

Testing Your Migrated Application
----------------------------------

Test fault tolerance before deploying to production:

Manual Testing
~~~~~~~~~~~~~~

Inject failures programmatically:

.. code-block:: cpp

   #include <signal.h>

   // Kill specific rank at specific iteration for testing
   if (rank == 2 && iteration == 50) {
     printf("Rank %d injecting failure\n", rank);
     raise(SIGKILL);
   }

Run and verify recovery:

.. code-block:: bash

   mpiexec --with-ft mpi -n 10 ./myapp

Automated Testing
~~~~~~~~~~~~~~~~~

Create test script that verifies recovery:

.. code-block:: bash

   #!/bin/bash

   # Run with known failure point
   mpiexec --with-ft mpi -n 8 ./myapp --test-mode --fail-rank 3 --fail-iter 25

   # Check output for successful recovery
   if grep -q "Successfully recovered" output.log; then
     echo "Test PASSED"
     exit 0
   else
     echo "Test FAILED"
     exit 1
   fi

Validation Checklist
~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   □ Application compiles with Fenix
   □ Runs successfully without failures
   □ Survives single rank failure
   □ Survives multiple rank failures
   □ Recovers to correct state (if using data recovery)
   □ Produces correct results after recovery
   □ Performance acceptable with checkpointing overhead
   □ Spare ranks properly configured (10-20% of active ranks)

----

Troubleshooting Migration Issues
---------------------------------

Compilation Issues
~~~~~~~~~~~~~~~~~~

**Problem:** Cannot find fenix.hpp or fenix.h

.. code-block:: text

   Solution: Add include path to Fenix installation

   mpicxx -I/path/to/fenix/include ...

**Problem:** Undefined reference to Fenix functions

.. code-block:: text

   Solution: Link against Fenix library

   mpicxx ... -L/path/to/fenix/lib -lfenix

Runtime Issues
~~~~~~~~~~~~~~

**Problem:** Application hangs at MPI_Init

.. code-block:: text

   Solution: Must run with --with-ft mpi flag

   mpiexec --with-ft mpi -n 16 ./myapp

**Problem:** Segfault when failure occurs

.. code-block:: text

   Possible causes:
   1. Forgot to replace MPI_COMM_WORLD with fenix_comm
   2. Multiple MPI versions - rebuild with -DFENIX_SYSTEM_INC_FIX=ON
   3. Invalid data member pointers in checkpoint

**Problem:** Application exits instead of recovering

.. code-block:: text

   Solution: Not enough spare ranks

   Increase spare count in fenix::init({.spares = N})

Data Recovery Issues
~~~~~~~~~~~~~~~~~~~~

**Problem:** State not restored correctly after recovery

.. code-block:: text

   Check:
   1. Are all necessary data members registered?
   2. Are checkpoint and restore using same member IDs?
   3. Is member_define using correct buffer pointers?
   4. Did you call commit_barrier after storing?

**Problem:** Crash during member_restore

.. code-block:: text

   Common causes:
   1. Buffer pointer changed between store and restore
   2. Mismatched count or datatype
   3. Group not created before restore
   4. Using member_create instead of member_define on recovery

Performance Issues
~~~~~~~~~~~~~~~~~~

**Problem:** Checkpointing takes too long

.. code-block:: text

   Solutions:
   1. Reduce checkpoint frequency
   2. Use partial checkpoints (SUBSET) for large arrays
   3. Choose efficient policy (IMR vs RAID)
   4. Checkpoint only essential state

See :doc:`/troubleshooting` for more detailed debugging help.

----

Performance Considerations
--------------------------

Checkpoint Overhead
~~~~~~~~~~~~~~~~~~~

Typical overhead ranges from 2-10% depending on:

* Checkpoint frequency
* Amount of data checkpointed
* Redundancy policy (IMR vs RAID)

**Optimization Tips:**

1. **Tune Frequency:** Profile to find sweet spot between overhead and recovery time

   .. code-block:: cpp

      // More frequent = less rollback but more overhead
      const int CHECKPOINT_FREQ = 10;  // Start here, tune based on profiling

2. **Minimize Data:** Only checkpoint essential state

   .. code-block:: cpp

      // Good: checkpoint only necessary arrays
      member_create(GROUP_ID, 0, critical_state, size, MPI_DOUBLE);

      // Avoid: checkpointing temporary/recomputable data

3. **Use Partial Checkpoints:** For large arrays, checkpoint only modified regions

   .. code-block:: cpp

      // Only checkpoint first 100 elements
      DataSubset subset;
      subset_create(0, 99, &subset);
      member_store(GROUP_ID, MEMBER_ID, subset);

Memory Overhead
~~~~~~~~~~~~~~~

Fenix stores redundant copies based on policy:

* **IMR:** Stores copy on buddy rank (2x memory per checkpoint)
* **RAID:** Stores parity across ranks (lower memory, more computation)

Choose based on your memory constraints:

.. code-block:: cpp

   // Lower memory overhead
   group_create(GROUP_ID, {
     .policy_name = FENIX_DATA_POLICY_IN_MEMORY_RAID,
     .policy_value = (int[]){1, num_ranks/2}
   });

Spare Rank Calculation
~~~~~~~~~~~~~~~~~~~~~~

Balance recovery capability vs resource efficiency:

.. code-block:: text

   Spare Ranks = ceil(Active Ranks × Failure Rate × MTTR / MTBF)

   Rule of thumb: 10-20% spare ranks, minimum 1-2

Example:

.. code-block:: text

   1000 active ranks, 0.1% failure probability per hour, 5 minute MTTR
   → ~10-20 spare ranks

----

Migration Checklist
-------------------

Use this checklist to track your migration progress:

Initial Setup
~~~~~~~~~~~~~

.. code-block:: text

   □ Install Fenix (see :doc:`/installation`)
   □ Verify Open MPI 5+ with ULFM support
   □ Choose migration approach (minimal vs full)
   □ Choose recovery pattern (exception vs longjmp)

Code Changes
~~~~~~~~~~~~

.. code-block:: text

   □ Add fenix.hpp or fenix.h include
   □ Add fenix::init() call after MPI_Init
   □ Replace all MPI_COMM_WORLD with fenix_comm
   □ Add Fenix_Finalize() before MPI_Finalize
   □ Update build system (CMakeLists.txt or Makefile)
   □ Test compilation

Data Recovery (if applicable)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   □ Set recovery mode (exception or longjmp)
   □ Identify application state to checkpoint
   □ Create data group
   □ Register data members
   □ Add checkpoint calls at regular intervals
   □ Implement state restoration for recovered ranks
   □ Register recovery callback (exception mode)
   □ Update application loop to resume from checkpoint

Message Logging (if applicable)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   □ Create message log
   □ Define log regions around communication
   □ Add message log to checkpoint
   □ Enable automatic replay
   □ Test with complex communication patterns

Testing
~~~~~~~

.. code-block:: text

   □ Compile successfully
   □ Run without failures
   □ Test with injected single failure
   □ Test with multiple failures
   □ Verify correct recovery
   □ Validate output correctness
   □ Measure performance overhead
   □ Test with different checkpoint frequencies

Deployment
~~~~~~~~~~

.. code-block:: text

   □ Document required spare ranks
   □ Update run scripts to include --with-ft mpi
   □ Train users on new execution requirements
   □ Set up monitoring for recovery events
   □ Establish performance baselines

----

Next Steps
----------

Now that you've migrated your application:

**Optimize Performance:**

* :doc:`optimize-checkpoints` - Reduce checkpoint overhead
* :doc:`performance-tuning` - Tune for your workload

**Advanced Features:**

* :doc:`message-logging` - Add message replay
* :doc:`inline-recovery-callbacks` - Customize recovery behavior
* :doc:`handle-cascading-failures` - Handle multiple concurrent failures

**Get Help:**

* :doc:`/troubleshooting` - Common issues and solutions
* :doc:`/faq` - Frequently asked questions
* :doc:`/api/index` - Complete API reference

----

Summary
-------

**What You've Learned:**

* How to perform a minimal migration (process recovery only)
* How to add data checkpointing for full recovery
* When and how to use message logging
* How to test fault tolerance
* Common migration patterns and pitfalls

**Key Takeaways:**

1. **Start Simple:** Begin with process recovery, add data recovery incrementally
2. **Test Early:** Inject failures during development, not just in production
3. **Tune Performance:** Balance checkpoint frequency with recovery time
4. **Choose Right Pattern:** Exception-based for modern C++, longjmp for simpler cases

**Migration Time Investment:**

* Minimal (process only): 15-30 minutes
* Full (with data recovery): 2-4 hours
* Advanced (with message logging): +1-2 hours
* Performance tuning: Ongoing

With Fenix, your MPI application can now survive rank failures and continue execution, dramatically improving reliability for long-running jobs.
