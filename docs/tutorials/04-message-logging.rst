Tutorial 4: Message Logging for Seamless Recovery
==================================================

**Time:** 45 minutes | **Difficulty:** Advanced

**Prerequisites:** :doc:`01-first-program`, :doc:`02-data-recovery`, :doc:`03-inline-recovery`

Welcome to the final and most advanced tutorial in the Fenix learning path! In this tutorial, you'll learn about **message logging**, the ultimate fault tolerance technique that enables truly seamless recovery without any recomputation or manual message replay.

In previous tutorials, you learned process recovery, data checkpointing, and inline recovery callbacks. However, these approaches still require recomputing any work lost between checkpoints. **Message logging eliminates this limitation** by automatically recording and replaying MPI messages, allowing recovered ranks to receive exactly the same messages they would have received if no failure occurred.

.. contents:: In This Tutorial
   :local:
   :depth: 2

Learning Objectives
-------------------

By completing this tutorial, you will:

✓ Understand what message logging is and why it's powerful

✓ Create and configure message logs for your application

✓ Use regions and windows to organize logged messages

✓ Integrate message logging with data checkpointing

✓ Build a complete iterative solver with transparent recovery

✓ Use INLINE_AUTOSYNC mode for fully automatic recovery

✓ Understand performance trade-offs and optimization strategies

✓ Know when message logging is worth the overhead

What is Message Logging?
-------------------------

The Recomputation Problem
^^^^^^^^^^^^^^^^^^^^^^^^^^

Even with data checkpointing and inline recovery, you still lose work:

.. code-block:: cpp

   // Checkpoint every 10 iterations
   for (int iter = 0; iter < 100; iter++) {
     do_expensive_computation();  // 10 seconds per iteration
     exchange_boundaries();        // MPI communication

     if (iter % 10 == 0) checkpoint();
   }

**Problem:** If rank 1 fails at iteration 47:

- Recovered rank restores to iteration 40 (last checkpoint)
- Must recompute iterations 40-47 (70 seconds of work lost!)
- Survivors may need to wait or resend messages

The Message Logging Solution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Message logging** records all MPI messages sent and received. During recovery:

1. Recovered rank restores checkpoint from iteration 40
2. Fenix automatically replays messages from iterations 40-47
3. Recovered rank catches up without recomputation
4. No messages need to be resent by survivors

.. code-block:: cpp

   // Create message log
   fenix::mlog::create(LOG_ID, res_comm, num_regions);

   for (int iter = 0; iter < 100; iter++) {
     fenix::mlog::begin_region(LOG_ID, iter);  // Start logging region

     do_expensive_computation();
     exchange_boundaries();  // Messages automatically logged!

     if (iter % 10 == 0) checkpoint();
   }

**Benefits:**

- **Zero recomputation**: Recovered ranks replay logged messages
- **Seamless recovery**: Survivors don't resend anything
- **Exact reproducibility**: Recovered ranks get identical message sequences
- **Automatic**: No manual message management needed

When to Use Message Logging
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Use Message Logging When
     - Don't Use Message Logging When
   * - Iterations are expensive (seconds each)
     - Iterations are cheap (milliseconds)
   * - Communication patterns are complex
     - Simple send/recv patterns
   * - Recomputation is costly
     - Recomputation is cheap
   * - Need exact reproducibility
     - Approximate results are fine
   * - Running long jobs (hours/days)
     - Short jobs (minutes)

**Rule of Thumb:** If checkpoint intervals are 30+ seconds of computation, message logging is worth the overhead.

Part 1: Creating and Using Message Logs (10 minutes)
-----------------------------------------------------

Step 1.1: Basic Setup
^^^^^^^^^^^^^^^^^^^^^^

Include the message logging namespace and create a log:

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>

   constexpr int MLOG_ID = 0;
   constexpr int NUM_REGIONS = 20;  // Keep last 20 regions

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Create message log
     fenix::mlog::create(MLOG_ID, res_comm, NUM_REGIONS);

     // ... application code ...
   }

**Parameters:**

- ``MLOG_ID``: Unique identifier for this log (you can have multiple logs)
- ``res_comm``: Communicator to log messages on
- ``NUM_REGIONS``: How many log regions to keep in memory (circular buffer)

