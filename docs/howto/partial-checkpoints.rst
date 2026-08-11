Partial Checkpoints with Data Subsets
=====================================

Learn how to checkpoint only portions of your data using Fenix data subsets, reducing checkpoint time and storage requirements while maintaining fault tolerance.

.. contents:: Quick Jump
   :local:
   :depth: 2

What are Data Subsets?
----------------------

A data subset specifies which elements of a data member to checkpoint or restore. Instead of saving an entire array, you can save just the parts that changed or that are critical for recovery.

Why Use Subsets?
~~~~~~~~~~~~~~~~

**Performance**: Checkpointing less data is faster.

Example: If only 10% of your array changes each iteration, checkpoint only that 10%. This can make checkpoints 10x faster.

**Storage**: Smaller checkpoints use less memory for redundancy.

With RAID-style policies, redundant data is stored on other ranks. Smaller checkpoints mean less memory overhead.

**Flexibility**: Different checkpoint strategies for different data.

You might checkpoint critical metadata fully but large working arrays partially.

When to Use Subsets
~~~~~~~~~~~~~~~~~~~

Use subsets when:

- Only a portion of data changes between checkpoints
- Some data is more critical than others
- Memory for redundancy is limited
- Checkpoint time is a bottleneck

Don't use subsets when:

- All data changes every iteration anyway
- Data is already small (overhead not worth it)
- Simplicity is more important than performance

Prerequisites
-------------

- Understanding of basic checkpointing (:doc:`checkpoint-data`)
- Knowledge of your application's data access patterns
- Fenix data groups and members already set up

Creating Data Subsets
----------------------

Two Ways to Create Subsets
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix provides two functions for creating subsets:

1. **Fenix_Data_subset_create**: Uniform subsets across all ranks
2. **Fenix_Data_subset_createv**: Varying subsets per rank

We'll cover both.

Regular Stride Subsets
----------------------

Use ``Fenix_Data_subset_create`` for patterns with regular spacing.

Basic Pattern
~~~~~~~~~~~~~

Checkpoint elements at regular intervals:

.. code-block:: cpp

   #include <fenix.h>

   Fenix_Data_subset subset;

   // Checkpoint elements 0-9, 20-29, 40-49, 60-69, ...
   // (10 blocks of 10 elements, starting at 0, stride 20)
   Fenix_Data_subset_create(
     /*num_blocks=*/10,
     /*start_offset=*/0,
     /*end_offset=*/9,
     /*stride=*/20,
     &subset
   );

   // Use subset
   Fenix_Data_member_store(group, member, subset);

   // Clean up when done
   Fenix_Data_subset_delete(&subset);

How It Works
~~~~~~~~~~~~

The function creates blocks based on:

.. code-block:: text

   Block 0: [start_offset, end_offset]
   Block 1: [start_offset + stride, end_offset + stride]
   Block 2: [start_offset + 2*stride, end_offset + 2*stride]
   ...
   Block N-1: [start_offset + (N-1)*stride, end_offset + (N-1)*stride]

Parameters
~~~~~~~~~~

.. code-block:: cpp

   int Fenix_Data_subset_create(
     int num_blocks,      // Number of blocks
     int start_offset,    // First element of first block
     int end_offset,      // Last element of first block
     int stride,          // Distance between blocks
     Fenix_Data_subset* subset
   );

- **num_blocks**: How many blocks to create
- **start_offset**: Index of first element (0-based)
- **end_offset**: Index of last element in first block (inclusive)
- **stride**: How far to jump for next block
- **subset**: Output parameter for created subset

Common Patterns
~~~~~~~~~~~~~~~

**Every Nth Element**

.. code-block:: cpp

   // Checkpoint every 10th element: 0, 10, 20, 30, ...
   Fenix_Data_subset_create(100, 0, 0, 10, &subset);

**First N Elements**

.. code-block:: cpp

   // Checkpoint first 100 elements
   Fenix_Data_subset_create(1, 0, 99, 1, &subset);

