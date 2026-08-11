Performance Tuning
==================

This guide shows you how to optimize Fenix for performance by tuning checkpoint frequency, memory usage, redundancy policies, and message logging. Follow these recommendations to minimize fault tolerance overhead while maintaining reliability.

.. contents:: On this page
   :local:
   :depth: 2

Quick Reference
---------------

**Common Performance Issues and Solutions:**

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Problem
     - Quick Fix
     - See Section
   * - Checkpoint too slow
     - Reduce frequency, use subsets
     - :ref:`checkpoint-frequency`
   * - High memory usage
     - Reduce depth, fewer snapshots
     - :ref:`memory-optimization`
   * - Recovery too slow
     - Use message logging
     - :ref:`message-logging-performance`
   * - Network congestion
     - Adjust redundancy policy
     - :ref:`redundancy-policies`
   * - High overhead
     - Profile and tune all parameters
     - :ref:`profiling`

.. _checkpoint-frequency:

Checkpoint Frequency Optimization
----------------------------------

Finding the Right Balance
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Too Frequent Checkpointing:**

- High overhead from data copying
- Network congestion from redundant storage
- Slower overall execution time
- Cache pollution

**Too Infrequent Checkpointing:**

- More work lost on failure
- Longer recovery time
- Higher total execution time (including recovery)

Optimal Checkpoint Interval Formula
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The optimal checkpoint interval depends on:

- **MTBF** (Mean Time Between Failures)
- **T_checkpoint** (Time to create checkpoint)
- **T_compute** (Time for one iteration)

**Rule of thumb:**

.. math::

   interval = \sqrt{\frac{2 \times MTBF \times T_{compute}}{T_{checkpoint}}}

**Practical example:**

.. code-block:: cpp

   // Given:
   // - MTBF = 10 hours = 36000 seconds
   // - T_compute = 0.1 seconds per iteration
   // - T_checkpoint = 2 seconds
   //
   // interval = sqrt(2 * 36000 * 0.1 / 2) = sqrt(3600) = 60 iterations

   const int CHECKPOINT_INTERVAL = 60;

   for (int i = 0; i < MAX_ITER; i++) {
     // ... computation ...

     if (i % CHECKPOINT_INTERVAL == 0) {
       checkpoint_all();
     }
   }

Adaptive Checkpointing
~~~~~~~~~~~~~~~~~~~~~~

Adjust checkpoint frequency based on observed failure rate:

.. code-block:: cpp

   #include <chrono>

   struct CheckpointStrategy {
     int base_interval = 50;
     int current_interval;
     int failure_count = 0;
     std::chrono::steady_clock::time_point last_failure;

     CheckpointStrategy() : current_interval(base_interval) {}

     void on_failure() {
       failure_count++;
       last_failure = std::chrono::steady_clock::now();

       // Increase checkpoint frequency after failure
       current_interval = std::max(10, base_interval / 2);
     }

     void on_checkpoint(int iteration) {
       // Gradually restore normal interval
       auto now = std::chrono::steady_clock::now();
       auto time_since_failure =
         std::chrono::duration_cast<std::chrono::minutes>(now - last_failure);

       if (time_since_failure.count() > 30) {
         // No failures for 30 minutes - restore normal frequency
         current_interval = base_interval;
       }
     }

     bool should_checkpoint(int iteration) const {
       return iteration % current_interval == 0;
     }
   };

   int main(int argc, char** argv) {
     // ...
     CheckpointStrategy strategy;

     for (int i = 0; i < MAX_ITER; i++) {
       try {
         // ... computation ...

         if (strategy.should_checkpoint(i)) {
           checkpoint_all();
           strategy.on_checkpoint(i);
         }

       } catch (fenix::CommException& e) {
         strategy.on_failure();
         continue;
       }
     }
   }

Time-Based Checkpointing
~~~~~~~~~~~~~~~~~~~~~~~~