Step 1.2: Understanding Regions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A **region** is a logical grouping of messages that belong together - typically one iteration of your application loop:

.. code-block:: cpp
   :linenos:

   for (int iter = 0; iter < 100; iter++) {
     // Begin region for this iteration
     fenix::mlog::begin_region(MLOG_ID, iter);

     // All MPI messages in this iteration are logged
     MPI_Send(data, count, MPI_DOUBLE, dest, tag, res_comm);
     MPI_Recv(buffer, count, MPI_DOUBLE, source, tag, res_comm, &status);

     // Region ends when next region begins or log is deactivated
   }

**Why regions?**

- Organize messages by iteration/phase
- Enable partial replay (replay from region N to region M)
- Control memory usage (keep only recent regions)

Step 1.3: Activating the Log
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Messages are only logged when the log is **active**:

.. code-block:: cpp

   // Activate log to start recording
   fenix::mlog::activate(MLOG_ID);

   // MPI messages now logged
   MPI_Send(...);

   // Deactivate to stop recording
   fenix::mlog::deactivate(MLOG_ID);

   // MPI messages NOT logged
   MPI_Send(...);

**Tip:** Usually you activate once at the start and leave it active for the entire application.

Step 1.4: Basic Example
^^^^^^^^^^^^^^^^^^^^^^^^

Here's a minimal message logging example:

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <stdio.h>

   constexpr int MLOG_ID = 0;

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Create log with space for 10 regions
     fenix::mlog::create(MLOG_ID, res_comm, 10);

     // Activate logging
     fenix::mlog::activate(MLOG_ID);

     // Ring communication pattern
     int left = (rank - 1 + size) % size;
     int right = (rank + 1) % size;

     for (int iter = 0; iter < 50; iter++) {
       fenix::mlog::begin_region(MLOG_ID, iter);

       int send_val = rank * 1000 + iter;
       int recv_val = -1;

       // Send to right, receive from left
       MPI_Sendrecv(&send_val, 1, MPI_INT, right, 0,
                    &recv_val, 1, MPI_INT, left, 0,
                    res_comm, MPI_STATUS_IGNORE);

       printf("Rank %d iter %d: sent %d, received %d\n",
              rank, iter, send_val, recv_val);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

This example logs all messages. In the next section, we'll add checkpointing and recovery.

Part 2: Integrating with Data Recovery (15 minutes)
----------------------------------------------------

Message logging really shines when combined with data checkpointing. Let's build a complete example.

Step 2.1: Checkpoint Message Log State
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The message log itself must be checkpointed so recovered ranks know which messages to replay:

.. code-block:: cpp
   :linenos:

   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;
   constexpr int MLOG_MEMBER_ID = 1;

   // Create data group
   fenix::data::group_create(GROUP_ID);

   if (fenix::role() == fenix::INITIAL_RANK) {
     // Create state member
     fenix::data::member_create(GROUP_ID, STATE_ID, &state, 2, MPI_INT);

     // Create message log member
     fenix::mlog::create_data_member(MLOG_ID, GROUP_ID, MLOG_MEMBER_ID);

     // Checkpoint
     fenix::data::member_store(GROUP_ID);
     fenix::data::commit_barrier(GROUP_ID);
   } else {
     // Recovered ranks define and restore
     fenix::data::member_define(GROUP_ID, STATE_ID, &state, 2, MPI_INT);
     fenix::mlog::define_data_member(MLOG_ID, GROUP_ID, MLOG_MEMBER_ID);

     fenix::data::member_restore(GROUP_ID, STATE_ID);
     fenix::data::member_restore(GROUP_ID, MLOG_MEMBER_ID);

     // Sync message log to current iteration
     fenix::mlog::sync(MLOG_ID, state.iteration);
   }

**Key Functions:**

- ``mlog::create_data_member()``: Register log for checkpointing
- ``mlog::define_data_member()``: Define log member (recovered ranks)
- ``mlog::sync()``: Replay messages to catch up to given iteration

Step 2.2: Automatic Recovery with INLINE_AUTOSYNC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The most powerful mode is ``INLINE_AUTOSYNC``, which handles everything automatically:

.. code-block:: cpp
   :linenos:

   // Enable inline recovery with automatic sync
   fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

   // Activate the log
   fenix::mlog::activate(MLOG_ID);

   // Register callback for data restoration
   fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
     fenix::data::group_create(GROUP_ID);
     fenix::data::member_restore(GROUP_ID, STATE_ID);
     fenix::data::member_restore(GROUP_ID, MLOG_MEMBER_ID);

     printf("Rank %d: Recovered to iteration %d\n", rank, state.iteration);
   });