**Boundary Elements**

.. code-block:: cpp

   // In a 1D domain with 1000 elements, checkpoint first and last 10
   Fenix_Data_subset_create(2,
     /*start=*/0, /*end=*/9, /*stride=*/980, &subset);
   // Gives [0-9] and [980-989]

**Halo Regions**

.. code-block:: cpp

   // For stencil computation: checkpoint boundary layers
   const int total_size = 1000;
   const int halo_width = 5;

   Fenix_Data_subset_create(2,
     /*start=*/0,
     /*end=*/halo_width - 1,
     /*stride=*/total_size - halo_width,
     &subset);

Complete Example with Regular Stride
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.h>
   #include <mpi.h>
   #include <stdio.h>
   #include <stdlib.h>

   constexpr int GROUP = 0;
   constexpr int MEMBER = 0;

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     int role, error;
     MPI_Comm res_comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
                &argc, &argv, /*spares=*/1, &error);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     const int array_size = 100;
     int data[array_size];

     // Create subset: checkpoint indices 0-2, 5-7, 10-12, ... (10 blocks)
     Fenix_Data_subset subset;
     Fenix_Data_subset_create(10, 0, 2, 5, &subset);

     if (role == FENIX_ROLE_INITIAL_RANK) {
       // Initialize
       for (int i = 0; i < array_size; i++) {
         data[i] = -1;  // Uninitialized marker
       }

       Fenix_Data_group_create(GROUP, res_comm, 0, 1,
                               FENIX_DATA_POLICY_IN_MEMORY_RAID,
                               NULL, &error);
       Fenix_Data_member_create(GROUP, MEMBER, data, array_size, MPI_INT);

       // Store entire array initially
       Fenix_Data_member_store(GROUP, MEMBER, FENIX_DATA_SUBSET_FULL);
       Fenix_Data_commit_barrier(GROUP, NULL);

       // Update only some elements
       for (int i = 0; i < array_size; i += 5) {
         data[i] = rank * 1000 + i;
         if (i + 1 < array_size) data[i + 1] = rank * 1000 + i + 1;
         if (i + 2 < array_size) data[i + 2] = rank * 1000 + i + 2;
       }

       // Store only the subset
       Fenix_Data_member_store(GROUP, MEMBER, subset);
       Fenix_Data_commit_barrier(GROUP, NULL);
     } else {
       // Recovery
       for (int i = 0; i < array_size; i++) {
         data[i] = -999;  // Different marker to verify restore
       }

       Fenix_Data_member_restore(GROUP, MEMBER, data, array_size,
                                 FENIX_DATA_SNAPSHOT_ALL, NULL);

       // Verify: subset elements should be restored, others should be -1
       for (int i = 0; i < array_size; i++) {
         int in_subset = 0;
         for (int block = 0; block < 10; block++) {
           if (i >= block * 5 && i <= block * 5 + 2) {
             in_subset = 1;
             break;
           }
         }

         if (in_subset && data[i] == -1) {
           printf("Rank %d: data[%d] should have been updated but is -1\n",
                  rank, i);
         } else if (!in_subset && data[i] != -1) {
           printf("Rank %d: data[%d] should be -1 but is %d\n",
                  rank, i, data[i]);
         }
       }

       printf("Rank %d: Recovery verification complete\n", rank);
     }

     Fenix_Data_subset_delete(&subset);
     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Varying Subsets with createv
-------------------------------

Use ``Fenix_Data_subset_createv`` when different ranks need different subsets.

Basic Usage
~~~~~~~~~~~

Specify start and end for each block explicitly:

.. code-block:: cpp

   Fenix_Data_subset subset;

   int num_blocks = 3;
   int starts[] = {0, 50, 200};
   int ends[] = {10, 75, 250};

   // Checkpoint [0-10], [50-75], [200-250]
   Fenix_Data_subset_createv(num_blocks, starts, ends, &subset);

   Fenix_Data_member_store(group, member, subset);

   Fenix_Data_subset_delete(&subset);

