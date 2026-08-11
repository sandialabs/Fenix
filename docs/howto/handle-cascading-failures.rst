Handle Cascading Failures
=========================

When failures happen during recovery, you have a cascading failure. This guide shows you how to handle multiple failures robustly, including failures that occur while recovering from previous failures.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Wrap recovery operations in a retry loop with exception handling:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 3});  // Need extra spares
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     const int GROUP_ID = 0, MEMBER_ID = 0;
     double my_data[1000];

     if (fenix::role() == fenix::INITIAL_RANK) {
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, MEMBER_ID, my_data, 1000, MPI_DOUBLE);
       data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     } else {
       // Retry loop for recovery
       while (true) {
         try {
           data::group_create(GROUP_ID);
           data::member_define(GROUP_ID, MEMBER_ID, my_data, 1000, MPI_DOUBLE);
           data::member_restore(GROUP_ID, MEMBER_ID);
           break;  // Success - exit retry loop
         } catch (fenix::CommException& e) {
           // Another failure during recovery - retry
           printf("Cascading failure detected, retrying...\n");
           continue;
         }
       }
     }

     // Register callback with retry logic
     fenix::callback_register([&](MPI_Comm comm, int err) {
       while (true) {
         try {
           data::group_create(GROUP_ID);
           data::member_restore(GROUP_ID, MEMBER_ID);
           break;
         } catch (fenix::CommException& e) {
           continue;  // Retry
         }
       }
     });

     // Main loop
     // ...
   }

What Are Cascading Failures?
-----------------------------

A **cascading failure** occurs when:

1. A rank fails
2. While recovering from that failure, another rank fails
3. The recovery process must handle both failures

Example Timeline
~~~~~~~~~~~~~~~~

.. code-block:: text

   Time 0: Ranks 0-7 running normally
   Time 1: Rank 3 fails
   Time 2: Fenix begins recovery (repairing communicator)
   Time 3: During recovery, Rank 5 also fails
   Time 4: Fenix must now recover from BOTH failures

   Without cascading failure handling:
     → Application crashes or hangs

   With cascading failure handling:
     → Recovery succeeds, application continues

Why Cascading Failures Happen
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**During Recovery Operations:**

- Collective operations (``commit_barrier``, ``MPI_Allreduce``)
- Data restore operations (communicating with replica holders)
- Message log synchronization

**Hardware Correlation:**

- Same node hosting multiple ranks
- Power supply issues affecting multiple nodes
- Network switch failures

**Time Correlation:**

- Software bugs triggered by recovery code path
- Memory corruption detected during recovery
- Resource exhaustion during recovery overhead

When to Expect Cascading Failures
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**High Risk Scenarios:**

- Large-scale systems (thousands of ranks)
- Long recovery operations (large checkpoints)
- Hardware with known issues
- Oversubscribed systems (multiple ranks per node)

**Lower Risk Scenarios:**

- Small-scale systems (tens of ranks)
- Fast recovery (small checkpoints, message logging)
- Reliable hardware
- Dedicated resources per rank

Retry Loop Pattern
------------------

Basic Retry Loop
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   while (true) {
     try {
       // Operation that may fail
       data::member_restore(GROUP_ID, MEMBER_ID);
       break;  // Success - exit loop

     } catch (fenix::CommException& e) {
       // Failure - loop will retry
       continue;
     }
   }

Retry with Maximum Attempts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Prevent infinite loops:

.. code-block:: cpp

   const int MAX_RETRIES = 10;
   int retry_count = 0;

   while (retry_count < MAX_RETRIES) {
     try {
       data::member_restore(GROUP_ID, MEMBER_ID);
       break;  // Success

     } catch (fenix::CommException& e) {
       retry_count++;
       printf("Retry %d/%d after cascading failure\n",
              retry_count, MAX_RETRIES);

       if (retry_count >= MAX_RETRIES) {
         fprintf(stderr, "Failed after %d retries, aborting\n", MAX_RETRIES);
         MPI_Abort(MPI_COMM_WORLD, 1);
       }
     }
   }