Checkpoint based on elapsed time instead of iterations:

.. code-block:: cpp

   using Clock = std::chrono::steady_clock;
   using Duration = std::chrono::seconds;

   const Duration CHECKPOINT_INTERVAL(300);  // 5 minutes

   auto last_checkpoint = Clock::now();

   for (int i = 0; i < MAX_ITER; i++) {
     // ... computation ...

     auto now = Clock::now();
     auto elapsed = now - last_checkpoint;

     if (elapsed >= CHECKPOINT_INTERVAL) {
       checkpoint_all();
       last_checkpoint = now;
     }
   }

.. _memory-optimization:

Memory Usage Optimization
--------------------------

Understanding Memory Overhead
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix's In-Memory RAID stores redundant copies of your data:

.. code-block:: text

   Original data size: D
   Number of ranks: N
   Redundancy parameter: k
   Effective ranks for storage: n

   Memory overhead per rank ≈ D * (k + 1) / n

   Example:
   D = 1 GB per rank
   N = 100 ranks
   k = 1 (one parity block)
   n = N/2 = 50 (distribute across half the ranks)

   Memory per rank ≈ 1 GB * (1 + 1) / 50 = 40 MB

Reduce Snapshot Depth
~~~~~~~~~~~~~~~~~~~~~~

Keep fewer historical snapshots:

.. code-block:: cpp

   // Default: depth = 1 (keep only latest snapshot)
   data::group_create(GROUP_ID, {
     .depth = 1  // Minimum memory usage
   });

   // Multiple snapshots for rollback capability
   data::group_create(GROUP_ID, {
     .depth = 3  // Keep last 3 snapshots (3x memory)
   });

**Recommendation:** Use depth = 1 unless you need rollback capability.

Use Partial Checkpoints
~~~~~~~~~~~~~~~~~~~~~~~~

Checkpoint only what changed:

.. code-block:: cpp

   const int N = 1000000;
   double large_array[N];

   // Only checkpoint the active region (same on all ranks)
   int active_start = compute_active_start();
   int active_end = compute_active_end();

   Fenix_Data_subset subset;
   Fenix_Data_subset_create(active_start, active_end, &subset);

   // Use member_store when all ranks checkpoint the same element ranges
   // Use member_storev if different ranks checkpoint different element ranges
   data::member_store(GROUP_ID, MEMBER_ID, subset);
   data::commit_barrier(GROUP_ID);

   // Saves memory and network bandwidth

See :doc:`partial-checkpoints` for more details.

Separate Groups by Frequency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use different groups for data with different checkpoint needs:

.. code-block:: cpp

   const int FREQUENT_GROUP = 0;  // Small, changes every iteration
   const int RARE_GROUP = 1;      // Large, rarely changes

   // Small state checkpointed every iteration
   data::group_create(FREQUENT_GROUP, {.depth = 1});
   data::member_create(FREQUENT_GROUP, 0, &state, sizeof(state), MPI_BYTE);

   // Large arrays checkpointed every 100 iterations
   data::group_create(RARE_GROUP, {.depth = 1});
   data::member_create(RARE_GROUP, 0, large_data, N, MPI_DOUBLE);

   for (int i = 0; i < MAX_ITER; i++) {
     // Checkpoint small state every iteration
     data::member_store(FREQUENT_GROUP, 0, SUBSET_FULL);
     data::commit_barrier(FREQUENT_GROUP);

     // Checkpoint large data every 100 iterations
     if (i % 100 == 0) {
       data::member_store(RARE_GROUP, 0, SUBSET_FULL);
       data::commit_barrier(RARE_GROUP);
     }
   }

.. _redundancy-policies:

Redundancy Policy Tuning
-------------------------

In-Memory RAID Parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~

The RAID policy uses two parameters: ``k`` (parity blocks) and ``n`` (ranks to use):

