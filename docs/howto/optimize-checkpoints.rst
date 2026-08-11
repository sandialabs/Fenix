Optimize Checkpoint Performance
================================

This guide covers advanced techniques for optimizing checkpoint performance including incremental checkpointing, dirty bit tracking, adaptive intervals, and compression strategies. Use these techniques when basic checkpointing causes too much overhead.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Here's a quick example using dirty bit tracking to avoid unnecessary checkpoints:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <bitset>

   constexpr int N = 10000;
   constexpr int BLOCK_SIZE = 100;
   constexpr int NUM_BLOCKS = N / BLOCK_SIZE;

   std::bitset<NUM_BLOCKS> dirty_blocks;  // Track which blocks changed
   double data[N];

   void mark_dirty(int index) {
     dirty_blocks[index / BLOCK_SIZE] = true;
   }

   void checkpoint_dirty_blocks() {
     if (dirty_blocks.none()) {
       return;  // Nothing changed, skip checkpoint
     }

     // Checkpoint only dirty blocks
     for (int i = 0; i < NUM_BLOCKS; i++) {
       if (dirty_blocks[i]) {
         int start = i * BLOCK_SIZE;
         int end = start + BLOCK_SIZE - 1;

         Fenix_Data_subset subset;
         Fenix_Data_subset_create(start, end, &subset);
         data::member_store(GROUP_ID, MEMBER_ID, subset);
       }
     }

     data::commit_barrier(GROUP_ID);
     dirty_blocks.reset();  // Clear dirty bits
   }

.. _incremental-checkpointing:

Incremental Checkpointing
--------------------------

Concept
~~~~~~~

Incremental checkpointing stores only the data that has changed since the last checkpoint, reducing both memory and time overhead.

**Benefits:**

- Reduced checkpoint time (only changed data)
- Lower network bandwidth usage
- Smaller memory footprint
- Better cache utilization

**When to use:**

- Large data structures with localized changes
- Sparse updates (few elements change per iteration)
- Memory-constrained systems

Dirty Bit Tracking
~~~~~~~~~~~~~~~~~~

Track which portions of your data have been modified:

.. code-block:: cpp

   #include <bitset>
   #include <vector>

   template<typename T>
   class TrackedArray {
     std::vector<T> data_;
     std::bitset<1024> dirty_bits_;  // Track up to 1024 blocks
     const int block_size_;

   public:
     TrackedArray(int size, int block_size = 100)
       : data_(size), block_size_(block_size) {
       dirty_bits_.reset();
     }

     T& operator[](int index) {
       mark_dirty(index);
       return data_[index];
     }

     const T& operator[](int index) const {
       return data_[index];
     }

     void mark_dirty(int index) {
       int block = index / block_size_;
       if (block < dirty_bits_.size()) {
         dirty_bits_[block] = true;
       }
     }

     std::vector<std::pair<int, int>> get_dirty_ranges() const {
       std::vector<std::pair<int, int>> ranges;

       for (int i = 0; i < dirty_bits_.size(); i++) {
         if (dirty_bits_[i]) {
           int start = i * block_size_;
           int end = std::min(start + block_size_ - 1,
                             static_cast<int>(data_.size()) - 1);
           ranges.push_back({start, end});
         }
       }

       return ranges;
     }

     void clear_dirty() {
       dirty_bits_.reset();
     }

     T* data() { return data_.data(); }
     int size() const { return data_.size(); }
   };