Retry with Backoff
~~~~~~~~~~~~~~~~~~

Add delays between retries to reduce contention:

.. code-block:: cpp

   #include <chrono>
   #include <thread>

   const int MAX_RETRIES = 10;
   const int BASE_DELAY_MS = 100;

   for (int retry = 0; retry < MAX_RETRIES; retry++) {
     try {
       data::member_restore(GROUP_ID, MEMBER_ID);
       break;  // Success

     } catch (fenix::CommException& e) {
       if (retry < MAX_RETRIES - 1) {
         // Exponential backoff: 100ms, 200ms, 400ms, ...
         int delay_ms = BASE_DELAY_MS * (1 << retry);
         printf("Retry %d after %d ms\n", retry + 1, delay_ms);
         std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
       } else {
         fprintf(stderr, "Recovery failed after %d retries\n", MAX_RETRIES);
         MPI_Abort(MPI_COMM_WORLD, 1);
       }
     }
   }

member_define vs member_create
-------------------------------

Key Difference
~~~~~~~~~~~~~~

- ``member_create``: Creates a new member. Fails if member already exists.
- ``member_define``: Defines member location. Idempotent - safe to call multiple times.

**For cascading failures, always use member_define in recovery code.**

Why member_define is Essential
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // WRONG - will fail on second attempt
   try {
     data::member_create(GROUP_ID, MEMBER_ID, buffer, count, type);
   } catch (fenix::CommException& e) {
     // Member was partially created - next member_create will fail!
     data::member_create(GROUP_ID, MEMBER_ID, buffer, count, type);  // ERROR
   }

   // RIGHT - idempotent, works every time
   try {
     data::member_define(GROUP_ID, MEMBER_ID, buffer, count, type);
   } catch (fenix::CommException& e) {
     // Safe to call again
     data::member_define(GROUP_ID, MEMBER_ID, buffer, count, type);  // OK
   }

Pattern: Create Once, Define for Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   if (fenix::role() == fenix::INITIAL_RANK) {
     // Initial ranks: use member_create
     data::group_create(GROUP_ID);
     data::member_create(GROUP_ID, MEMBER_ID, buffer, count, type);

   } else {
     // Recovered ranks: use member_define
     while (true) {
       try {
         data::group_create(GROUP_ID);
         data::member_define(GROUP_ID, MEMBER_ID, buffer, count, type);
         data::member_restore(GROUP_ID, MEMBER_ID);
         break;
       } catch (fenix::CommException& e) {
         continue;  // Safe because member_define is idempotent
       }
     }
   }

   // Callbacks: always use member_define
   fenix::callback_register([&](MPI_Comm comm, int err) {
     while (true) {
       try {
         data::group_create(GROUP_ID);
         data::member_define(GROUP_ID, MEMBER_ID, buffer, count, type);
         data::member_restore(GROUP_ID, MEMBER_ID);
         break;
       } catch (fenix::CommException& e) {
         continue;
       }
     }
   });

Nested Try-Catch Pattern
-------------------------

Recovery at Multiple Levels
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Outer level: main application loop
   for (int i = 0; i < MAX_ITER; i++) {
     try {

       // Inner level: checkpoint operation
       if (i % 10 == 0) {
         while (true) {
           try {
             data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
             data::commit_barrier(GROUP_ID);
             break;  // Checkpoint succeeded
           } catch (fenix::CommException& e) {
             printf("Cascading failure during checkpoint, retrying...\n");
             continue;
           }
         }
       }

       // MPI operation
       MPI_Allreduce(MPI_IN_PLACE, buffer, count, MPI_DOUBLE, MPI_SUM, comm);

     } catch (fenix::CommException& e) {
       printf("Failure in main loop, continuing from iteration %d\n", i);
       continue;
     }
   }

