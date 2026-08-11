Custom Recovery Strategies
==========================

This guide shows you when and how to implement custom recovery strategies beyond Fenix's built-in checkpoint/restart. Learn how to use interpolation, external libraries, and hybrid approaches for specialized recovery needs.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Example: Recover using interpolation from neighbors instead of checkpoints:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   constexpr int N = 1000;

   void recover_from_neighbors(double* data, int rank, MPI_Comm comm) {
     int size;
     MPI_Comm_size(comm, &size);

     int left = (rank + size - 1) % size;
     int right = (rank + 1) % size;

     // Request data from neighbors
     double left_data[N], right_data[N];
     MPI_Sendrecv(data, N, MPI_DOUBLE, left, 0,
                 right_data, N, MPI_DOUBLE, right, 0, comm, MPI_STATUS_IGNORE);
     MPI_Sendrecv(data, N, MPI_DOUBLE, right, 0,
                 left_data, N, MPI_DOUBLE, left, 0, comm, MPI_STATUS_IGNORE);

     // Interpolate to recover
     for (int i = 0; i < N; i++) {
       data[i] = 0.5 * (left_data[i] + right_data[i]);
     }
   }

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     double data[N];

     if (fenix::role() == fenix::RECOVERED_RANK) {
       // Custom recovery: interpolate from neighbors
       recover_from_neighbors(data, rank, res_comm);
       printf("Rank %d recovered using neighbor interpolation\n", rank);
     }

     // Register callback for inline failures
     fenix::callback_register([&](MPI_Comm comm, int err) {
       recover_from_neighbors(data, rank, comm);
     });

     // Main loop
     // ...
   }

When to Use Custom Recovery
----------------------------

Built-In Recovery is Best When
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- General-purpose applications
- Complete state must be preserved exactly
- No domain-specific recovery knowledge
- Simplicity is valued

Custom Recovery is Better When
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Mathematical Properties Allow Reconstruction:**

- Interpolation from neighbors (stencil codes, PDEs)
- Recomputation from boundary conditions
- Statistical reconstruction (Monte Carlo, machine learning)

**Specialized Data Structures:**

- Adaptive meshes (can be regenerated)
- Octrees/quadtrees (can be rebuilt)
- Sparse matrices (can be recomputed)

**External Dependencies:**

- Coupling with non-Fenix libraries
- Integration with existing checkpoint systems
- Hardware-accelerated storage

**Performance Critical:**

- Very large state (TB+)
- Very frequent checkpoints needed
- Recovery time must be minimized
- Memory is severely constrained

**Application-Specific Tradeoffs:**

- Acceptable to lose precision
- Faster approximate recovery preferred
- Domain knowledge enables better recovery

Alternative Recovery Approaches
--------------------------------

Interpolation from Neighbors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best for:** Stencil computations, PDE solvers, structured grids

**Concept:** Reconstruct failed rank's data from neighbors using domain knowledge.

.. code-block:: cpp

   class StencilRecovery {
     double* data_;
     int n_;
     MPI_Comm comm_;
     int rank_, size_;

   public:
     StencilRecovery(double* data, int n, MPI_Comm comm)
       : data_(data), n_(n), comm_(comm) {
       MPI_Comm_rank(comm, &rank_);
       MPI_Comm_size(comm, &size_);
     }

     void recover_linear_interpolation() {
       // Get neighbors
       int left = (rank_ + size_ - 1) % size_;
       int right = (rank_ + 1) % size_;

       // Exchange boundary data
       double left_boundary[100], right_boundary[100];
       MPI_Sendrecv(&data_[0], 100, MPI_DOUBLE, left, 0,
                   right_boundary, 100, MPI_DOUBLE, right, 0,
                   comm_, MPI_STATUS_IGNORE);
       MPI_Sendrecv(&data_[n_-100], 100, MPI_DOUBLE, right, 0,
                   left_boundary, 100, MPI_DOUBLE, left, 0,
                   comm_, MPI_STATUS_IGNORE);

       // Linear interpolation
       for (int i = 0; i < n_; i++) {
         double t = static_cast<double>(i) / n_;
         data_[i] = (1 - t) * left_boundary[i % 100] +
                    t * right_boundary[i % 100];
       }

       printf("Rank %d recovered via linear interpolation\n", rank_);
     }

     void recover_laplacian_smoothing() {
       // Get neighbors
       int left = (rank_ + size_ - 1) % size_;
       int right = (rank_ + 1) % size_;

       // Get full neighbor data
       std::vector<double> left_data(n_), right_data(n_);
       MPI_Sendrecv(data_, n_, MPI_DOUBLE, left, 0,
                   right_data.data(), n_, MPI_DOUBLE, right, 0,
                   comm_, MPI_STATUS_IGNORE);
       MPI_Sendrecv(data_, n_, MPI_DOUBLE, right, 0,
                   left_data.data(), n_, MPI_DOUBLE, left, 0,
                   comm_, MPI_STATUS_IGNORE);

       // Apply Laplacian smoothing operator
       for (int iter = 0; iter < 10; iter++) {
         std::vector<double> new_data(n_);
         new_data[0] = 0.5 * (left_data[n_-1] + data_[1]);
         for (int i = 1; i < n_-1; i++) {
           new_data[i] = 0.25 * (data_[i-1] + 2*data_[i] + data_[i+1]);
         }
         new_data[n_-1] = 0.5 * (data_[n_-2] + right_data[0]);
         std::copy(new_data.begin(), new_data.end(), data_);
       }

       printf("Rank %d recovered via Laplacian smoothing\n", rank_);
     }
   };