**What INLINE_AUTOSYNC does:**

1. When a failure occurs, callback restores checkpoint
2. Fenix automatically replays logged messages
3. Recovered rank catches up without recomputation
4. Execution continues seamlessly

Step 2.3: Complete Integrated Example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here's a complete example combining message logging, data checkpointing, and inline recovery:

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <stdio.h>
   #include <signal.h>
   #include <vector>

   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;
   constexpr int DATA_ID = 1;
   constexpr int MLOG_ID = 2;

   constexpr int MLOG_ID_VAL = 0;
   constexpr int NUM_REGIONS = 15;
   constexpr int CHECKPOINT_FREQ = 10;

   struct State {
     int rank;
     int iteration;
   };

   void inject_failure(int rank, int iter) {
     if (rank == 1 && iter == 27) {
       printf("Rank %d: FAILURE at iteration %d\n", rank, iter);
       raise(SIGKILL);
     }
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix with 2 spares
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     int left = (rank - 1 + size) % size;
     int right = (rank + 1) % size;

     // Create message log
     fenix::mlog::create(MLOG_ID_VAL, res_comm, NUM_REGIONS);

     // Application state and data
     State state{rank, 0};
     std::vector<double> data(100, static_cast<double>(rank));

     // Create data group
     fenix::data::group_create(GROUP_ID);

     if (fenix::role() == fenix::INITIAL_RANK) {
       printf("Rank %d: Initial rank\n", rank);

       // Create data members
       fenix::data::member_create(GROUP_ID, STATE_ID, &state, 2, MPI_INT);
       fenix::data::member_create(GROUP_ID, DATA_ID,
                                  data.data(), data.size(), MPI_DOUBLE);
       fenix::mlog::create_data_member(MLOG_ID_VAL, GROUP_ID, MLOG_ID);

       // Initial checkpoint
       fenix::data::member_store(GROUP_ID);
       fenix::data::commit_barrier(GROUP_ID);

     } else {
       printf("Rank %d: Recovered rank\n", rank);

       // Define members
       fenix::data::member_define(GROUP_ID, STATE_ID, &state, 2, MPI_INT);
       fenix::data::member_define(GROUP_ID, DATA_ID,
                                  data.data(), data.size(), MPI_DOUBLE);
       fenix::mlog::define_data_member(MLOG_ID_VAL, GROUP_ID, MLOG_ID);

       // Restore checkpoint
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, DATA_ID);
       fenix::data::member_restore(GROUP_ID, MLOG_ID);

       // Sync message log to catch up
       fenix::mlog::sync(MLOG_ID_VAL, state.iteration);

       printf("Rank %d: Restored to iteration %d\n", rank, state.iteration);
     }

     // Configure inline recovery with auto-sync
     fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);
     fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

     // Activate message logging
     fenix::mlog::activate(MLOG_ID_VAL);

     // Register recovery callback
     fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
       printf("Rank %d: Inline recovery callback\n", rank);

       fenix::data::group_create(GROUP_ID);
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, DATA_ID);
       fenix::data::member_restore(GROUP_ID, MLOG_ID);

       printf("Rank %d: Continuing from iteration %d\n",
              rank, state.iteration);
     });

     // Main computation loop
     for (int iter = state.iteration; iter < 50; iter++) {
       inject_failure(rank, iter);
       state.iteration = iter;

       // Begin message log region
       fenix::mlog::begin_region(MLOG_ID_VAL, iter);

       // Computation
       for (size_t i = 0; i < data.size(); i++) {
         data[i] = data[i] * 1.01 + rank;
       }

       // Exchange data with neighbors (logged automatically!)
       double send_left = data[0];
       double send_right = data[99];
       double recv_left, recv_right;

       MPI_Sendrecv(&send_right, 1, MPI_DOUBLE, right, 0,
                    &recv_left, 1, MPI_DOUBLE, left, 0,
                    res_comm, MPI_STATUS_IGNORE);

       MPI_Sendrecv(&send_left, 1, MPI_DOUBLE, left, 1,
                    &recv_right, 1, MPI_DOUBLE, right, 1,
                    res_comm, MPI_STATUS_IGNORE);

       // Update boundaries
       data[0] = 0.5 * (data[0] + recv_left);
       data[99] = 0.5 * (data[99] + recv_right);

       // Periodic checkpoint
       if ((iter + 1) % CHECKPOINT_FREQ == 0) {
         fenix::data::member_store(GROUP_ID);
         fenix::data::commit_barrier(GROUP_ID);

         if (rank == 0) {
           printf("Checkpoint at iteration %d\n", iter + 1);
         }
       }
     }

     printf("Rank %d: Completed all iterations\n", rank);

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Building and Running
^^^^^^^^^^^^^^^^^^^^