.. code-block:: cpp

   int error;
   int raid_params[2] = {k, n};

   Fenix_Data_group_create(
     GROUP_ID,
     res_comm,
     0,                                   // Starting timestamp
     1,                                   // Depth
     FENIX_DATA_POLICY_IN_MEMORY_RAID,
     raid_params,
     &error
   );

**Parameter k (parity blocks):**

- ``k = 1``: Tolerate 1 failure, low overhead
- ``k = 2``: Tolerate 2 failures, moderate overhead
- ``k = 3+``: Tolerate more failures, high overhead

**Parameter n (storage ranks):**

- ``n = size``: Distribute across all ranks (low per-rank memory, high network)
- ``n = size/2``: Distribute across half (moderate memory and network)
- ``n = size/4``: Distribute across quarter (high per-rank memory, low network)

Choosing RAID Parameters
~~~~~~~~~~~~~~~~~~~~~~~~~

**For small-scale systems (< 100 ranks):**

.. code-block:: cpp

   int k = 1;           // One parity block
   int n = size;        // All ranks
   int raid_params[2] = {k, n};

**For medium-scale systems (100-1000 ranks):**

.. code-block:: cpp

   int k = 1;
   int n = size / 2;    // Half the ranks
   int raid_params[2] = {k, n};

**For large-scale systems (1000+ ranks):**

.. code-block:: cpp

   int k = 2;           // Two parity blocks for better fault tolerance
   int n = size / 4;    // Quarter of ranks
   int raid_params[2] = {k, n};

**High-reliability systems:**

.. code-block:: cpp

   int k = 3;           // Three parity blocks
   int n = size / 2;
   int raid_params[2] = {k, n};

Tradeoff Analysis
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Scenario 1: Minimize memory overhead
   int raid_params[2] = {1, 100};  // k=1, n=100 ranks
   // Memory overhead: ~1% per rank
   // Network overhead: High (100 ranks participate)
   // Fault tolerance: 1 failure

   // Scenario 2: Balance
   int raid_params[2] = {1, 50};   // k=1, n=50 ranks
   // Memory overhead: ~2% per rank
   // Network overhead: Moderate
   // Fault tolerance: 1 failure

   // Scenario 3: Minimize network overhead
   int raid_params[2] = {1, 10};   // k=1, n=10 ranks
   // Memory overhead: ~10% per rank
   // Network overhead: Low (only 10 ranks)
   // Fault tolerance: 1 failure

   // Scenario 4: High reliability
   int raid_params[2] = {2, 50};   // k=2, n=50 ranks
   // Memory overhead: ~4% per rank
   // Network overhead: Moderate
   // Fault tolerance: 2 simultaneous failures

.. _message-logging-performance:

Message Logging Optimization
-----------------------------

When to Use Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Use message logging when:**

- Recovery time must be minimized
- Network patterns are regular (stencils, nearest-neighbor)
- Messages are small to moderate size
- Failures affect few ranks at a time

**Don't use message logging when:**

- Global communication dominates (allreduce, allgather on all data)
- Messages are very large (approaching checkpoint size)
- Communication patterns are irregular
- Memory is constrained

Message Log Window Sizing
~~~~~~~~~~~~~~~~~~~~~~~~~~

The window size (number of regions) affects memory usage and recovery capability:

.. code-block:: cpp

   const int CHECKPOINT_FREQ = 10;
   const int MLOG_REGIONS = CHECKPOINT_FREQ + 1;  // +1 for safety

   fenix::mlog::create(LOG_ID, res_comm, MLOG_REGIONS);

**Guidelines:**

- Window size ≥ checkpoint interval
- Larger window = more memory, better recovery
- Smaller window = less memory, may need full rollback

**Example:**