Recomputation from Boundaries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Best for:** Time-stepping codes, explicit methods

.. code-block:: cpp

   class TimeSteppingRecovery {
     double* state_;
     int n_;
     int timestep_;
     MPI_Comm comm_;

   public:
     void recover_from_checkpoint(int checkpoint_timestep) {
       // Roll back to last checkpoint (all ranks)
       restore_checkpoint(checkpoint_timestep);

       // Re-execute timesteps from checkpoint to current
       for (int t = checkpoint_timestep; t < timestep_; t++) {
         perform_timestep();
       }

       printf("Recovered by recomputing %d timesteps\n",
              timestep_ - checkpoint_timestep);
     }

     void recover_from_initial_conditions() {
       // Get rank's initial conditions from rank 0
       broadcast_initial_conditions();

       // Recompute all timesteps
       for (int t = 0; t < timestep_; t++) {
         perform_timestep();
       }

       printf("Recovered by recomputing from initial conditions\n");
     }

   private:
     void perform_timestep() {
       // Exchange ghost cells
       exchange_boundaries();

       // Update interior
       for (int i = 1; i < n_-1; i++) {
         state_[i] = compute_next_value(state_, i);
       }

       timestep_++;
     }

     void exchange_boundaries() {
       // Exchange with neighbors
       // ...
     }

     double compute_next_value(double* state, int i) {
       // Application-specific computation
       return 0.25 * (state[i-1] + 2*state[i] + state[i+1]);
     }
   };

Statistical Recovery
~~~~~~~~~~~~~~~~~~~~

**Best for:** Monte Carlo simulations, stochastic methods

.. code-block:: cpp

   class MonteCarloRecovery {
     std::vector<double> samples_;
     int n_samples_;
     uint64_t rng_state_;
     MPI_Comm comm_;

   public:
     void recover_redistribute() {
       int rank, size;
       MPI_Comm_rank(comm_, &rank);
       MPI_Comm_size(comm_, &size);

       // Collect sample counts from all ranks
       std::vector<int> all_counts(size);
       int my_count = samples_.size();
       MPI_Allgather(&my_count, 1, MPI_INT,
                    all_counts.data(), 1, MPI_INT, comm_);

       // Calculate target per rank
       int total_samples = std::accumulate(all_counts.begin(),
                                          all_counts.end(), 0);
       int target = total_samples / size;

       // Redistribute samples to recovered rank
       // ... redistribution logic ...

       printf("Recovered by redistributing %d samples\n", target);
     }

     void recover_resample() {
       // Generate new samples with proper RNG state
       reset_rng_to_checkpoint();

       samples_.clear();
       for (int i = 0; i < n_samples_; i++) {
         samples_.push_back(generate_sample());
       }

       printf("Recovered by regenerating %d samples\n", n_samples_);
     }

   private:
     void reset_rng_to_checkpoint() {
       // Restore RNG state from checkpoint
       // ...
     }

     double generate_sample() {
       // Generate sample using RNG
       // ...
       return 0.0;
     }
   };

External Library Integration
-----------------------------