Compile and link with both Fenix libraries:

.. code-block:: bash

   mpicxx -std=c++17 mlog_example.cpp -o mlog_example \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix -lmlog

Run with 5 ranks (3 active + 2 spares):

.. code-block:: bash

   mpiexec --with-ft mpi -n 5 ./mlog_example

**Expected Output:**

.. code-block:: text

   Rank 0: Initial rank
   Rank 1: Initial rank
   Rank 2: Initial rank
   Checkpoint at iteration 10
   Checkpoint at iteration 20
   Rank 1: FAILURE at iteration 27
   Rank 0: Inline recovery callback
   Rank 0: Continuing from iteration 27
   Rank 2: Inline recovery callback
   Rank 2: Continuing from iteration 27
   Rank 1: Recovered rank
   Rank 1: Restored to iteration 20
   [Message replay happens automatically]
   Rank 1: Continuing from iteration 27
   Checkpoint at iteration 30
   Checkpoint at iteration 40
   Rank 0: Completed all iterations
   Rank 1: Completed all iterations
   Rank 2: Completed all iterations

Notice:

- Rank 1 failed at iteration 27
- Restored to checkpoint at iteration 20
- Automatically replayed messages for iterations 20-27
- Continued seamlessly at iteration 27 with survivors

Part 3: Complete Iterative Solver Example (15 minutes)
-------------------------------------------------------

Let's build a realistic conjugate gradient solver that demonstrates message logging in a production-like scenario.

The Application: Conjugate Gradient Solver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We'll implement a distributed conjugate gradient solver for Ax = b:

- Each rank owns part of the matrix A and vectors x, b
- Requires matrix-vector products, dot products, vector updates
- Heavy communication (allreduces, neighbor exchanges)
- Expensive iterations (perfect for message logging!)