Parameters
~~~~~~~~~~

.. code-block:: cpp

   int Fenix_Data_subset_createv(
     int num_blocks,
     int* array_start_offsets,  // Start of each block
     int* array_end_offsets,    // End of each block (inclusive)
     Fenix_Data_subset* subset
   );

- **num_blocks**: Number of blocks
- **array_start_offsets**: Array of start indices
- **array_end_offsets**: Array of end indices (inclusive)
- **subset**: Output parameter

Common Patterns
~~~~~~~~~~~~~~~

**Sparse Updates**

.. code-block:: cpp

   // Checkpoint only elements that changed
   std::vector<int> starts, ends;

   for (int i = 0; i < array_size; i++) {
     if (data_changed[i]) {
       starts.push_back(i);
       ends.push_back(i);  // Single element
     }
   }

   if (!starts.empty()) {
     Fenix_Data_subset_createv(starts.size(),
                               starts.data(), ends.data(),
                               &subset);
     Fenix_Data_member_store(group, member, subset);
   }

**Non-uniform Partitions**

.. code-block:: cpp

   // Checkpoint different-sized regions
   int starts[] = {0, 100, 150, 500};
   int ends[] = {50, 120, 300, 999};  // Variable-size blocks

   Fenix_Data_subset_createv(4, starts, ends, &subset);

**Critical Data Sections**

.. code-block:: cpp

   // Checkpoint only the important parts
   int starts[] = {
     metadata_start,
     active_region_start,
     boundary_start
   };
   int ends[] = {
     metadata_end,
     active_region_end,
     boundary_end
   };

   Fenix_Data_subset_createv(3, starts, ends, &subset);

Complete Example with createv
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.h>
   #include <mpi.h>
   #include <vector>
   #include <stdio.h>

   constexpr int GROUP = 0;
   constexpr int MEMBER = 0;

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     int role, error;
     MPI_Comm res_comm;
     Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
                &argc, &argv, 1, &error);

     int rank;
     MPI_Comm_rank(res_comm, &rank);

     const int array_size = 1000;
     std::vector<double> data(array_size);
     std::vector<bool> modified(array_size, false);

     if (role == FENIX_ROLE_INITIAL_RANK) {
       // Initialize all data
       for (int i = 0; i < array_size; i++) {
         data[i] = rank * 10000.0 + i;
       }

       Fenix_Data_group_create(GROUP, res_comm, 0, 1,
                               FENIX_DATA_POLICY_IN_MEMORY_RAID,
                               NULL, &error);
       Fenix_Data_member_create(GROUP, MEMBER, data.data(),
                                array_size, MPI_DOUBLE);

       // Initial full checkpoint
       Fenix_Data_member_store(GROUP, MEMBER, FENIX_DATA_SUBSET_FULL);
       Fenix_Data_commit_barrier(GROUP, NULL);

       // Simulate work: modify only some elements
       // For example, modify elements 10-20, 100-105, and 500-600
       for (int i = 10; i <= 20; i++) {
         data[i] *= 2.0;
         modified[i] = true;
       }
       for (int i = 100; i <= 105; i++) {
         data[i] *= 2.0;
         modified[i] = true;
       }
       for (int i = 500; i <= 600; i++) {
         data[i] *= 2.0;
         modified[i] = true;
       }

       // Build subset of modified ranges
       std::vector<int> starts, ends;
       int range_start = -1;

       for (int i = 0; i < array_size; i++) {
         if (modified[i] && range_start == -1) {
           range_start = i;  // Start new range
         } else if (!modified[i] && range_start != -1) {
           // End current range
           starts.push_back(range_start);
           ends.push_back(i - 1);
           range_start = -1;
         }
       }
       // Handle range extending to end
       if (range_start != -1) {
         starts.push_back(range_start);
         ends.push_back(array_size - 1);
       }

       // Create and use subset
       Fenix_Data_subset subset;
       Fenix_Data_subset_createv(starts.size(),
                                 starts.data(), ends.data(),
                                 &subset);

       printf("Rank %d: Checkpointing %zu ranges\n", rank, starts.size());
       for (size_t i = 0; i < starts.size(); i++) {
         printf("  Range %zu: [%d, %d] (%d elements)\n",
                i, starts[i], ends[i], ends[i] - starts[i] + 1);
       }

       Fenix_Data_member_store(GROUP, MEMBER, subset);
       Fenix_Data_commit_barrier(GROUP, NULL);

       Fenix_Data_subset_delete(&subset);
     } else {
       // Recovery
       Fenix_Data_member_restore(GROUP, MEMBER, data.data(),
                                 array_size, FENIX_DATA_SNAPSHOT_ALL,
                                 NULL);

       // Verify modified elements were restored correctly
       bool valid = true;
       for (int i = 10; i <= 20; i++) {
         double expected = (rank * 10000.0 + i) * 2.0;
         if (data[i] != expected) {
           printf("Rank %d: data[%d] = %f, expected %f\n",
                  rank, i, data[i], expected);
           valid = false;
         }
       }

       printf("Rank %d: Recovery %s\n", rank, valid ? "PASSED" : "FAILED");
     }

     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