VeloC Integration
~~~~~~~~~~~~~~~~~

`VeloC <https://github.com/ECP-VeloC/VELOC>`_ is a multi-level checkpoint/restart library. Use with Fenix for hybrid recovery:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <veloc.h>

   class HybridVeloCRecovery {
     int veloc_id_;
     MPI_Comm comm_;

   public:
     HybridVeloCRecovery(const char* config, MPI_Comm comm)
       : comm_(comm) {
       // Initialize VeloC
       VELOC_Init(MPI_COMM_WORLD, config);
       veloc_id_ = 0;
     }

     void checkpoint_to_veloc(void* data, size_t size) {
       // Register memory with VeloC
       VELOC_Mem_protect(veloc_id_, data, size, 0);

       // Checkpoint to persistent storage
       VELOC_Checkpoint("app_checkpoint", veloc_id_);

       printf("Checkpointed to VeloC (persistent storage)\n");
     }

     bool recover_from_veloc(void* data, size_t size) {
       // Check if VeloC has a checkpoint
       int version = VELOC_Restart_test("app_checkpoint", veloc_id_);

       if (version > 0) {
         // Restart from persistent storage
         VELOC_Restart("app_checkpoint", veloc_id_);
         printf("Recovered from VeloC checkpoint version %d\n", version);
         return true;
       }

       return false;
     }

     ~HybridVeloCRecovery() {
       VELOC_Finalize(0);
     }
   };

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     HybridVeloCRecovery veloc("veloc.cfg", res_comm);

     double my_data[10000];

     if (fenix::role() == fenix::RECOVERED_RANK) {
       // Try VeloC first (fast, persistent)
       if (!veloc.recover_from_veloc(my_data, sizeof(my_data))) {
         // Fall back to Fenix in-memory checkpoint
         data::group_create(0);
         data::member_define(0, 0, my_data, 10000, MPI_DOUBLE);
         data::member_restore(0, 0);
       }
     }

     // Dual checkpoint: Fenix (fast) + VeloC (persistent)
     for (int i = 0; i < 1000; i++) {
       // ... computation ...

       if (i % 10 == 0) {
         // Fast Fenix in-memory checkpoint
         data::member_store(0, 0, SUBSET_FULL);
         data::commit_barrier(0);
       }

       if (i % 100 == 0) {
         // Slow but persistent VeloC checkpoint
         veloc.checkpoint_to_veloc(my_data, sizeof(my_data));
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

HDF5 Integration
~~~~~~~~~~~~~~~~

Use HDF5 for portable, parallel checkpoints:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <hdf5.h>

   class HDF5Recovery {
     const char* filename_;
     MPI_Comm comm_;

   public:
     HDF5Recovery(const char* filename, MPI_Comm comm)
       : filename_(filename), comm_(comm) {}

     void checkpoint_to_hdf5(double* data, int n, int iteration) {
       // Create HDF5 file
       hid_t file_id = H5Fcreate(filename_, H5F_ACC_TRUNC,
                                 H5P_DEFAULT, H5P_DEFAULT);

       // Create dataset
       hsize_t dims[1] = {static_cast<hsize_t>(n)};
       hid_t dataspace = H5Screate_simple(1, dims, NULL);
       hid_t dataset = H5Dcreate(file_id, "data", H5T_NATIVE_DOUBLE,
                                dataspace, H5P_DEFAULT, H5P_DEFAULT,
                                H5P_DEFAULT);

       // Write data
       H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
               H5P_DEFAULT, data);

       // Store iteration as attribute
       hid_t attr_space = H5Screate(H5S_SCALAR);
       hid_t attr = H5Acreate(dataset, "iteration", H5T_NATIVE_INT,
                             attr_space, H5P_DEFAULT, H5P_DEFAULT);
       H5Awrite(attr, H5T_NATIVE_INT, &iteration);

       // Cleanup
       H5Aclose(attr);
       H5Sclose(attr_space);
       H5Dclose(dataset);
       H5Sclose(dataspace);
       H5Fclose(file_id);

       printf("Checkpointed to HDF5: %s (iteration %d)\n",
              filename_, iteration);
     }

     bool recover_from_hdf5(double* data, int n, int* iteration) {
       // Check if file exists
       if (H5Fis_hdf5(filename_) <= 0) {
         return false;
       }

       // Open file
       hid_t file_id = H5Fopen(filename_, H5F_ACC_RDONLY, H5P_DEFAULT);
       hid_t dataset = H5Dopen(file_id, "data", H5P_DEFAULT);

       // Read data
       H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
              H5P_DEFAULT, data);

       // Read iteration attribute
       hid_t attr = H5Aopen(dataset, "iteration", H5P_DEFAULT);
       H5Aread(attr, H5T_NATIVE_INT, iteration);

       // Cleanup
       H5Aclose(attr);
       H5Dclose(dataset);
       H5Fclose(file_id);

       printf("Recovered from HDF5: %s (iteration %d)\n",
              filename_, *iteration);
       return true;
     }
   };