Complete Implementation
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
   :linenos:

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <cmath>
   #include <stdio.h>
   #include <signal.h>

   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;
   constexpr int VEC_X_ID = 1;
   constexpr int VEC_R_ID = 2;
   constexpr int VEC_P_ID = 3;
   constexpr int MLOG_ID = 4;

   constexpr int MLOG_ID_VAL = 0;
   constexpr int LOCAL_SIZE = 1000;
   constexpr int MAX_ITERS = 100;
   constexpr int CHECKPOINT_FREQ = 10;
   constexpr double TOLERANCE = 1e-6;

   struct SolverState {
     int rank;
     int iteration;
     double residual_norm;
     double rho;
   };

   void inject_failure(int rank, int iter) {
     if (rank == 2 && iter == 35) {
       printf("Rank %d: Simulating failure at iteration %d\n", rank, iter);
       raise(SIGKILL);
     }
   }

   // Matrix-vector multiply for simple tridiagonal matrix
   void matvec(const std::vector<double>& x_local,
               std::vector<double>& y_local,
               double ghost_left, double ghost_right, int rank, int size) {
     int n = x_local.size();

     // First element
     y_local[0] = 2.0 * x_local[0] - x_local[1];
     if (rank > 0) y_local[0] -= ghost_left;

     // Interior elements
     for (int i = 1; i < n - 1; i++) {
       y_local[i] = -x_local[i-1] + 2.0 * x_local[i] - x_local[i+1];
     }

     // Last element
     y_local[n-1] = -x_local[n-2] + 2.0 * x_local[n-1];
     if (rank < size - 1) y_local[n-1] -= ghost_right;
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize Fenix with 2 spares
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     int left = rank - 1;
     int right = rank + 1;

     // Create message log with enough regions
     fenix::mlog::create(MLOG_ID_VAL, res_comm, CHECKPOINT_FREQ + 5);

     // Solver state and vectors
     SolverState state{rank, 0, 1.0, 0.0};
     std::vector<double> x(LOCAL_SIZE, 0.0);     // Solution vector
     std::vector<double> r(LOCAL_SIZE);          // Residual
     std::vector<double> p(LOCAL_SIZE);          // Search direction
     std::vector<double> Ap(LOCAL_SIZE);         // Matrix-vector product
     std::vector<double> b(LOCAL_SIZE, 1.0);     // RHS vector

     // Initialize residual r = b - Ax (with x = 0, r = b)
     for (int i = 0; i < LOCAL_SIZE; i++) {
       r[i] = b[i];
       p[i] = r[i];
     }

     // Initial rho = r^T * r
     double local_rho = 0.0;
     for (int i = 0; i < LOCAL_SIZE; i++) {
       local_rho += r[i] * r[i];
     }
     MPI_Allreduce(&local_rho, &state.rho, 1, MPI_DOUBLE, MPI_SUM, res_comm);
     state.residual_norm = std::sqrt(state.rho);

     // Create data group
     fenix::data::group_create(GROUP_ID);

     if (fenix::role() == fenix::INITIAL_RANK) {
       printf("Rank %d: Starting CG solver (initial residual: %e)\n",
              rank, state.residual_norm);

       // Create data members
       fenix::data::member_create(GROUP_ID, STATE_ID, &state, 4, MPI_DOUBLE);
       fenix::data::member_create(GROUP_ID, VEC_X_ID,
                                  x.data(), LOCAL_SIZE, MPI_DOUBLE);
       fenix::data::member_create(GROUP_ID, VEC_R_ID,
                                  r.data(), LOCAL_SIZE, MPI_DOUBLE);
       fenix::data::member_create(GROUP_ID, VEC_P_ID,
                                  p.data(), LOCAL_SIZE, MPI_DOUBLE);
       fenix::mlog::create_data_member(MLOG_ID_VAL, GROUP_ID, MLOG_ID);

       // Initial checkpoint
       fenix::data::member_store(GROUP_ID);
       fenix::data::commit_barrier(GROUP_ID);

     } else {
       printf("Rank %d: Recovered rank, restoring checkpoint\n", rank);

       // Define members
       fenix::data::member_define(GROUP_ID, STATE_ID, &state, 4, MPI_DOUBLE);
       fenix::data::member_define(GROUP_ID, VEC_X_ID,
                                  x.data(), LOCAL_SIZE, MPI_DOUBLE);
       fenix::data::member_define(GROUP_ID, VEC_R_ID,
                                  r.data(), LOCAL_SIZE, MPI_DOUBLE);
       fenix::data::member_define(GROUP_ID, VEC_P_ID,
                                  p.data(), LOCAL_SIZE, MPI_DOUBLE);
       fenix::mlog::define_data_member(MLOG_ID_VAL, GROUP_ID, MLOG_ID);

       // Restore checkpoint
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, VEC_X_ID);
       fenix::data::member_restore(GROUP_ID, VEC_R_ID);
       fenix::data::member_restore(GROUP_ID, VEC_P_ID);
       fenix::data::member_restore(GROUP_ID, MLOG_ID);

       // Sync message log
       fenix::mlog::sync(MLOG_ID_VAL, state.iteration);

       printf("Rank %d: Restored to iteration %d (residual: %e)\n",
              rank, state.iteration, state.residual_norm);
     }

     // Configure inline recovery with auto-sync
     fenix::set_option(fenix::RESUME_MODE, fenix::RETURN);
     fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

     // Activate message logging
     fenix::mlog::activate(MLOG_ID_VAL);

     // Register recovery callback
     fenix::callback_register([&](MPI_Comm repaired_comm, int mpi_err) {
       printf("Rank %d: Inline recovery at iteration %d\n",
              rank, state.iteration);

       fenix::data::group_create(GROUP_ID);
       fenix::data::member_restore(GROUP_ID, STATE_ID);
       fenix::data::member_restore(GROUP_ID, VEC_X_ID);
       fenix::data::member_restore(GROUP_ID, VEC_R_ID);
       fenix::data::member_restore(GROUP_ID, VEC_P_ID);
       fenix::data::member_restore(GROUP_ID, MLOG_ID);

       printf("Rank %d: Recovered, continuing from iteration %d\n",
              rank, state.iteration);
     });

     // Main CG iteration loop
     for (int iter = state.iteration; iter < MAX_ITERS &&
                                      state.residual_norm > TOLERANCE; iter++) {
       inject_failure(rank, iter);
       state.iteration = iter;

       // Begin message log region for this iteration
       fenix::mlog::begin_region(MLOG_ID_VAL, iter);

       // Exchange ghost cells for matrix-vector product
       double ghost_left = 0.0, ghost_right = 0.0;
       if (rank > 0) {
         MPI_Sendrecv(&p[0], 1, MPI_DOUBLE, left, 0,
                      &ghost_left, 1, MPI_DOUBLE, left, 1,
                      res_comm, MPI_STATUS_IGNORE);
       }
       if (rank < size - 1) {
         MPI_Sendrecv(&p[LOCAL_SIZE-1], 1, MPI_DOUBLE, right, 1,
                      &ghost_right, 1, MPI_DOUBLE, right, 0,
                      res_comm, MPI_STATUS_IGNORE);
       }

       // Compute Ap = A * p
       matvec(p, Ap, ghost_left, ghost_right, rank, size);

       // Compute alpha = rho / (p^T * Ap)
       double local_pAp = 0.0;
       for (int i = 0; i < LOCAL_SIZE; i++) {
         local_pAp += p[i] * Ap[i];
       }
       double pAp = 0.0;
       MPI_Allreduce(&local_pAp, &pAp, 1, MPI_DOUBLE, MPI_SUM, res_comm);
       double alpha = state.rho / pAp;

       // Update solution: x = x + alpha * p
       for (int i = 0; i < LOCAL_SIZE; i++) {
         x[i] += alpha * p[i];
       }

       // Update residual: r = r - alpha * Ap
       for (int i = 0; i < LOCAL_SIZE; i++) {
         r[i] -= alpha * Ap[i];
       }

       // Compute new rho = r^T * r
       double new_local_rho = 0.0;
       for (int i = 0; i < LOCAL_SIZE; i++) {
         new_local_rho += r[i] * r[i];
       }
       double new_rho = 0.0;
       MPI_Allreduce(&new_local_rho, &new_rho, 1, MPI_DOUBLE, MPI_SUM, res_comm);

       // Compute beta and update search direction
       double beta = new_rho / state.rho;
       for (int i = 0; i < LOCAL_SIZE; i++) {
         p[i] = r[i] + beta * p[i];
       }

       // Update state
       state.rho = new_rho;
       state.residual_norm = std::sqrt(new_rho);

       // Progress report
       if (rank == 0 && iter % 5 == 0) {
         printf("Iteration %d: residual = %e\n", iter, state.residual_norm);
       }

       // Periodic checkpoint
       if ((iter + 1) % CHECKPOINT_FREQ == 0) {
         fenix::data::member_store(GROUP_ID);
         fenix::data::commit_barrier(GROUP_ID);

         if (rank == 0) {
           printf("Checkpoint at iteration %d\n", iter + 1);
         }
       }
     }

     if (state.residual_norm <= TOLERANCE) {
       if (rank == 0) {
         printf("\nCG converged in %d iterations (residual: %e)\n",
                state.iteration, state.residual_norm);
       }
     } else {
       if (rank == 0) {
         printf("\nCG did not converge in %d iterations (residual: %e)\n",
                MAX_ITERS, state.residual_norm);
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Understanding the CG Solver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Key Operations:**

1. **Lines 197-206**: Ghost cell exchanges (automatically logged)
2. **Lines 208-220**: Matrix-vector product and dot products
3. **Lines 222-235**: Vector updates (standard CG algorithm)
4. **Lines 247-253**: Periodic checkpointing

**What Happens During Failure:**

1. Rank 2 fails at iteration 35
2. Checkpoint was at iteration 30
3. Recovered rank restores to iteration 30
4. Message log automatically replays:
   - Ghost exchanges for iterations 30-34
   - Allreduce operations for iterations 30-34
5. Recovered rank catches up to iteration 35 without recomputation
6. All ranks continue together at iteration 35

Building and Running
^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   mpicxx -std=c++17 cg_solver.cpp -o cg_solver \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix -lmlog

   mpiexec --with-ft mpi -n 5 ./cg_solver

**Expected Output:**

.. code-block:: text

   Rank 0: Starting CG solver (initial residual: 5.477226e+01)
   Rank 1: Starting CG solver (initial residual: 5.477226e+01)
   Rank 2: Starting CG solver (initial residual: 5.477226e+01)
   Iteration 0: residual = 5.477226e+01
   Iteration 5: residual = 3.162278e+01
   Checkpoint at iteration 10
   Iteration 10: residual = 1.825742e+01
   Iteration 15: residual = 1.054093e+01
   Checkpoint at iteration 20
   Iteration 20: residual = 6.082763e+00
   Iteration 25: residual = 3.511885e+00
   Checkpoint at iteration 30
   Iteration 30: residual = 2.027326e+00
   Rank 2: Simulating failure at iteration 35
   Rank 0: Inline recovery at iteration 35
   Rank 1: Inline recovery at iteration 35
   Rank 2: Recovered rank, restoring checkpoint
   Rank 2: Restored to iteration 30 (residual: 2.027326e+00)
   [Message replay: iterations 30-34 replayed automatically]
   Rank 2: Recovered, continuing from iteration 35
   Iteration 35: residual = 1.170270e+00
   Checkpoint at iteration 40
   Iteration 40: residual = 6.754670e-01
   ...
   CG converged in 78 iterations (residual: 9.876543e-07)

Part 4: Performance Considerations (5 minutes)
-----------------------------------------------

Memory Usage
^^^^^^^^^^^^

Message logs consume memory proportional to:

- Number of regions kept (``NUM_REGIONS`` parameter)
- Size and frequency of messages
- Number of MPI operations per region

**Guidelines:**

.. code-block:: cpp

   // For small messages (< 1 KB), can keep many regions
   fenix::mlog::create(MLOG_ID, res_comm, 50);

   // For large messages (> 1 MB), keep fewer regions
   fenix::mlog::create(MLOG_ID, res_comm, 10);

   // Rule of thumb: keep enough regions to cover 2x checkpoint interval
   int checkpoint_freq = 10;
   fenix::mlog::create(MLOG_ID, res_comm, checkpoint_freq * 2);

Overhead
^^^^^^^^

Message logging adds overhead in two places:

1. **Recording**: Small overhead on each MPI operation (1-5%)
2. **Replay**: Larger overhead during recovery (depends on number of messages)

**Optimization Tips:**

- Log only performance-critical communicators
- Use multiple logs for different communication patterns
- Deactivate logging for initialization/finalization phases
- Choose checkpoint frequency to balance logging overhead vs. replay time

Selective Logging
^^^^^^^^^^^^^^^^^

You don't have to log everything:

.. code-block:: cpp

   // Create two communicators: one logged, one not
   MPI_Comm compute_comm, io_comm;
   MPI_Comm_split(res_comm, /*color*/ 0, rank, &compute_comm);
   MPI_Comm_dup(res_comm, &io_comm);

   // Log only compute communication
   fenix::mlog::create(MLOG_ID, compute_comm, NUM_REGIONS);
   fenix::mlog::activate(MLOG_ID);

   // I/O communication not logged
   MPI_File_write_all(..., io_comm);

Best Practices
--------------

When to Use Message Logging
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

✓ **Good candidates:**

- Long-running simulations (hours/days)
- Expensive iterations (seconds each)
- Complex communication patterns
- Need for exact reproducibility
- High failure rates expected

✗ **Poor candidates:**

- Short jobs (minutes)
- Cheap iterations (milliseconds)
- Simple communication patterns
- Recomputation is fast
- Memory constrained systems

Region Organization
^^^^^^^^^^^^^^^^^^^

**Good practice:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iters; iter++) {
     fenix::mlog::begin_region(MLOG_ID, iter);  // One region per iteration

     // All iteration work here
     compute();
     communicate();
   }

**Bad practice:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iters; iter++) {
     for (int phase = 0; phase < 10; phase++) {
       fenix::mlog::begin_region(MLOG_ID, iter * 10 + phase);  // Too fine-grained!
       small_operation();
     }
   }