.. code-block:: cpp

   const int CHECKPOINT_FREQ = 100;
   const int MLOG_REGIONS = 110;  // 10% buffer

   fenix::mlog::create(LOG_ID, res_comm, MLOG_REGIONS);
   fenix::mlog::activate(LOG_ID);
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

   for (int i = 0; i < MAX_ITER; i++) {
     fenix::mlog::begin_region(LOG_ID, i);

     // Logged MPI operations
     MPI_Sendrecv(...);

     if (i % CHECKPOINT_FREQ == 0) {
       checkpoint_all();
     }
   }

Selective Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~

Log only critical communication patterns:

.. code-block:: cpp

   fenix::mlog::create(LOG_ID, res_comm, REGIONS);

   for (int i = 0; i < MAX_ITER; i++) {
     // Activate logging for nearest-neighbor communication
     fenix::mlog::activate(LOG_ID);
     fenix::mlog::begin_region(LOG_ID, i);

     // Logged: nearest-neighbor exchanges
     exchange_ghost_points(left_neighbor, right_neighbor);

     fenix::mlog::deactivate(LOG_ID);

     // Not logged: global reductions (too expensive to replay)
     double global_sum;
     MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, res_comm);
   }

.. _profiling:

Profiling Fenix Overhead
-------------------------

Measuring Checkpoint Time
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <chrono>

   using Clock = std::chrono::high_resolution_clock;
   using Duration = std::chrono::duration<double>;

   struct CheckpointStats {
     Duration total_time{0};
     int count = 0;

     void record_checkpoint(Duration time) {
       total_time += time;
       count++;
     }

     double average_ms() const {
       return count > 0 ?
         std::chrono::duration_cast<std::chrono::milliseconds>(
           total_time / count
         ).count() : 0.0;
     }
   };

   CheckpointStats stats;

   void checkpoint_with_timing() {
     auto start = Clock::now();

     data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
     data::commit_barrier(GROUP_ID);

     auto end = Clock::now();
     stats.record_checkpoint(end - start);
   }

   // At end of program
   printf("Average checkpoint time: %.2f ms\n", stats.average_ms());

Comprehensive Performance Profiling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct PerformanceProfile {
     Duration checkpoint_time{0};
     Duration recovery_time{0};
     Duration computation_time{0};
     Duration mpi_time{0};

     int checkpoint_count = 0;
     int recovery_count = 0;

     void print_report(MPI_Comm comm) {
       int rank;
       MPI_Comm_rank(comm, &rank);

       if (rank == 0) {
         auto total = checkpoint_time + recovery_time +
                     computation_time + mpi_time;

         printf("\n=== Performance Profile ===\n");
         printf("Total time: %.2f s\n",
                std::chrono::duration<double>(total).count());
         printf("Computation: %.2f s (%.1f%%)\n",
                std::chrono::duration<double>(computation_time).count(),
                100.0 * computation_time / total);
         printf("MPI: %.2f s (%.1f%%)\n",
                std::chrono::duration<double>(mpi_time).count(),
                100.0 * mpi_time / total);
         printf("Checkpointing: %.2f s (%.1f%%) [%d checkpoints]\n",
                std::chrono::duration<double>(checkpoint_time).count(),
                100.0 * checkpoint_time / total,
                checkpoint_count);
         printf("Recovery: %.2f s (%.1f%%) [%d recoveries]\n",
                std::chrono::duration<double>(recovery_time).count(),
                100.0 * recovery_time / total,
                recovery_count);
         printf("\nCheckpoint overhead: %.2f ms per checkpoint\n",
                checkpoint_count > 0 ?
                std::chrono::duration_cast<std::chrono::milliseconds>(
                  checkpoint_time / checkpoint_count
                ).count() : 0.0);
         printf("Recovery overhead: %.2f ms per recovery\n",
                recovery_count > 0 ?
                std::chrono::duration_cast<std::chrono::milliseconds>(
                  recovery_time / recovery_count
                ).count() : 0.0);
       }
     }
   };

   template<typename Func>
   void profile(PerformanceProfile& prof, Duration& timer, Func&& func) {
     auto start = Clock::now();
     func();
     timer += Clock::now() - start;
   }

   int main(int argc, char** argv) {
     PerformanceProfile prof;

     // ...

     for (int i = 0; i < MAX_ITER; i++) {
       // Profile computation
       profile(prof, prof.computation_time, [&]() {
         compute_iteration();
       });

       // Profile MPI
       profile(prof, prof.mpi_time, [&]() {
         MPI_Allreduce(MPI_IN_PLACE, data, count,
                      MPI_DOUBLE, MPI_SUM, res_comm);
       });

       // Profile checkpointing
       if (i % CHECKPOINT_FREQ == 0) {
         profile(prof, prof.checkpoint_time, [&]() {
           checkpoint_all();
         });
         prof.checkpoint_count++;
       }
     }

     prof.print_report(res_comm);
   }