Custom Callback Logic
---------------------

Conditional Recovery
~~~~~~~~~~~~~~~~~~~~

Choose recovery strategy based on conditions:

.. code-block:: cpp

   void register_conditional_recovery() {
     fenix::callback_register([&](MPI_Comm comm, int err) {
       int rank;
       MPI_Comm_rank(comm, &rank);

       // Check which data is available
       bool fenix_checkpoint_available = data::group_created(GROUP_ID);
       bool external_checkpoint_available = check_external_checkpoint();

       if (fenix_checkpoint_available) {
         // Fast recovery from Fenix
         recover_from_fenix();
         printf("Rank %d: recovered from Fenix checkpoint\n", rank);

       } else if (external_checkpoint_available) {
         // Fall back to external checkpoint
         recover_from_external();
         printf("Rank %d: recovered from external checkpoint\n", rank);

       } else {
         // Last resort: recompute
         recover_from_initial_conditions();
         printf("Rank %d: recovered by recomputing\n", rank);
       }
     });
   }

Multi-Level Recovery
~~~~~~~~~~~~~~~~~~~~

Try fast recovery first, fall back to slower methods:

.. code-block:: cpp

   class MultiLevelRecovery {
   public:
     void register_recovery() {
       fenix::callback_register([this](MPI_Comm comm, int err) {
         try_fast_recovery() ||
         try_medium_recovery() ||
         try_slow_recovery() ||
         abort_recovery();
       });
     }

   private:
     bool try_fast_recovery() {
       // Level 1: Neighbor interpolation (milliseconds)
       try {
         if (can_interpolate_from_neighbors()) {
           interpolate_from_neighbors();
           printf("Fast recovery: interpolation\n");
           return true;
         }
       } catch (...) {}
       return false;
     }

     bool try_medium_recovery() {
       // Level 2: Fenix in-memory checkpoint (seconds)
       try {
         if (data::group_created(GROUP_ID)) {
           // Define member with buffer pointer (replace with actual data pointer)
           data::member_define(GROUP_ID, MEMBER_ID, data_ptr, count, datatype);
           data::member_restore(GROUP_ID, MEMBER_ID);
           printf("Medium recovery: Fenix checkpoint\n");
           return true;
         }
       } catch (...) {}
       return false;
     }

     bool try_slow_recovery() {
       // Level 3: Recompute from boundary conditions (minutes)
       try {
         recompute_from_boundaries();
         printf("Slow recovery: recomputation\n");
         return true;
       } catch (...) {}
       return false;
     }

     void abort_recovery() {
       fprintf(stderr, "All recovery methods failed\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
     }
   };

Hybrid Approaches
-----------------

Fenix + Application-Specific Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Combine Fenix checkpoints with domain knowledge:

.. code-block:: cpp

   class HybridStencilRecovery {
     double* data_;
     int n_;
     const int GROUP_ID = 0;
     const int MEMBER_ID = 0;

   public:
     void initialize() {
       // Use Fenix for core data
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, MEMBER_ID, data_, n_, MPI_DOUBLE);

       // Register hybrid recovery
       fenix::callback_register([this](MPI_Comm comm, int err) {
         // First, restore from Fenix checkpoint
         data::group_create(GROUP_ID);
         data::member_define(GROUP_ID, MEMBER_ID, data_, n_, MPI_DOUBLE);
         data::member_restore(GROUP_ID, MEMBER_ID);

         // Then, refine using application knowledge
         refine_with_laplacian_smoothing();

         printf("Hybrid recovery: Fenix + smoothing\n");
       });
     }

     void checkpoint() {
       // Coarse checkpoint with Fenix (less frequent)
       data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     }

   private:
     void refine_with_laplacian_smoothing() {
       // Apply a few smoothing iterations to improve accuracy
       for (int iter = 0; iter < 5; iter++) {
         apply_laplacian();
       }
     }

     void apply_laplacian() {
       // Smooth the data using Laplacian operator
       std::vector<double> temp(n_);
       for (int i = 1; i < n_-1; i++) {
         temp[i] = 0.25 * (data_[i-1] + 2*data_[i] + data_[i+1]);
       }
       std::copy(temp.begin() + 1, temp.end() - 1, data_ + 1);
     }
   };