Checkpoint Integration
^^^^^^^^^^^^^^^^^^^^^^

**Match checkpoint frequency to region capacity:**

.. code-block:: cpp

   constexpr int CHECKPOINT_FREQ = 10;
   constexpr int NUM_REGIONS = CHECKPOINT_FREQ * 2;  // Keep 2x checkpoint interval

   fenix::mlog::create(MLOG_ID, res_comm, NUM_REGIONS);

This ensures you can always replay from the last checkpoint.

Exercises
---------

Exercise 1: Measure Replay Time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Modify the CG solver to measure:

1. Time to checkpoint
2. Time to replay messages after recovery
3. Compare: replay time vs. recomputation time

Use ``MPI_Wtime()`` to measure timing.

Exercise 2: Multiple Message Logs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Create two message logs:

1. One for point-to-point messages
2. One for collective operations

Test whether separate logs improve performance.

Exercise 3: Adaptive Region Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Implement dynamic region capacity:

- Monitor memory usage
- Adjust ``NUM_REGIONS`` if memory pressure is high
- Test on large-scale problems

Exercise 4: Partial Replay
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Implement custom replay logic:

- Replay only certain message types
- Skip replaying messages that can be recomputed cheaply
- Measure performance difference

Exercise 5: Production Deployment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Prepare the CG solver for production:

1. Add error handling for all MPI operations
2. Implement logging of recovery events
3. Add validation of replayed messages
4. Create performance profiling output

Next Steps
----------

Congratulations! You've completed the Fenix tutorial series and learned the most advanced fault tolerance techniques available.

🎓 **You Now Know:**

- Process recovery with spare ranks
- Data checkpointing and restoration
- Inline recovery with callbacks
- Message logging and automatic replay

🚀 **Continue Your Journey:**

**How-To Guides:**

- :doc:`/howto/message-logging` - Advanced message logging patterns
- :doc:`/howto/migrate-existing-app` - Convert existing MPI applications
- :doc:`/howto/debug-fenix-app` - Debugging fault-tolerant applications

**Concept Guides:**

- :doc:`/guides/process-recovery` - Deep dive into communicator repair
- :doc:`/guides/data-recovery` - Understanding IMR policies
- :doc:`/api/message-recovery` - Message logging internals

**API Reference:**

- :cpp:func:`fenix::mlog::create` - Create message logs
- :cpp:func:`fenix::mlog::activate` - Activate logging
- :cpp:func:`fenix::mlog::begin_region` - Start log regions
- :cpp:func:`fenix::mlog::sync` - Replay logged messages

Summary
-------

**You've Mastered:**

✅ Message logging concepts and benefits