Performance Benchmarks
----------------------

Checkpoint Performance
~~~~~~~~~~~~~~~~~~~~~~

Typical checkpoint performance (1000 doubles per rank):

.. code-block:: text

   Ranks    Size      Time (ms)   Overhead
   ------   -------   ---------   --------
   10       80 KB     5-10        < 1%
   100      800 KB    15-30       1-2%
   1000     8 MB      50-100      2-5%

Factors affecting performance:

- Network bandwidth and latency
- Data size per rank
- Number of ranks in RAID group
- MPI implementation

Recovery Performance
~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   Recovery Method          Time (1 failure)
   -------------------      ----------------
   Global checkpoint        500-2000 ms
   Message logging only     50-200 ms
   Checkpoint + mlog        100-500 ms

Message Logging Overhead
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   Pattern             Memory          Replay Time
   -----------------   -------------   -----------
   Nearest-neighbor    10-50 KB        5-20 ms
   Stencil (5-point)   50-200 KB       20-50 ms
   All-to-all          1-10 MB         100-500 ms

Tradeoffs and Recommendations
------------------------------

Checkpoint vs Message Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Metric
     - Checkpoint Only
     - Checkpoint + Message Log
   * - Memory overhead
     - Low (2-5%)
     - Moderate (5-15%)
   * - Recovery time
     - Slow (seconds)
     - Fast (milliseconds)
   * - Complexity
     - Simple
     - Moderate
   * - Best for
     - Irregular communication
     - Regular, local communication

Frequency vs Overhead
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   Checkpoint Frequency    Overhead    Recovery Cost
   --------------------    --------    -------------
   Every iteration         High 10%    Very low
   Every 10 iterations     Low 1-2%    Low
   Every 100 iterations    Very low    Moderate
   Every 1000 iterations   Minimal     High