Tiered Storage
~~~~~~~~~~~~~~

Use different storage tiers for different recovery scenarios:

.. code-block:: cpp

   class TieredRecovery {
   public:
     enum Tier {
       MEMORY,      // Fenix in-memory (fastest, volatile)
       NVRAM,       // Non-volatile RAM (fast, persistent)
       SSD,         // Local SSD (moderate, persistent)
       PARALLEL_FS  // Parallel filesystem (slow, persistent)
     };

     void checkpoint_tiered(int iteration) {
       // Every iteration: memory
       checkpoint_to_tier(MEMORY);

       // Every 10 iterations: NVRAM
       if (iteration % 10 == 0) {
         checkpoint_to_tier(NVRAM);
       }

       // Every 100 iterations: SSD
       if (iteration % 100 == 0) {
         checkpoint_to_tier(SSD);
       }

       // Every 1000 iterations: parallel FS
       if (iteration % 1000 == 0) {
         checkpoint_to_tier(PARALLEL_FS);
       }
     }

     void recover_tiered() {
       // Try fastest tier first
       if (recover_from_tier(MEMORY)) return;
       if (recover_from_tier(NVRAM)) return;
       if (recover_from_tier(SSD)) return;
       if (recover_from_tier(PARALLEL_FS)) return;

       // All tiers failed
       fprintf(stderr, "Recovery failed from all tiers\n");
       MPI_Abort(MPI_COMM_WORLD, 1);
     }

   private:
     bool recover_from_tier(Tier tier) {
       switch (tier) {
         case MEMORY:
           return recover_from_fenix();
         case NVRAM:
           return recover_from_nvram();
         case SSD:
           return recover_from_ssd();
         case PARALLEL_FS:
           return recover_from_pfs();
       }
       return false;
     }

     void checkpoint_to_tier(Tier tier) {
       // Implementation for each tier
       // ...
     }

     bool recover_from_fenix() {
       try {
         // Define member with buffer pointer (replace with actual data pointer)
         data::member_define(GROUP_ID, MEMBER_ID, data_ptr, count, datatype);
         data::member_restore(GROUP_ID, MEMBER_ID);
         return true;
       } catch (...) {
         return false;
       }
     }

     bool recover_from_nvram() { /* ... */ return false; }
     bool recover_from_ssd() { /* ... */ return false; }
     bool recover_from_pfs() { /* ... */ return false; }
   };

Complete Custom Recovery Example
---------------------------------