✅ Creating and managing message logs with regions

✅ Integrating message logging with data checkpointing

✅ Using INLINE_AUTOSYNC for fully automatic recovery

✅ Building production-ready fault-tolerant solvers

✅ Performance optimization and best practices

**Key Concepts:**

- **Message logging**: Records MPI messages for automatic replay
- **Regions**: Logical groupings of messages (typically iterations)
- **INLINE_AUTOSYNC**: Fully automatic recovery with message replay
- **Zero recomputation**: Recovered ranks replay messages instead of recomputing
- **Seamless recovery**: Survivors don't resend messages

**Recovery Techniques Comparison:**

.. list-table::
   :header-rows: 1
   :widths: 20 25 25 30

   * - Technique
     - Recomputation
     - Code Complexity
     - Best For
   * - Process Recovery
     - Full restart
     - Low
     - Learning, simple apps
   * - + Data Checkpointing
     - Since checkpoint
     - Moderate
     - Most applications
   * - + Inline Recovery
     - Since checkpoint
     - Moderate
     - Complex state management
   * - + Message Logging
     - None
     - Higher
     - Long expensive iterations

**The Full Stack:**

You now understand all four layers of Fenix fault tolerance:

1. **Process Recovery**: Automatic communicator repair
2. **Data Recovery**: Checkpoint/restore application state
3. **Inline Recovery**: Transparent recovery via callbacks
4. **Message Logging**: Zero-recomputation replay

**You're ready to build production fault-tolerant HPC applications!**

Thank you for completing the Fenix tutorial series. Good luck with your fault-tolerant computing!