Complete Tuned Example
-----------------------

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <chrono>

   constexpr int N = 100000;
   constexpr int MAX_ITER = 1000;

   // Tuned parameters
   constexpr int CHECKPOINT_FREQ = 50;  // Based on profiling
   constexpr int MLOG_REGIONS = 55;     // Checkpoint freq + buffer

   constexpr int GROUP_ID = 0;
   constexpr int LIGHT_GROUP = 1;  // Frequent small checkpoints
   constexpr int HEAVY_GROUP = 2;  // Infrequent large checkpoints

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     namespace mlog = fenix::mlog;
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Application state
     struct LightState {
       int iteration;
       double convergence;
     } light;

     std::vector<double> heavy_data(N);

     // Tuned RAID parameters
     int raid_params[2] = {1, size / 2};  // k=1, n=size/2 for balance

     // Light state group: checkpoint frequently
     Fenix_Data_group_create(
       LIGHT_GROUP, res_comm, 0, 1,
       FENIX_DATA_POLICY_IN_MEMORY_RAID,
       raid_params, nullptr
     );
     data::member_create(LIGHT_GROUP, 0, &light, sizeof(light), MPI_BYTE);

     // Heavy data group: checkpoint infrequently
     Fenix_Data_group_create(
       HEAVY_GROUP, res_comm, 0, 1,
       FENIX_DATA_POLICY_IN_MEMORY_RAID,
       raid_params, nullptr
     );
     data::member_create(HEAVY_GROUP, 0, heavy_data.data(), N, MPI_DOUBLE);

     // Message logging for fast local recovery
     mlog::create(0, res_comm, MLOG_REGIONS);
     mlog::create_data_member(0, LIGHT_GROUP, 10);
     mlog::activate(0);
     fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

     // Initial checkpoint
     data::member_store(LIGHT_GROUP, 0, SUBSET_FULL);
     data::member_store(HEAVY_GROUP, 0, SUBSET_FULL);
     data::commit_barrier(LIGHT_GROUP);
     data::commit_barrier(HEAVY_GROUP);

     // Recovery callback
     fenix::callback_register([&](MPI_Comm comm, int err) {
       data::group_create(LIGHT_GROUP);
       // member_define to recreate member after group recreation
       data::member_define(LIGHT_GROUP, 0, &light, sizeof(light), MPI_BYTE);
       data::member_restore(LIGHT_GROUP, 0, NULL, 0);
       mlog::define_data_member(0, LIGHT_GROUP, 10);
       data::member_restore(LIGHT_GROUP, 10);
     });

     // Main loop
     for (int i = 0; i < MAX_ITER; i++) {
       try {
         mlog::begin_region(0, i);
         light.iteration = i;

         // Nearest-neighbor communication (logged)
         int left = (rank + size - 1) % size;
         int right = (rank + 1) % size;
         MPI_Sendrecv(&heavy_data[0], 1, MPI_DOUBLE, left, 0,
                     &heavy_data[N-1], 1, MPI_DOUBLE, right, 0,
                     res_comm, MPI_STATUS_IGNORE);

         // Computation
         for (int j = 1; j < N-1; j++) {
           heavy_data[j] = 0.25 * (heavy_data[j-1] + 2*heavy_data[j] +
                                   heavy_data[j+1]);
         }

         // Checkpoint light state frequently
         if (i % 10 == 0) {
           data::member_store(LIGHT_GROUP, 0, SUBSET_FULL);
           data::commit_barrier(LIGHT_GROUP);
         }

         // Checkpoint heavy data infrequently
         if (i % CHECKPOINT_FREQ == 0) {
           data::member_store(LIGHT_GROUP, 10, SUBSET_FULL);
           data::member_store(HEAVY_GROUP, 0, SUBSET_FULL);
           data::commit_barrier(HEAVY_GROUP);
         }

       } catch (fenix::CommException& e) {
         continue;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Troubleshooting Performance
----------------------------

**Problem: Checkpoint takes too long**

- Reduce data size with partial checkpoints
- Increase checkpoint interval
- Use fewer ranks in RAID group (increase n)
- Check network bandwidth

**Problem: High memory usage**

- Reduce snapshot depth to 1
- Use larger n in RAID parameters
- Use separate groups with different frequencies
- Use partial checkpoints

**Problem: Recovery is slow**

- Add message logging
- Reduce checkpoint interval (paradoxically, faster incremental recovery)
- Ensure enough spare ranks available
- Check if data is distributed efficiently

**Problem: Network congestion**

- Reduce n in RAID parameters (fewer ranks participate)
- Stagger checkpoints across ranks
- Use partial checkpoints
- Increase checkpoint interval

See Also
--------

- :doc:`checkpoint-data` - Basic checkpointing guide
- :doc:`partial-checkpoints` - Reduce checkpoint size
- :doc:`message-logging` - Fast recovery with message replay
- :doc:`optimize-checkpoints` - Advanced checkpoint optimization
- :doc:`/guides/imr-policy` - Understanding In-Memory RAID
- :doc:`/guides/data-recovery` - Data recovery concepts