Incremental Checkpoint Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class IncrementalCheckpointer {
     const int GROUP_ID = 0;
     const int MEMBER_ID = 0;
     TrackedArray<double> tracked_data_;
     bool first_checkpoint_ = true;

   public:
     IncrementalCheckpointer(int size, int block_size = 100)
       : tracked_data_(size, block_size) {}

     void initialize() {
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, MEMBER_ID,
                          tracked_data_.data(),
                          tracked_data_.size(),
                          MPI_DOUBLE);
     }

     void checkpoint() {
       if (first_checkpoint_) {
         // First checkpoint: store everything
         data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
         data::commit_barrier(GROUP_ID);
         first_checkpoint_ = false;
         return;
       }

       // Incremental: store only dirty blocks
       auto dirty_ranges = tracked_data_.get_dirty_ranges();

       if (dirty_ranges.empty()) {
         // Nothing changed, skip checkpoint
         return;
       }

       printf("Incremental checkpoint: %zu ranges\n", dirty_ranges.size());

       for (const auto& range : dirty_ranges) {
         Fenix_Data_subset subset;
         Fenix_Data_subset_create(range.first, range.second, &subset);
         data::member_store(GROUP_ID, MEMBER_ID, subset);
       }

       data::commit_barrier(GROUP_ID);
       tracked_data_.clear_dirty();
     }

     TrackedArray<double>& data() { return tracked_data_; }
   };

   int main(int argc, char** argv) {
     namespace data = fenix::data;
     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     IncrementalCheckpointer ckpt(10000, 100);
     ckpt.initialize();

     for (int iter = 0; iter < 1000; iter++) {
       // Update only a small portion of the array
       for (int i = 0; i < 10; i++) {
         int index = (iter * 10 + i) % 10000;
         ckpt.data()[index] = compute_new_value(index);
       }

       if (iter % 10 == 0) {
         ckpt.checkpoint();  // Incremental checkpoint
       }
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Copy-on-Write Tracking
~~~~~~~~~~~~~~~~~~~~~~~

Alternative approach using copy-on-write semantics:

.. code-block:: cpp

   template<typename T>
   class CopyOnWriteArray {
     std::vector<T> current_;
     std::vector<T> checkpoint_;
     std::vector<bool> modified_;

   public:
     CopyOnWriteArray(int size) : current_(size), checkpoint_(size),
                                   modified_(size, false) {}

     T& operator[](int index) {
       modified_[index] = true;
       return current_[index];
     }

     std::vector<int> get_modified_indices() const {
       std::vector<int> indices;
       for (int i = 0; i < modified_.size(); i++) {
         if (modified_[i]) {
           indices.push_back(i);
         }
       }
       return indices;
     }

     void checkpoint() {
       // Copy only modified elements
       for (int i = 0; i < modified_.size(); i++) {
         if (modified_[i]) {
           checkpoint_[i] = current_[i];
           modified_[i] = false;
         }
       }
     }

     void restore() {
       current_ = checkpoint_;
       std::fill(modified_.begin(), modified_.end(), false);
     }

     T* data() { return current_.data(); }
   };

.. _adaptive-checkpoint-intervals:

Adaptive Checkpoint Intervals
------------------------------

Dynamic Interval Adjustment
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Adjust checkpoint frequency based on runtime conditions:

.. code-block:: cpp

   #include <chrono>
   #include <cmath>

   class AdaptiveCheckpointer {
     using Clock = std::chrono::steady_clock;
     using Duration = std::chrono::duration<double>;

     // Configuration
     int min_interval_ = 10;
     int max_interval_ = 1000;
     double target_overhead_ = 0.05;  // 5% overhead target

     // Runtime statistics
     int current_interval_;
     int checkpoint_count_ = 0;
     Duration total_checkpoint_time_{0};
     Duration total_compute_time_{0};
     Clock::time_point last_checkpoint_time_;

   public:
     AdaptiveCheckpointer() : current_interval_(min_interval_) {
       last_checkpoint_time_ = Clock::now();
     }

     void record_checkpoint_time(Duration time) {
       total_checkpoint_time_ += time;
       checkpoint_count_++;

       // Adjust interval based on observed overhead
       double actual_overhead = compute_overhead();

       if (actual_overhead > target_overhead_ * 1.2) {
         // Too much overhead, checkpoint less frequently
         current_interval_ = std::min(
           static_cast<int>(current_interval_ * 1.5),
           max_interval_
         );
         printf("Reducing checkpoint frequency: interval = %d\n",
                current_interval_);

       } else if (actual_overhead < target_overhead_ * 0.8) {
         // Under target, can checkpoint more frequently
         current_interval_ = std::max(
           static_cast<int>(current_interval_ * 0.8),
           min_interval_
         );
         printf("Increasing checkpoint frequency: interval = %d\n",
                current_interval_);
       }
     }

     void record_compute_time(Duration time) {
       total_compute_time_ += time;
     }

     bool should_checkpoint(int iteration) const {
       return iteration % current_interval_ == 0;
     }

     int get_interval() const { return current_interval_; }

     double compute_overhead() const {
       if (total_compute_time_.count() == 0) return 0.0;
       return total_checkpoint_time_.count() /
              (total_checkpoint_time_.count() + total_compute_time_.count());
     }

     void print_statistics() const {
       printf("Checkpointing statistics:\n");
       printf("  Total checkpoints: %d\n", checkpoint_count_);
       printf("  Average interval: %d iterations\n", current_interval_);
       printf("  Overhead: %.2f%%\n", compute_overhead() * 100);
       printf("  Avg checkpoint time: %.2f ms\n",
              checkpoint_count_ > 0 ?
                std::chrono::duration_cast<std::chrono::milliseconds>(
                  total_checkpoint_time_ / checkpoint_count_
                ).count() : 0.0);
     }
   };

   int main(int argc, char** argv) {
     // ...
     AdaptiveCheckpointer adaptive;

     for (int iter = 0; iter < MAX_ITER; iter++) {
       auto compute_start = Clock::now();
       perform_computation();
       adaptive.record_compute_time(Clock::now() - compute_start);

       if (adaptive.should_checkpoint(iter)) {
         auto ckpt_start = Clock::now();
         perform_checkpoint();
         adaptive.record_checkpoint_time(Clock::now() - ckpt_start);
       }
     }

     adaptive.print_statistics();
   }

Failure-Aware Adaptation
~~~~~~~~~~~~~~~~~~~~~~~~~

Adjust based on observed failure patterns:

.. code-block:: cpp

   class FailureAwareCheckpointer {
     int base_interval_ = 100;
     int current_interval_;
     int recent_failure_count_ = 0;
     Clock::time_point last_failure_time_;
     const int HISTORY_WINDOW = 10;  // Last 10 failures

   public:
     FailureAwareCheckpointer() : current_interval_(base_interval_) {
       last_failure_time_ = Clock::now();
     }

     void on_failure() {
       recent_failure_count_++;
       auto now = Clock::now();
       auto time_since_last = now - last_failure_time_;
       last_failure_time_ = now;

       // If failures are frequent, checkpoint more often
       if (time_since_last < std::chrono::minutes(5)) {
         current_interval_ = std::max(base_interval_ / 2, 10);
         printf("Frequent failures detected, increasing checkpoint frequency\n");
       }
     }

     void on_stable_period() {
       auto time_since_failure = Clock::now() - last_failure_time_;

       // If stable for a while, reduce checkpoint frequency
       if (time_since_failure > std::chrono::minutes(30)) {
         current_interval_ = base_interval_;
         recent_failure_count_ = 0;
         printf("Stable period detected, restoring normal frequency\n");
       }
     }

     int get_interval() const {
       return current_interval_;
     }
   };

Cost-Based Optimization
~~~~~~~~~~~~~~~~~~~~~~~

Minimize total execution time (compute + checkpoint + recovery):

.. code-block:: cpp

   class CostOptimizedCheckpointer {
     double T_compute_;       // Time per iteration
     double T_checkpoint_;    // Time to checkpoint
     double MTBF_;            // Mean time between failures
     double T_recovery_;      // Time to recover

   public:
     CostOptimizedCheckpointer(double t_compute, double t_checkpoint,
                              double mtbf, double t_recovery)
       : T_compute_(t_compute), T_checkpoint_(t_checkpoint),
         MTBF_(mtbf), T_recovery_(t_recovery) {}

     int compute_optimal_interval() const {
       // Young/Daly formula for optimal checkpoint interval
       // Minimizes: T_total = T_compute + T_checkpoint + T_recovery
       //
       // Optimal interval ≈ sqrt(2 * MTBF * T_compute / T_checkpoint)

       double optimal = std::sqrt(
         2.0 * MTBF_ * T_compute_ / T_checkpoint_
       );

       return std::max(1, static_cast<int>(optimal));
     }

     void update_measurements(double t_compute, double t_checkpoint) {
       // Exponential moving average
       const double alpha = 0.1;
       T_compute_ = alpha * t_compute + (1 - alpha) * T_compute_;
       T_checkpoint_ = alpha * t_checkpoint + (1 - alpha) * T_checkpoint_;
     }

     double estimate_total_time(int interval, int total_iterations) const {
       int num_checkpoints = total_iterations / interval;
       double p_failure = total_iterations * T_compute_ / MTBF_;

       return total_iterations * T_compute_ +
              num_checkpoints * T_checkpoint_ +
              p_failure * (T_recovery_ + interval * T_compute_ / 2);
     }
   };

Asynchronous Checkpointing
---------------------------

Non-Blocking Checkpoint Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Overlap checkpoint with computation:

.. code-block:: cpp

   class AsyncCheckpointer {
     const int GROUP_ID = 0;
     bool checkpoint_in_progress_ = false;
     std::vector<double> checkpoint_buffer_;
     std::vector<double> compute_buffer_;

   public:
     AsyncCheckpointer(int size) : checkpoint_buffer_(size),
                                    compute_buffer_(size) {}

     void start_async_checkpoint() {
       if (checkpoint_in_progress_) {
         // Wait for previous checkpoint to complete
         wait_checkpoint();
       }

       // Copy data to checkpoint buffer
       checkpoint_buffer_ = compute_buffer_;

       // Start non-blocking checkpoint
       data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
       int time_stamp;
       Fenix_Data_commit(GROUP_ID, &time_stamp);

       checkpoint_in_progress_ = true;

       printf("Async checkpoint started, continuing computation\n");
     }

     void wait_checkpoint() {
       if (checkpoint_in_progress_) {
         Fenix_Data_wait(GROUP_ID);
         checkpoint_in_progress_ = false;
         printf("Async checkpoint completed\n");
       }
     }

     std::vector<double>& data() { return compute_buffer_; }

     ~AsyncCheckpointer() {
       wait_checkpoint();  // Ensure checkpoint completes
     }
   };

   int main(int argc, char** argv) {
     // ...
     AsyncCheckpointer async_ckpt(N);

     for (int iter = 0; iter < MAX_ITER; iter++) {
       // Compute using compute_buffer
       for (int i = 0; i < N; i++) {
         async_ckpt.data()[i] = compute_new_value(i);
       }

       if (iter % CHECKPOINT_FREQ == 0) {
         // Start checkpoint (non-blocking)
         async_ckpt.start_async_checkpoint();
         // Computation continues while checkpoint happens in background
       }

       // Do more computation
       // ...
     }

     async_ckpt.wait_checkpoint();  // Wait before finalize
   }

**Note:** Fenix's ``Fenix_Data_commit`` is synchronous with commit_barrier, but you can separate store operations from the commit for a degree of overlap.

Compression Techniques
----------------------

Simple Compression
~~~~~~~~~~~~~~~~~~

Compress data before checkpointing:

.. code-block:: cpp

   #include <zlib.h>

   class CompressedCheckpointer {
     const int GROUP_ID = 0;
     const int MEMBER_ID = 0;
     std::vector<double> data_;
     std::vector<unsigned char> compressed_buffer_;

   public:
     CompressedCheckpointer(int size) : data_(size) {
       // Compressed buffer (worst case: same size as uncompressed)
       compressed_buffer_.resize(compressBound(size * sizeof(double)));
     }

     void checkpoint_compressed() {
       // Compress data
       uLongf compressed_size = compressed_buffer_.size();
       int ret = compress(
         compressed_buffer_.data(),
         &compressed_size,
         reinterpret_cast<unsigned char*>(data_.data()),
         data_.size() * sizeof(double)
       );

       if (ret != Z_OK) {
         fprintf(stderr, "Compression failed\n");
         return;
       }

       double ratio = static_cast<double>(compressed_size) /
                     (data_.size() * sizeof(double));
       printf("Compression ratio: %.2f\n", ratio);

       // Checkpoint compressed data
       data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     }

     void restore_compressed() {
       // Restore compressed data
       data::member_restore(GROUP_ID, MEMBER_ID);

       // Decompress
       uLongf uncompressed_size = data_.size() * sizeof(double);
       int ret = uncompress(
         reinterpret_cast<unsigned char*>(data_.data()),
         &uncompressed_size,
         compressed_buffer_.data(),
         compressed_buffer_.size()
       );

       if (ret != Z_OK) {
         fprintf(stderr, "Decompression failed\n");
       }
     }

     std::vector<double>& data() { return data_; }
   };

Delta Encoding
~~~~~~~~~~~~~~

Store only differences from previous checkpoint:

.. code-block:: cpp

   class DeltaCheckpointer {
     std::vector<double> current_data_;
     std::vector<double> previous_checkpoint_;
     std::vector<double> delta_;

   public:
     DeltaCheckpointer(int size) : current_data_(size),
                                    previous_checkpoint_(size),
                                    delta_(size) {}

     void checkpoint_delta() {
       // Compute delta
       int non_zero_count = 0;
       for (int i = 0; i < current_data_.size(); i++) {
         delta_[i] = current_data_[i] - previous_checkpoint_[i];
         if (delta_[i] != 0.0) {
           non_zero_count++;
         }
       }

       double sparsity = static_cast<double>(non_zero_count) /
                        current_data_.size();

       printf("Delta sparsity: %.2f%%\n", sparsity * 100);

       if (sparsity < 0.1) {
         // Very sparse, use sparse representation
         checkpoint_sparse_delta(non_zero_count);
       } else {
         // Dense, checkpoint full delta
         checkpoint_full_delta();
       }

       // Update previous checkpoint
       previous_checkpoint_ = current_data_;
     }

   private:
     void checkpoint_sparse_delta(int count) {
       // Store only non-zero delta values
       std::vector<int> indices;
       std::vector<double> values;

       for (int i = 0; i < delta_.size(); i++) {
         if (delta_[i] != 0.0) {
           indices.push_back(i);
           values.push_back(delta_[i]);
         }
       }

       // Checkpoint sparse representation
       // ... use Fenix to checkpoint indices and values
       printf("Sparse delta checkpoint: %d/%zu elements\n",
              count, delta_.size());
     }

     void checkpoint_full_delta() {
       // Checkpoint full delta array
       data::member_store(GROUP_ID, DELTA_MEMBER, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     }

   public:
     std::vector<double>& data() { return current_data_; }
   };

Quantization
~~~~~~~~~~~~

Reduce precision for reduced storage:

.. code-block:: cpp

   class QuantizedCheckpointer {
     std::vector<double> data_;
     std::vector<int16_t> quantized_;
     double scale_ = 1.0;
     double offset_ = 0.0;

   public:
     QuantizedCheckpointer(int size) : data_(size), quantized_(size) {}

     void checkpoint_quantized() {
       // Find min/max for quantization
       double min_val = *std::min_element(data_.begin(), data_.end());
       double max_val = *std::max_element(data_.begin(), data_.end());

       scale_ = (max_val - min_val) / 65535.0;  // 16-bit range
       offset_ = min_val;

       // Quantize
       for (int i = 0; i < data_.size(); i++) {
         quantized_[i] = static_cast<int16_t>(
           (data_[i] - offset_) / scale_
         );
       }

       printf("Quantization: scale=%.6f, offset=%.6f\n", scale_, offset_);
       printf("Size reduction: %.1fx\n",
              sizeof(double) / static_cast<double>(sizeof(int16_t)));

       // Checkpoint quantized data + scale/offset
       // Size is 4x smaller (double->int16)
       data::member_store(GROUP_ID, QUANTIZED_MEMBER, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
     }

     void restore_quantized() {
       // Restore quantized data
       data::member_restore(GROUP_ID, QUANTIZED_MEMBER);

       // Dequantize
       for (int i = 0; i < quantized_.size(); i++) {
         data_[i] = quantized_[i] * scale_ + offset_;
       }
     }

     std::vector<double>& data() { return data_; }
   };

Complete Optimized Example
---------------------------

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>
   #include <bitset>
   #include <chrono>

   constexpr int N = 100000;
   constexpr int BLOCK_SIZE = 1000;
   constexpr int NUM_BLOCKS = N / BLOCK_SIZE;
   constexpr int MAX_ITER = 10000;

   class OptimizedCheckpointer {
     using Clock = std::chrono::steady_clock;

     // Data
     std::vector<double> data_;
     std::bitset<NUM_BLOCKS> dirty_blocks_;

     // Adaptive checkpointing
     int current_interval_ = 100;
     int min_interval_ = 10;
     int max_interval_ = 500;

     // Statistics
     Clock::duration total_ckpt_time_{0};
     Clock::duration total_compute_time_{0};
     int checkpoint_count_ = 0;

     const int GROUP_ID = 0;
     const int MEMBER_ID = 0;

   public:
     OptimizedCheckpointer(int size) : data_(size) {
       dirty_blocks_.reset();
     }

     void initialize() {
       data::group_create(GROUP_ID);
       data::member_create(GROUP_ID, MEMBER_ID,
                          data_.data(), data_.size(), MPI_DOUBLE);

       // Initial full checkpoint
       auto start = Clock::now();
       data::member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
       data::commit_barrier(GROUP_ID);
       total_ckpt_time_ += Clock::now() - start;
       checkpoint_count_++;
     }

     void update(int index, double value) {
       data_[index] = value;
       dirty_blocks_[index / BLOCK_SIZE] = true;
     }

     bool should_checkpoint(int iteration) const {
       return iteration % current_interval_ == 0;
     }

     void checkpoint_incremental() {
       auto start = Clock::now();

       if (dirty_blocks_.none()) {
         return;  // Nothing to checkpoint
       }

       // Checkpoint only dirty blocks
       int dirty_count = 0;
       for (int i = 0; i < NUM_BLOCKS; i++) {
         if (dirty_blocks_[i]) {
           int block_start = i * BLOCK_SIZE;
           int block_end = block_start + BLOCK_SIZE - 1;

           Fenix_Data_subset subset;
           Fenix_Data_subset_create(block_start, block_end, &subset);
           data::member_store(GROUP_ID, MEMBER_ID, subset);
           dirty_count++;
         }
       }

       data::commit_barrier(GROUP_ID);
       dirty_blocks_.reset();

       auto elapsed = Clock::now() - start;
       total_ckpt_time_ += elapsed;
       checkpoint_count_++;

       printf("Incremental checkpoint: %d/%d blocks (%.2f ms)\n",
              dirty_count, NUM_BLOCKS,
              std::chrono::duration<double, std::milli>(elapsed).count());

       adapt_interval();
     }

     void record_compute_time(Clock::duration time) {
       total_compute_time_ += time;
     }

     void adapt_interval() {
       // Adjust interval to maintain ~5% overhead
       double overhead = compute_overhead();
       const double target = 0.05;

       if (overhead > target * 1.2) {
         current_interval_ = std::min(
           static_cast<int>(current_interval_ * 1.3),
           max_interval_
         );
       } else if (overhead < target * 0.8) {
         current_interval_ = std::max(
           static_cast<int>(current_interval_ * 0.9),
           min_interval_
         );
       }
     }

     double compute_overhead() const {
       auto total = total_ckpt_time_ + total_compute_time_;
       return total.count() > 0 ?
         static_cast<double>(total_ckpt_time_.count()) / total.count() : 0.0;
     }

     void print_statistics() {
       printf("\n=== Checkpoint Statistics ===\n");
       printf("Total checkpoints: %d\n", checkpoint_count_);
       printf("Current interval: %d iterations\n", current_interval_);
       printf("Checkpoint overhead: %.2f%%\n", compute_overhead() * 100);
       printf("Avg checkpoint time: %.2f ms\n",
              checkpoint_count_ > 0 ?
              std::chrono::duration<double, std::milli>(
                total_ckpt_time_ / checkpoint_count_
              ).count() : 0.0);
     }

     std::vector<double>& data() { return data_; }
   };

   int main(int argc, char** argv) {
     using Clock = std::chrono::steady_clock;

     MPI_Init(&argc, &argv);

     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 2});

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     OptimizedCheckpointer ckpt(N);
     ckpt.initialize();

     for (int iter = 0; iter < MAX_ITER; iter++) {
       auto compute_start = Clock::now();

       // Sparse updates (only 1% of array changes per iteration)
       for (int i = 0; i < N / 100; i++) {
         int index = (iter * N / 100 + i) % N;
         ckpt.update(index, rank + iter * 0.001);
       }

       ckpt.record_compute_time(Clock::now() - compute_start);

       if (ckpt.should_checkpoint(iter)) {
         ckpt.checkpoint_incremental();
       }
     }

     if (rank == 0) {
       ckpt.print_statistics();
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Troubleshooting
---------------

**Problem: Incremental checkpoints not faster**

- Verify dirty tracking is working correctly
- Check that updates are truly sparse
- Ensure block size is appropriate
- Profile to find actual bottleneck

**Problem: Adaptive interval oscillates**

- Increase adaptation smoothing (slower response)
- Add hysteresis (require larger changes to trigger adaptation)
- Check if measurements are accurate

**Problem: Compression too slow**

- Use faster compression algorithm (LZ4 instead of zlib)
- Reduce compression level
- Only compress large data
- Use compression selectively

**Problem: Delta encoding produces larger checkpoints**

- Check if data changes are truly small
- Consider threshold for full vs delta checkpoint
- Verify delta computation is correct

See Also
--------

- :doc:`checkpoint-data` - Basic checkpointing guide
- :doc:`partial-checkpoints` - Using data subsets
- :doc:`performance-tuning` - Overall performance optimization
- :doc:`/guides/data-recovery` - Data recovery concepts
- :doc:`/api/data-recovery` - Data recovery API reference