Stencil Code with Interpolation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <cmath>

   constexpr int N = 10000;
   constexpr int MAX_ITER = 1000;
   constexpr int CHECKPOINT_FREQ = 100;  // Infrequent checkpoint

   class CustomStencilRecovery {
     std::vector<double> data_;
     int rank_, size_;
     MPI_Comm comm_;
     const int GROUP_ID = 0;

   public:
     CustomStencilRecovery(int rank, int size, MPI_Comm comm)
       : rank_(rank), size_(size), comm_(comm), data_(N, 0.0) {}

     void initialize() {
       // Initialize data
       for (int i = 0; i < N; i++) {
         data_[i] = sin(2.0 * M_PI * i / N) + rank_;
       }

       // Create Fenix checkpoint group (infrequent use)
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, 0, data_.data(), N, MPI_DOUBLE);
       checkpoint();

       // Register custom recovery callback
       fenix::callback_register([this](MPI_Comm comm, int err) {
         this->recover_custom();
       });
     }

     void recover_custom() {
       // Try interpolation first (fast, approximate)
       if (try_interpolation_recovery()) {
         printf("Rank %d: recovered via interpolation\n", rank_);
         return;
       }

       // Fall back to Fenix checkpoint (slow, exact)
       try {
         data::group_create(GROUP_ID);
         data::member_define(GROUP_ID, 0, data_.data(), N, MPI_DOUBLE);
         data::member_restore(GROUP_ID, 0);
         printf("Rank %d: recovered from checkpoint\n", rank_);
       } catch (...) {
         fprintf(stderr, "Rank %d: all recovery failed\n", rank_);
         MPI_Abort(MPI_COMM_WORLD, 1);
       }
     }

     bool try_interpolation_recovery() {
       try {
         int left = (rank_ + size_ - 1) % size_;
         int right = (rank_ + 1) % size_;

         // Get neighbor data
         std::vector<double> left_data(N), right_data(N);

         // Since we're recovered, send zeros, receive real data
         std::vector<double> zeros(N, 0.0);
         MPI_Sendrecv(zeros.data(), N, MPI_DOUBLE, left, 0,
                     right_data.data(), N, MPI_DOUBLE, right, 0,
                     comm_, MPI_STATUS_IGNORE);
         MPI_Sendrecv(zeros.data(), N, MPI_DOUBLE, right, 0,
                     left_data.data(), N, MPI_DOUBLE, left, 0,
                     comm_, MPI_STATUS_IGNORE);

         // Linear interpolation
         for (int i = 0; i < N; i++) {
           data_[i] = 0.5 * (left_data[i] + right_data[i]);
         }

         // Refine with smoothing
         for (int iter = 0; iter < 10; iter++) {
           std::vector<double> temp(N);
           temp[0] = left_data[N-1];
           temp[N-1] = right_data[0];
           for (int i = 1; i < N-1; i++) {
             temp[i] = 0.25 * (data_[i-1] + 2*data_[i] + data_[i+1]);
           }
           data_ = temp;
         }

         return true;

       } catch (...) {
         return false;
       }
     }

     void checkpoint() {
       data::member_store(GROUP_ID, 0, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     }

     void step() {
       // Stencil computation
       exchange_boundaries();

       std::vector<double> new_data(N);
       for (int i = 1; i < N-1; i++) {
         new_data[i] = 0.25 * (data_[i-1] + 2*data_[i] + data_[i+1]);
       }
       data_ = new_data;
     }

     double* data() { return data_.data(); }

   private:
     void exchange_boundaries() {
       int left = (rank_ + size_ - 1) % size_;
       int right = (rank_ + 1) % size_;

       MPI_Sendrecv(&data_[0], 1, MPI_DOUBLE, left, 0,
                   &data_[N-1], 1, MPI_DOUBLE, right, 0,
                   comm_, MPI_STATUS_IGNORE);
     }
   };

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});
     fenix::set_option(fenix::RESUME_MODE, fenix::RESUME_THROW);

     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     CustomStencilRecovery recovery(rank, size, res_comm);

     if (fenix::role() == fenix::INITIAL_RANK) {
       recovery.initialize();
     } else {
       recovery.recover_custom();
     }

     // Main loop
     for (int i = 0; i < MAX_ITER; i++) {
       try {
         recovery.step();

         // Infrequent checkpoint (rely on interpolation for recovery)
         if (i % CHECKPOINT_FREQ == 0) {
           recovery.checkpoint();
         }

       } catch (fenix::CommException& e) {
         continue;
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Troubleshooting
---------------

**Problem: Custom recovery produces incorrect results**

- Verify interpolation assumptions are valid
- Check boundary conditions are correct
- Test recovery against known checkpoint
- Add validation/verification step

**Problem: Custom recovery is slower than expected**

- Profile to identify bottlenecks
- Reduce communication in recovery path
- Consider caching neighbor data
- Use asynchronous communication

**Problem: Recovery fails when Fenix checkpoint unavailable**

- Implement fallback to alternate recovery method
- Increase Fenix checkpoint frequency
- Use tiered storage approach
- Add error handling and abort if necessary

**Problem: Neighbors don't have needed data**

- Check if neighbors survived the failure
- Use all-gather to find data holders
- Fall back to global checkpoint
- Implement redundant data storage

See Also
--------

- :doc:`checkpoint-data` - Using Fenix's built-in checkpointing
- :doc:`inline-recovery-callbacks` - Setting up recovery callbacks
- :doc:`performance-tuning` - Optimizing recovery performance
- :doc:`/guides/data-recovery` - Understanding data recovery concepts
- :doc:`/api/data-recovery` - Data recovery API reference