C++ API with DataSubset
------------------------

The C++ API provides a more convenient interface:

Basic C++ Usage
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.hpp>
   #include <fenix_data_subset.hpp>

   using namespace fenix::data;

   // Create subset with initializer lists
   DataSubset subset{{0, 10}, {50, 75}, {200, 250}};

   // Use directly
   member_store(group, member, subset);

This automatically handles memory management (no need for delete).

Dynamic Subsets
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.hpp>
   #include <vector>

   std::vector<std::pair<int, int>> ranges;

   // Build ranges dynamically
   for (int i = 0; i < data.size(); i++) {
     if (needs_checkpoint(data[i])) {
       ranges.push_back({i, i});
     }
   }

   // Create subset from vector
   fenix::DataSubset subset(ranges);
   fenix::data::member_store(group, member, subset);

Combining Ranges
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Merge adjacent ranges for efficiency
   std::vector<std::pair<int, int>> optimize_ranges(
     const std::vector<std::pair<int, int>>& ranges
   ) {
     if (ranges.empty()) return {};

     std::vector<std::pair<int, int>> result;
     result.push_back(ranges[0]);

     for (size_t i = 1; i < ranges.size(); i++) {
       auto& last = result.back();
       if (ranges[i].first <= last.second + 1) {
         // Adjacent or overlapping - merge
         last.second = std::max(last.second, ranges[i].second);
       } else {
         // Not adjacent - add new range
         result.push_back(ranges[i]);
       }
     }

     return result;
   }

Practical Patterns
------------------

Pattern 1: Track Dirty Bits
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Checkpoint only modified data:

.. code-block:: cpp

   class CheckpointableArray {
     std::vector<double> data_;
     std::vector<bool> dirty_;

   public:
     CheckpointableArray(size_t size) : data_(size), dirty_(size, false) {}

     void set(int i, double value) {
       data_[i] = value;
       dirty_[i] = true;
     }

     double get(int i) const { return data_[i]; }

     fenix::DataSubset get_dirty_subset() {
       std::vector<std::pair<int, int>> ranges;
       int start = -1;

       for (size_t i = 0; i < dirty_.size(); i++) {
         if (dirty_[i] && start == -1) {
           start = i;
         } else if (!dirty_[i] && start != -1) {
           ranges.push_back({start, static_cast<int>(i - 1)});
           start = -1;
         }
       }
       if (start != -1) {
         ranges.push_back({start, static_cast<int>(dirty_.size() - 1)});
       }

       return fenix::DataSubset(ranges);
     }

     void clear_dirty() {
       std::fill(dirty_.begin(), dirty_.end(), false);
     }

     double* data() { return data_.data(); }
     size_t size() const { return data_.size(); }
   };

   // Usage
   CheckpointableArray arr(10000);

   // Work
   arr.set(100, 42.0);
   arr.set(101, 43.0);
   arr.set(500, 99.0);

   // Checkpoint only changed data
   auto subset = arr.get_dirty_subset();
   fenix::data::member_store(group, member, subset);
   fenix::data::commit_barrier(group);

   arr.clear_dirty();