Separate Recovery for Different Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void checkpoint_with_retry() {
     const int MAX_RETRIES = 5;

     for (int retry = 0; retry < MAX_RETRIES; retry++) {
       try {
         data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
         data::commit_barrier(GROUP_ID);
         return;  // Success
       } catch (fenix::CommException& e) {
         if (retry < MAX_RETRIES - 1) {
           printf("Checkpoint failed, retry %d/%d\n", retry + 1, MAX_RETRIES);
         } else {
           throw;  // Re-throw after max retries
         }
       }
     }
   }

   void restore_with_retry() {
     const int MAX_RETRIES = 10;

     for (int retry = 0; retry < MAX_RETRIES; retry++) {
       try {
         data::group_create(GROUP_ID);
         data::member_define(GROUP_ID, MEMBER_ID, buffer, count, type);
         data::member_restore(GROUP_ID, MEMBER_ID);
         return;  // Success
       } catch (fenix::CommException& e) {
         if (retry < MAX_RETRIES - 1) {
           printf("Restore failed, retry %d/%d\n", retry + 1, MAX_RETRIES);
         } else {
           fprintf(stderr, "Restore failed after %d retries\n", MAX_RETRIES);
           MPI_Abort(MPI_COMM_WORLD, 1);
         }
       }
     }
   }

   int main(int argc, char** argv) {
     // ...

     try {
       checkpoint_with_retry();
       // ... application code ...

     } catch (fenix::CommException& e) {
       restore_with_retry();
     }
   }

Timeout Strategies
------------------

