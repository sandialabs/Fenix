Test Fault Tolerance Locally
=============================

Testing fault-tolerant code is challenging because you need to simulate failures without actually killing your development environment. This guide shows you practical techniques for testing Fenix applications locally.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

The simplest way to test locally is to inject failures programmatically:

.. code-block:: cpp

   #include <signal.h>

   if (rank == 1 && iteration == 10) {
     printf("Rank %d failing at iteration %d\n", rank, iteration);
     raise(SIGKILL);  // Simulate sudden failure
   }

This guide covers more sophisticated techniques for reliable testing.

Testing Strategies
------------------

Strategy 1: Programmatic Failure Injection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best for:** Development, reproducible tests, CI/CD

**How it works:** Add conditional code that kills specific ranks at specific times.

**Basic Example:**

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <signal.h>
   #include <unistd.h>

   void inject_failure(int rank, int iteration) {
     // Kill rank 2 at iteration 10
     if (rank == 2 && iteration == 10) {
       printf("Injecting failure on rank %d at iteration %d\n",
              rank, iteration);
       raise(SIGKILL);
     }

     // Kill rank 5 at iteration 25
     if (rank == 5 && iteration == 25) {
       printf("Injecting failure on rank %d at iteration %d\n",
              rank, iteration);
       raise(SIGTERM);
     }
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     for (int i = 0; i < 100; i++) {
       inject_failure(rank, i);

       // Your application code
       MPI_Barrier(res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Advanced: Use Global Rank to Avoid Repeated Failures**

When a rank fails and is replaced by a spare, the new rank gets the same ``res_comm`` rank but a different global rank. Use global rank to ensure failures only happen once:

.. code-block:: cpp

   void inject_failure_once(int iteration) {
     // Use MPI_COMM_WORLD rank (global, doesn't change)
     int global_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

     if (global_rank == 2 && iteration == 10) {
       printf("Rank %d (global) failing\n", global_rank);
       raise(SIGKILL);
     }
   }

**Example with Multiple Staggered Failures:**

.. code-block:: cpp

   void inject_test_failures(int iteration, int n_ranks) {
     int global_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

     bool should_fail = false;

     // First failure: middle rank at iteration 18
     should_fail |= global_rank == n_ranks / 2 && iteration == 18;

     // Second failure: last rank at iteration 21
     should_fail |= global_rank == n_ranks - 1 && iteration == 21;

     // Third failure: rank 0 at iteration 78
     should_fail |= global_rank == 0 && iteration == 78;

     if (should_fail) {
       printf("Rank %d failing at iteration %d\n",
              global_rank, iteration);
       raise(SIGKILL);
     }
   }

Strategy 2: Environment Variable Control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best for:** Flexible testing, debugging, parameter sweeps

**How it works:** Control failure injection via environment variables so you don't need to recompile.

**Example:**

.. code-block:: cpp

   #include <cstdlib>
   #include <cstring>

   struct FailureConfig {
     int target_rank = -1;
     int target_iteration = -1;

     static FailureConfig from_env() {
       FailureConfig config;

       const char* rank_str = std::getenv("FENIX_TEST_FAIL_RANK");
       if (rank_str) {
         config.target_rank = std::atoi(rank_str);
       }

       const char* iter_str = std::getenv("FENIX_TEST_FAIL_ITER");
       if (iter_str) {
         config.target_iteration = std::atoi(iter_str);
       }

       return config;
     }

     bool should_fail(int rank, int iteration) const {
       return target_rank >= 0 &&
              rank == target_rank &&
              iteration == target_iteration;
     }
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     auto fail_config = FailureConfig::from_env();

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     for (int i = 0; i < 100; i++) {
       if (fail_config.should_fail(rank, i)) {
         printf("Configured failure: rank %d, iteration %d\n",
                rank, i);
         raise(SIGKILL);
       }

       // Application code
       MPI_Barrier(res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Usage:**

.. code-block:: bash

   # No failure
   mpiexec --with-ft mpi -n 8 ./my_app

   # Fail rank 3 at iteration 15
   FENIX_TEST_FAIL_RANK=3 FENIX_TEST_FAIL_ITER=15 \
     mpiexec --with-ft mpi -n 8 ./my_app

   # Test different failure scenarios
   for rank in 0 1 2 3; do
     for iter in 10 20 30; do
       echo "Testing rank=$rank iter=$iter"
       FENIX_TEST_FAIL_RANK=$rank FENIX_TEST_FAIL_ITER=$iter \
         mpiexec --with-ft mpi -n 8 ./my_app
     done
   done

Strategy 3: Dedicated Test Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best for:** Production code, clean separation of concerns

**How it works:** Add a ``--test-failures`` flag that enables failure injection only during testing.

**Example:**

.. code-block:: cpp

   #include <getopt.h>

   struct TestConfig {
     bool enable_failures = false;
     int failure_rank = -1;
     int failure_iteration = -1;

     static TestConfig parse_args(int argc, char** argv) {
       TestConfig config;

       static struct option long_options[] = {
         {"test-failures", no_argument, nullptr, 't'},
         {"fail-rank", required_argument, nullptr, 'r'},
         {"fail-iter", required_argument, nullptr, 'i'},
         {nullptr, 0, nullptr, 0}
       };

       int opt;
       while ((opt = getopt_long(argc, argv, "tr:i:",
                                 long_options, nullptr)) != -1) {
         switch (opt) {
           case 't':
             config.enable_failures = true;
             break;
           case 'r':
             config.failure_rank = std::atoi(optarg);
             break;
           case 'i':
             config.failure_iteration = std::atoi(optarg);
             break;
         }
       }

       return config;
     }
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     auto config = TestConfig::parse_args(argc, argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     for (int i = 0; i < 100; i++) {
       // Only inject failures in test mode
       if (config.enable_failures &&
           rank == config.failure_rank &&
           i == config.failure_iteration) {
         printf("Test failure: rank %d, iteration %d\n", rank, i);
         raise(SIGKILL);
       }

       // Normal application code
       MPI_Barrier(res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**Usage:**

.. code-block:: bash

   # Production run - no failures
   mpiexec --with-ft mpi -n 8 ./my_app

   # Test run with failure
   mpiexec --with-ft mpi -n 8 ./my_app \
     --test-failures --fail-rank 2 --fail-iter 15

Strategy 4: Random Failure Injection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best for:** Stress testing, finding race conditions

**How it works:** Randomly kill ranks with some probability to test recovery under unpredictable conditions.

**Example:**

.. code-block:: cpp

   #include <random>
   #include <chrono>

   class RandomFailureInjector {
     std::mt19937 rng;
     std::uniform_real_distribution<double> dist{0.0, 1.0};
     double failure_probability;
     int min_iteration;
     int max_iteration;
     bool already_failed = false;

   public:
     RandomFailureInjector(double prob, int min_iter, int max_iter)
       : rng(std::chrono::steady_clock::now().time_since_epoch().count()),
         failure_probability(prob),
         min_iteration(min_iter),
         max_iteration(max_iter) {}

     bool should_fail(int iteration) {
       if (already_failed) return false;
       if (iteration < min_iteration) return false;
       if (iteration > max_iteration) return false;

       if (dist(rng) < failure_probability) {
         already_failed = true;
         return true;
       }
       return false;
     }
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 3});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     // Each rank has 5% chance to fail between iterations 10-50
     RandomFailureInjector injector(0.05, 10, 50);

     for (int i = 0; i < 100; i++) {
       if (injector.should_fail(i)) {
         printf("Random failure on rank %d at iteration %d\n",
                rank, i);
         raise(SIGKILL);
       }

       MPI_Barrier(res_comm);
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Debugging Techniques
--------------------

Debug with Verbose Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add detailed logging to trace recovery behavior:

.. code-block:: cpp

   #include <cstdio>

   #define LOG_RECOVERY(...) \
     do { \
       int rank; \
       MPI_Comm_rank(MPI_COMM_WORLD, &rank); \
       printf("[Rank %d] ", rank); \
       printf(__VA_ARGS__); \
       printf("\n"); \
       fflush(stdout); \
     } while(0)

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     if (fenix::role() == fenix::INITIAL_RANK) {
       LOG_RECOVERY("Starting as initial rank");
     } else {
       LOG_RECOVERY("Starting as recovered rank");
     }

     fenix::callback_register([](MPI_Comm comm, int err) {
       LOG_RECOVERY("Recovery callback triggered, error=%d", err);
     });

     for (int i = 0; i < 100; i++) {
       LOG_RECOVERY("Iteration %d", i);
       // ... application code ...
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Verify Recovery with Checksums
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Ensure recovered state matches expected state:

.. code-block:: cpp

   uint64_t compute_checksum(const double* data, size_t count) {
     uint64_t sum = 0;
     for (size_t i = 0; i < count; i++) {
       sum += static_cast<uint64_t>(data[i] * 1000000);
     }
     return sum;
   }

   void verify_recovery(const double* data, size_t count,
                       uint64_t expected_checksum) {
     uint64_t actual = compute_checksum(data, count);
     if (actual != expected_checksum) {
       int rank;
       MPI_Comm_rank(MPI_COMM_WORLD, &rank);
       printf("CHECKSUM MISMATCH on rank %d: expected %lu, got %lu\n",
              rank, expected_checksum, actual);
     }
   }

Test Recovery at Different Points
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test failures at various stages of your computation:

.. code-block:: cpp

   enum class ComputePhase {
     INIT,
     COMMUNICATION,
     COMPUTATION,
     CHECKPOINT,
     FINALIZE
   };

   void test_failure_in_phase(ComputePhase target_phase,
                             ComputePhase current_phase,
                             int rank, int iteration) {
     if (current_phase == target_phase &&
         rank == 1 && iteration == 10) {
       printf("Failing in phase: %d\n",
              static_cast<int>(current_phase));
       raise(SIGKILL);
     }
   }

CI/CD Integration
-----------------

Running in Continuous Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**CMake/CTest Configuration:**

Add tests that inject failures:

.. code-block:: cmake

   # In CMakeLists.txt
   enable_testing()

   # Test without failures
   add_test(NAME my_app_no_failure
            COMMAND ${MPIEXEC} --with-ft mpi
                    ${MPIEXEC_NUMPROC_FLAG} 8
                    ${MPIEXEC_PREFLAGS}
                    $<TARGET_FILE:my_app>)

   # Test with rank 2 failure
   add_test(NAME my_app_rank2_failure
            COMMAND ${MPIEXEC} --with-ft mpi
                    ${MPIEXEC_NUMPROC_FLAG} 8
                    ${MPIEXEC_PREFLAGS}
                    $<TARGET_FILE:my_app>
                    --test-failures --fail-rank 2 --fail-iter 15)

   # Test with multiple failures
   add_test(NAME my_app_multiple_failures
            COMMAND ${MPIEXEC} --with-ft mpi
                    ${MPIEXEC_NUMPROC_FLAG} 10
                    ${MPIEXEC_PREFLAGS}
                    $<TARGET_FILE:my_app>
                    --test-failures --fail-rank 2 --fail-iter 10)

   # Set timeout for fault tolerance tests
   set_tests_properties(
     my_app_rank2_failure
     my_app_multiple_failures
     PROPERTIES TIMEOUT 60
   )

**Run Tests:**

.. code-block:: bash

   cd build
   ctest -V --timeout 60

**Repeat Tests to Catch Flaky Failures:**

.. code-block:: bash

   # Run each test 10 times
   ctest -V --repeat until-fail:10

GitHub Actions Example
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: yaml

   # .github/workflows/test-fault-tolerance.yml
   name: Fault Tolerance Tests

   on: [push, pull_request]

   jobs:
     test:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v3

         - name: Install Open MPI with ULFM
           run: |
             # Install Open MPI 5+ with ULFM support
             # (see installation guide for details)

         - name: Build Fenix
           run: |
             mkdir build && cd build
             cmake ../ \
               -DCMAKE_C_COMPILER=mpicc \
               -DCMAKE_CXX_COMPILER=mpicxx \
               -DBUILD_TESTING=ON
             make -j4

         - name: Run Tests
           run: |
             cd build
             ctest -V --timeout 60

         - name: Run Stress Tests
           run: |
             cd build
             ctest -V --timeout 60 --repeat until-fail:5

Common Testing Patterns
-----------------------

Pattern: Test Each Recovery Path
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void test_initial_rank_path() {
     // Test code that only initial ranks execute
   }

   void test_recovered_rank_path() {
     // Test code that only recovered ranks execute
   }

   void test_survivor_rank_path() {
     // Test code that survivor ranks execute
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     switch (fenix::role()) {
       case fenix::INITIAL_RANK:
         test_initial_rank_path();
         break;
       case fenix::RECOVERED_RANK:
         test_recovered_rank_path();
         break;
       case fenix::SURVIVOR_RANK:
         test_survivor_rank_path();
         break;
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Pattern: Test Cascading Failures
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void inject_cascading_failures(int iteration, int n_ranks) {
     int global_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

     // First wave of failures
     if (iteration == 10 && global_rank < n_ranks / 2) {
       printf("First failure: rank %d\n", global_rank);
       raise(SIGKILL);
     }

     // Second wave during recovery
     if (iteration == 11 && global_rank >= n_ranks / 2 &&
         global_rank < n_ranks * 3 / 4) {
       printf("Cascading failure: rank %d\n", global_rank);
       raise(SIGKILL);
     }
   }

Pattern: Test Checkpoint/Restore
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   bool verify_checkpoint_restore(int rank, int iteration) {
     // Store known data
     double test_data[100];
     for (int i = 0; i < 100; i++) {
       test_data[i] = rank * 1000.0 + iteration + i;
     }

     const int GROUP = 99;
     const int MEMBER = 99;

     // Checkpoint
     fenix::data::group_create(GROUP);
     fenix::data::member_create(GROUP, MEMBER,
                               test_data, 100, MPI_DOUBLE);
     fenix::data::member_store(GROUP, MEMBER, SUBSET_FULL);
     fenix::data::commit_barrier(GROUP);

     // Restore
     double restored[100] = {0};
     fenix::data::member_restore(GROUP, MEMBER);
     std::memcpy(restored, test_data, sizeof(test_data));

     // Verify
     for (int i = 0; i < 100; i++) {
       if (std::abs(restored[i] - test_data[i]) > 1e-10) {
         printf("Checkpoint verify failed at index %d\n", i);
         return false;
       }
     }

     return true;
   }

Troubleshooting
---------------

**Problem: Failures don't trigger recovery**

- Check that you're using ``--with-ft mpi`` flag with mpiexec
- Verify Open MPI has ULFM support: ``ompi_info | grep ft``
- Ensure you have spare ranks configured

**Problem: Tests hang after injected failure**

- Check that all ranks participate in collective operations
- Verify that checkpointing is complete before failure
- Use ``MPI_Barrier`` to synchronize before killing ranks

**Problem: Recovery succeeds but results are wrong**

- Add checksum verification to validate restored data
- Check that your checkpoint includes all necessary state
- Verify that callbacks restore all application state

**Problem: Tests pass individually but fail when run together**

- Ensure each test properly cleans up resources
- Check for race conditions in failure timing
- Use different random seeds for each test run

**Problem: Can't reproduce failures**

- Use fixed failure points instead of random injection
- Add verbose logging to trace execution
- Use global rank instead of resilient communicator rank

See Also
--------

- :doc:`choose-recovery-pattern` - Choose the right recovery approach
- :doc:`checkpoint-data` - How to checkpoint application state
- :doc:`/troubleshooting` - Common issues and solutions
- :doc:`/api/process-recovery` - Process recovery API reference
- :doc:`/installation` - Building Open MPI with ULFM support