Pattern 2: Staged Checkpointing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Checkpoint different data at different frequencies:

.. code-block:: cpp

   struct AppState {
     std::vector<double> critical_data;    // Checkpoint every iteration
     std::vector<double> intermediate_data; // Checkpoint every 10 iterations
     std::vector<double> cache_data;        // Checkpoint every 100 iterations
   };

   void checkpoint_staged(AppState& state, int iteration) {
     namespace data = fenix::data;

     // Always checkpoint critical data
     data::member_store(GROUP, CRITICAL_MEMBER);

     // Conditionally checkpoint others
     if (iteration % 10 == 0) {
       data::member_store(GROUP, INTERMEDIATE_MEMBER);
     }

     if (iteration % 100 == 0) {
       data::member_store(GROUP, CACHE_MEMBER);
     }

     data::commit_barrier(GROUP);
   }

Pattern 3: Adaptive Subsets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Adjust subset size based on how much data changed:

.. code-block:: cpp

   fenix::DataSubset compute_adaptive_subset(
     const std::vector<double>& data,
     const std::vector<double>& last_checkpoint,
     double threshold = 0.01
   ) {
     std::vector<std::pair<int, int>> ranges;
     int start = -1;

     for (size_t i = 0; i < data.size(); i++) {
       double change = std::abs(data[i] - last_checkpoint[i]);
       bool changed = (change > threshold * std::abs(last_checkpoint[i]));

       if (changed && start == -1) {
         start = i;
       } else if (!changed && start != -1) {
         ranges.push_back({start, static_cast<int>(i - 1)});
         start = -1;
       }
     }
     if (start != -1) {
       ranges.push_back({start, static_cast<int>(data.size() - 1)});
     }

     // If too much changed, checkpoint everything
     int changed_elements = 0;
     for (const auto& [s, e] : ranges) {
       changed_elements += (e - s + 1);
     }

     if (changed_elements > data.size() * 0.5) {
       return fenix::data::SUBSET_FULL;
     }

     return fenix::DataSubset(ranges);
   }

Performance Benefits
--------------------

Measuring Improvement
~~~~~~~~~~~~~~~~~~~~~

Compare full vs. partial checkpointing:

.. code-block:: cpp

   #include <chrono>

   auto start = std::chrono::high_resolution_clock::now();

   // Full checkpoint
   fenix::data::member_store(group, member, fenix::data::SUBSET_FULL);
   fenix::data::commit_barrier(group);

   auto end = std::chrono::high_resolution_clock::now();
   auto full_time = std::chrono::duration_cast<std::chrono::milliseconds>(
     end - start
   ).count();

   // Partial checkpoint
   start = std::chrono::high_resolution_clock::now();

   fenix::DataSubset subset = compute_dirty_subset();
   fenix::data::member_store(group, member, subset);
   fenix::data::commit_barrier(group);

   end = std::chrono::high_resolution_clock::now();
   auto partial_time = std::chrono::duration_cast<std::chrono::milliseconds>(
     end - start
   ).count();

   printf("Full: %ldms, Partial: %ldms, Speedup: %.2fx\n",
          full_time, partial_time,
          static_cast<double>(full_time) / partial_time);

Example Results
~~~~~~~~~~~~~~~

Real-world case: Stencil computation with 1M element array:

.. code-block:: text

   Iteration 0 (full checkpoint):     450ms
   Iteration 1 (10% changed):          52ms (8.7x faster)
   Iteration 2 (15% changed):          78ms (5.8x faster)
   Iteration 10 (full checkpoint):    450ms
   Iteration 11 (5% changed):          28ms (16x faster)

Memory savings with RAID policy:

.. code-block:: text

   Full checkpoint:    1M elements * 8 bytes * 2 copies = 16MB per rank
   Partial (10%):     100K elements * 8 bytes * 2 copies = 1.6MB per rank
   Savings:           90% less memory used

Limitations and Caveats
------------------------

Understanding FENIX_DATA_SNAPSHOT_ALL
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When restoring with ``FENIX_DATA_SNAPSHOT_ALL``, Fenix loads each element from the most recent snapshot containing it:

.. code-block:: cpp

   // Iteration 0: Store full array
   member_store(group, member, SUBSET_FULL);
   commit(group);  // Snapshot 0: has all elements

   // Iteration 1: Store subset [0-99]
   DataSubset subset1{{0, 99}};
   member_store(group, member, subset1);
   commit(group);  // Snapshot 1: has elements 0-99 only

   // On restore with SNAPSHOT_ALL:
   // Elements 0-99 come from snapshot 1 (most recent)
   // Elements 100+ come from snapshot 0
   member_restore(group, member, nullptr, 0,
                  FENIX_DATA_SNAPSHOT_ALL, &found);

This is usually what you want, but be aware of the semantics.

Resizable Members
~~~~~~~~~~~~~~~~~

For resizable members (``count = FENIX_RESIZEABLE``), you cannot use ``SUBSET_FULL``:

.. code-block:: cpp

   // Create resizable member
   member_create(group, member, data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // Must use explicit subset
   DataSubset subset{{0, current_size - 1}};
   member_store(group, member, subset);  // OK

   // This would error:
   // member_store(group, member, SUBSET_FULL);  // ERROR!

Overlapping Blocks
~~~~~~~~~~~~~~~~~~

Blocks can overlap:

.. code-block:: cpp

   int starts[] = {0, 5, 10};
   int ends[] = {10, 15, 20};  // [0-10] and [5-15] overlap

   Fenix_Data_subset_createv(3, starts, ends, &subset);

This is allowed and sometimes useful, but be aware of the redundancy.

Troubleshooting
---------------

Empty Subset Created
~~~~~~~~~~~~~~~~~~~~

Check that ranges are valid:

.. code-block:: cpp

   // Debug: Print subset details
   if (ranges.empty()) {
     printf("Warning: Empty subset - no data to checkpoint!\n");
   } else {
     printf("Subset has %zu ranges:\n", ranges.size());
     for (size_t i = 0; i < ranges.size(); i++) {
       printf("  [%d, %d]\n", ranges[i].first, ranges[i].second);
     }
   }

Restore Returns PARTIAL_RESTORE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This means some elements couldn't be restored:

.. code-block:: cpp

   fenix::DataSubset found;
   int ret = fenix::data::member_restore(group, member, nullptr, 0,
                                         FENIX_DATA_SNAPSHOT_ALL, &found);

   if (ret == FENIX_WARNING_PARTIAL_RESTORE) {
     printf("Warning: Partial restore\n");
     // Check what was actually found
     // Might need to initialize missing elements to defaults
   }

Invalid Subset Error
~~~~~~~~~~~~~~~~~~~~

Check that:

1. End offset >= start offset
2. Indices are within bounds
3. Subset was created successfully

.. code-block:: cpp

   int ret = Fenix_Data_subset_createv(num_blocks, starts, ends, &subset);
   if (ret != FENIX_SUCCESS) {
     fprintf(stderr, "Subset creation failed: %d\n", ret);
     // Check your start/end arrays
   }

Next Steps
----------

- :doc:`optimize-checkpoints` - Further optimization strategies
- :doc:`checkpoint-data` - Basic checkpointing guide
- :doc:`/api/data-recovery` - Complete data recovery API
- :doc:`/guides/data-recovery` - Understanding data recovery concepts

See Also
--------

- Example: ``examples/05_subset_create/subset_create.c``
- Example: ``examples/06_subset_createv/subset_createv.c``
- :doc:`/guides/imr-policy` - RAID-style in-memory redundancy