Time-Based Retry Limits
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <chrono>

   using Clock = std::chrono::steady_clock;
   using Duration = std::chrono::seconds;

   bool restore_with_timeout(int group_id, int member_id, Duration timeout) {
     auto start_time = Clock::now();

     while (true) {
       try {
         data::group_create(group_id);
         data::member_define(group_id, member_id, buffer, count, type);
         data::member_restore(group_id, member_id);
         return true;  // Success

       } catch (fenix::CommException& e) {
         auto elapsed = Clock::now() - start_time;

         if (elapsed >= timeout) {
           fprintf(stderr, "Recovery timeout after %ld seconds\n",
                   std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
           return false;
         }

         printf("Retry after %.1f seconds elapsed\n",
                std::chrono::duration<double>(elapsed).count());
       }
     }
   }

   int main(int argc, char** argv) {
     // ...

     if (!restore_with_timeout(GROUP_ID, MEMBER_ID, std::chrono::seconds(60))) {
       fprintf(stderr, "Could not recover within 60 seconds, aborting\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
     }
   }

Adaptive Timeout
~~~~~~~~~~~~~~~~

Increase timeout based on failure count:

.. code-block:: cpp

   struct RecoveryStats {
     int failure_count = 0;
     int total_retries = 0;

     Duration get_timeout() const {
       // Base timeout: 30s, increases by 10s per failure
       return std::chrono::seconds(30 + failure_count * 10);
     }
   };

   bool restore_adaptive(int group_id, int member_id, RecoveryStats& stats) {
     stats.failure_count++;
     Duration timeout = stats.get_timeout();

     printf("Recovery attempt #%d with timeout %lds\n",
            stats.failure_count, timeout.count());

     auto start_time = Clock::now();
     int retry = 0;

     while (Clock::now() - start_time < timeout) {
       try {
         data::group_create(group_id);
         data::member_define(group_id, member_id, buffer, count, type);
         data::member_restore(group_id, member_id);
         return true;

       } catch (fenix::CommException& e) {
         retry++;
         stats.total_retries++;
       }
     }

     return false;  // Timeout
   }

Complete Example with Multiple Failures
----------------------------------------

This example demonstrates handling multiple cascading failures:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <signal.h>
   #include <vector>
   #include <chrono>
   #include <thread>

   constexpr int MAX_ITER = 100;
   constexpr int CHECKPOINT_FREQ = 10;
   constexpr int GROUP_ID = 0;
   constexpr int STATE_ID = 0;

   struct State {
     int iteration;
     int rank;
     int recovery_count;
   };

   void inject_failures(int iteration) {
     int global_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

     // Multiple failures at different times
     bool should_fail = false;
     should_fail |= global_rank == 2 && iteration == 25;  // First failure
     should_fail |= global_rank == 5 && iteration == 26;  // Cascading failure
     should_fail |= global_rank == 1 && iteration == 27;  // Third failure
     should_fail |= global_rank == 6 && iteration == 60;  // Later failure

     if (should_fail) {
       printf("Rank %d failing at iteration %d\n", global_rank, iteration);
       raise(SIGKILL);
     }
   }

   bool restore_with_retry(State& state, const int MAX_RETRIES = 10) {
     for (int retry = 0; retry < MAX_RETRIES; retry++) {
       try {
         data::group_create(GROUP_ID);
         data::member_define(GROUP_ID, STATE_ID,
                            &state, sizeof(State), MPI_BYTE);
         data::member_restore(GROUP_ID, STATE_ID);

         state.recovery_count++;
         printf("Rank %d restored to iteration %d (recovery #%d, retry %d)\n",
                state.rank, state.iteration, state.recovery_count, retry);
         return true;

       } catch (fenix::CommException& e) {
         if (retry < MAX_RETRIES - 1) {
           printf("Cascading failure during restore, retry %d/%d\n",
                  retry + 1, MAX_RETRIES);
           // Brief delay to reduce contention
           std::this_thread::sleep_for(std::chrono::milliseconds(50));
         } else {
           fprintf(stderr, "Failed to restore after %d retries\n", MAX_RETRIES);
           return false;
         }
       }
     }
     return false;
   }

   bool checkpoint_with_retry(int group_id, const int MAX_RETRIES = 5) {
     for (int retry = 0; retry < MAX_RETRIES; retry++) {
       try {
         data::member_store(group_id, STATE_ID, SUBSET_FULL);
         data::commit_barrier(group_id);
         return true;

       } catch (fenix::CommException& e) {
         if (retry < MAX_RETRIES - 1) {
           printf("Cascading failure during checkpoint, retry %d/%d\n",
                  retry + 1, MAX_RETRIES);
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
         } else {
           fprintf(stderr, "Failed to checkpoint after %d retries\n", MAX_RETRIES);
           return false;
         }
       }
     }
     return false;
   }

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     // Need more spares for multiple failures
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 5});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     State state;

     // Initialize or recover
     if (fenix::role() == fenix::INITIAL_RANK) {
       state.rank = rank;
       state.iteration = 0;
       state.recovery_count = 0;

       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, STATE_ID,
                          &state, sizeof(State), MPI_BYTE);

       if (!checkpoint_with_retry(GROUP_ID)) {
         fprintf(stderr, "Initial checkpoint failed\n");
         MPI_Abort(MPI_COMM_WORLD, 1);
       }

       printf("Rank %d initialized\n", rank);

     } else {
       // Recovered rank
       if (!restore_with_retry(state)) {
         fprintf(stderr, "Initial recovery failed, aborting\n");
         MPI_Abort(MPI_COMM_WORLD, 1);
       }
     }

     // Register callback with retry logic
     fenix::callback_register([&state](MPI_Comm comm, int err) {
       if (!restore_with_retry(state)) {
         fprintf(stderr, "Callback recovery failed\n");
         MPI_Abort(MPI_COMM_WORLD, 1);
       }
     });

     // Main application loop
     for (int i = state.iteration; i < MAX_ITER; i++) {
       try {
         inject_failures(i);

         state.iteration = i;

         // Application work
         double value = i * rank;
         MPI_Allreduce(MPI_IN_PLACE, &value, 1, MPI_DOUBLE, MPI_SUM, res_comm);

         if (rank == 0 && i % 20 == 0) {
           printf("Iteration %d complete\n", i);
         }

         // Checkpoint periodically
         if (i % CHECKPOINT_FREQ == 0) {
           if (!checkpoint_with_retry(GROUP_ID)) {
             fprintf(stderr, "Checkpoint failed at iteration %d\n", i);
             // Continue anyway - will lose progress if failure occurs
           }
         }

       } catch (fenix::CommException& e) {
         printf("Rank %d recovered, continuing from iteration %d\n",
                rank, state.iteration);
         continue;
       }
     }

     if (rank == 0) {
       printf("Completed with %d recoveries\n", state.recovery_count);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Testing Cascading Failures
---------------------------

Inject Multiple Failures
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void inject_cascading_failures(int iteration, int n_ranks) {
     int global_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

     // First wave of failures at iteration 20
     if (iteration == 20) {
       if (global_rank == 2 || global_rank == 5) {
         printf("First wave failure: rank %d\n", global_rank);
         raise(SIGKILL);
       }
     }

     // Second wave during recovery (iteration 21-22)
     if (iteration == 21) {
       if (global_rank == 7) {
         printf("Second wave failure: rank %d\n", global_rank);
         raise(SIGKILL);
       }
     }

     // Third wave
     if (iteration == 22) {
       if (global_rank == 1 || global_rank == 9) {
         printf("Third wave failure: rank %d\n", global_rank);
         raise(SIGKILL);
       }
     }
   }

Test Recovery Under Load
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void test_recovery_stress(MPI_Comm comm) {
     const int ITERATIONS = 1000;
     const int LARGE_SIZE = 1000000;

     std::vector<double> data(LARGE_SIZE);

     for (int i = 0; i < ITERATIONS; i++) {
       try {
         // Large communication during recovery stress
         MPI_Allreduce(MPI_IN_PLACE, data.data(), LARGE_SIZE,
                      MPI_DOUBLE, MPI_SUM, comm);

       } catch (fenix::CommException& e) {
         printf("Recovered under load at iteration %d\n", i);
         continue;
       }
     }
   }

Verify Recovery Statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct RecoveryMetrics {
     int total_failures = 0;
     int cascading_failures = 0;
     int max_retry_count = 0;
     std::chrono::duration<double> total_recovery_time{0};

     void print_summary(MPI_Comm comm) {
       int global_failures, global_cascading, global_max_retry;

       MPI_Reduce(&total_failures, &global_failures, 1, MPI_INT,
                 MPI_SUM, 0, comm);
       MPI_Reduce(&cascading_failures, &global_cascading, 1, MPI_INT,
                 MPI_SUM, 0, comm);
       MPI_Reduce(&max_retry_count, &global_max_retry, 1, MPI_INT,
                 MPI_MAX, 0, comm);

       int rank;
       MPI_Comm_rank(comm, &rank);

       if (rank == 0) {
         printf("\n=== Recovery Statistics ===\n");
         printf("Total failures: %d\n", global_failures);
         printf("Cascading failures: %d\n", global_cascading);
         printf("Max retry count: %d\n", global_max_retry);
         printf("Avg recovery time: %.3f seconds\n",
                total_recovery_time.count() / global_failures);
       }
     }
   };

Troubleshooting
---------------

**Problem: Infinite retry loop**

- Add maximum retry count
- Implement timeout
- Check if you have enough spare ranks
- Verify checkpoint data is not corrupted

**Problem: Recovery succeeds but data is wrong**

- Ensure ``member_define`` uses correct buffer pointer
- Check that checkpoint completed before first failure
- Verify datatype and count match between store and restore
- Use checksums to detect corruption

**Problem: Hang during recovery**

- Some ranks may be waiting in collective operation
- Check if all surviving ranks are in the retry loop
- Verify no deadlock in callback code
- Ensure timeout is implemented

**Problem: Out of spare ranks**

- Increase number of spares in ``fenix::init``
- Reduce scope of failures (spatial/temporal correlation)
- Consider shrinking communicator instead of replacing ranks
- Check if spares are being released properly

**Problem: Recovery too slow**

- Reduce checkpoint size
- Use partial checkpoints (subsets)
- Implement backoff delays to reduce contention
- Consider message logging for faster recovery

See Also
--------

- :doc:`inline-recovery-callbacks` - Setting up recovery callbacks
- :doc:`checkpoint-data` - How to checkpoint application state
- :doc:`test-locally` - Testing cascading failures locally
- :doc:`performance-tuning` - Optimizing recovery performance
- :doc:`/guides/process-recovery` - Understanding recovery mechanisms
- :doc:`/troubleshooting` - Common problems and solutions
