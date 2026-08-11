API Quick Reference
===================

One-page reference for the most commonly used Fenix functions and constants. For complete API documentation, see :doc:`api/index`.

.. contents:: Sections
   :local:
   :depth: 2

Initialization & Finalization
------------------------------

**C++ API (Recommended):**

.. code-block:: cpp

   #include <fenix.hpp>

   MPI_Comm res_comm;
   fenix::init({
     .out_comm = &res_comm,  // Output: resilient communicator
     .spares = 3             // Number of spare ranks
   });

   fenix::role();      // Get current rank role
   fenix::error();     // Get error code from last recovery
   fenix::nspare();    // Get remaining spare ranks

   Fenix_Finalize();   // Cleanup (all ranks must call)

**C API:**

.. code-block:: c

   #include <fenix.h>

   int role, error;
   MPI_Comm res_comm;
   Fenix_Init(&role, MPI_COMM_WORLD, &res_comm,
              &argc, &argv, 3, &error);

   Fenix_Rank_role my_role = Fenix_get_role();
   int err = Fenix_get_error();
   int nspares = Fenix_get_nspare();

   Fenix_Finalize();

Configuration
-------------

**Recovery Mode:**

.. code-block:: cpp

   // Set before Fenix_Init or during execution
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);  // C++: Use exceptions
   fenix::set_option(fenix::RESUME_MODE, fenix::RETURN); // Inline: Return error
   fenix::set_option(fenix::RESUME_MODE, fenix::JUMP);   // Default: longjmp

**Other Settings:**

.. code-block:: cpp

   fenix::set_option(fenix::RECOVERY_MODE, fenix::REPAIR);  // Enable recovery
   fenix::set_option(fenix::SPARE_WAIT_MODE, fenix::YIELD); // Spare wait mode

Process Recovery
----------------

**Callbacks (C++):**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     printf("Recovery callback invoked\n");
     // Restore application state here
     fenix::data::member_restore(GROUP_ID, MEMBER_ID);
   });

**Callbacks (C):**

.. code-block:: c

   void my_callback(MPI_Comm repaired, int err, void* ctx) {
     printf("Recovery callback invoked\n");
   }

   Fenix_Callback_register(my_callback, context_data);

**Failure Detection:**

.. code-block:: cpp

   // Manually check for failures (useful in long compute phases)
   fenix::detect_failures(true);  // Recover if detected

   // Get list of failed ranks from last recovery
   std::vector<int> failed = fenix::fail_list();

Data Recovery - Setup
---------------------

**Create Data Group (C++):**

.. code-block:: cpp

   using namespace fenix::data;

   const int GROUP_ID = 1;

   group_create(GROUP_ID, {
     .comm = res_comm,               // Resilient communicator
     .start_time_stamp = 0,          // Initial timestamp
     .depth = 1,                     // Keep 1 old snapshot + latest
     .policy_name = FENIX_DATA_POLICY_IMR,
     .policy_value = &policy_params
   });

**Create Data Members:**

.. code-block:: cpp

   const int STATE_ID = 1, VELOCITY_ID = 2;

   // Fixed-size member
   member_create(GROUP_ID, STATE_ID,
                 state_array, array_size, MPI_DOUBLE);

   // Variable-size member
   member_create(GROUP_ID, VELOCITY_ID,
                 velocity.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

   // After resizing variable-size data:
   velocity.resize(new_size);
   int flag;
   Fenix_Data_member_attr_set(GROUP_ID, VELOCITY_ID,
                              FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                              velocity.data(), &flag);

Data Recovery - Checkpointing
------------------------------

**Basic Pattern (C++):**

.. code-block:: cpp

   using namespace fenix::data;

   // Option 1: Store and commit separately
   member_store(GROUP_ID, MEMBER_ID);
   int time_stamp;
   commit_barrier(GROUP_ID, &time_stamp);

   // Option 2: Checkpoint all members at once
   checkpoint(GROUP_ID, SUBSET_FULL, {}, &time_stamp);

**Partial Checkpoints:**

.. code-block:: cpp

   // Create subset for boundary regions
   Fenix_Data_subset boundary;
   Fenix_Data_subset_createv(2,
     (int[]){0, n-10},       // Start offsets
     (int[]){10, n-1},       // End offsets
     &boundary);

   member_store(GROUP_ID, MEMBER_ID, boundary);
   commit_barrier(GROUP_ID);

   Fenix_Data_subset_delete(&boundary);

**Staged Checkpointing (for performance):**

.. code-block:: cpp

   // Stage locally (fast, non-collective)
   member_stage(GROUP_ID, MEMBER_ID);

   // Later: store pre-staged data (collective)
   member_store(GROUP_ID, MEMBER_ID, SUBSET_PRESTAGED);
   commit_barrier(GROUP_ID);

Data Recovery - Restoration
----------------------------

**C++ API:**

.. code-block:: cpp

   using namespace fenix::data;

   // Option 1: Repair and load (collective, then local)
   member_repair(GROUP_ID, MEMBER_ID);
   member_load(GROUP_ID, MEMBER_ID);

   // Option 2: Restore (repair + load in one call)
   member_restore(GROUP_ID, MEMBER_ID);

   // Restore to custom buffer
   member_restore(GROUP_ID, MEMBER_ID, custom_buffer, buffer_size);

   // Restore specific timestamp
   member_restore(GROUP_ID, MEMBER_ID, FENIX_DATA_RESTORE_INPLACE,
                  FENIX_DATA_RESTORE_FULL, specific_time_stamp);

**Check what data was found:**

.. code-block:: cpp

   Fenix_Data_subset found_data;
   int status = member_load(GROUP_ID, MEMBER_ID,
                            FENIX_DATA_SNAPSHOT_ALL, &found_data);

   if (status == FENIX_WARNING_PARTIAL_RESTORE) {
     // Not all data was recovered
   }

   Fenix_Data_subset_delete(&found_data);

Message Logging
---------------

**C++ API:**

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   const int LOG_ID = 1;

   // Create message logger
   mlog::create(LOG_ID, res_comm, num_regions);

   // Activate and begin region
   mlog::activate(LOG_ID, region_id);

   // Or separately:
   mlog::activate(LOG_ID);
   mlog::begin_region(LOG_ID, region_id);

   // After recovery: sync to region
   mlog::sync(LOG_ID, recovery_region_id);

   // Or continue from latest:
   mlog::sync(LOG_ID, FENIX_MLOG_CONTINUE);

**Enable Automatic Recovery:**

.. code-block:: cpp

   // Set inline recovery mode
   fenix::set_option(fenix::MLOG_RECOVERY_MODE, fenix::INLINE_AUTOSYNC);

**Link to Data Member (checkpoint message logs):**

.. code-block:: cpp

   mlog::create_data_member(LOG_ID, GROUP_ID, MLOG_MEMBER_ID);

   // Checkpoint as normal
   checkpoint(GROUP_ID, SUBSET_FULL);

Error Codes
-----------

**Success:**

.. code-block:: cpp

   FENIX_SUCCESS                      // 0 - Operation succeeded

**Common Errors:**

.. code-block:: cpp

   FENIX_ERROR_UNINITIALIZED          // Fenix_Init not called
   FENIX_ERROR_INVALID_GROUPID        // Data group doesn't exist
   FENIX_ERROR_INVALID_MEMBERID       // Data member doesn't exist
   FENIX_ERROR_NODATA_FOUND           // No snapshot data available
   FENIX_ERROR_PROCESS_FAILURE        // Process failure detected

**Warnings (positive values):**

.. code-block:: cpp

   FENIX_WARNING_SPARE_RANKS_DEPLETED // Out of spares, communicator shrank
   FENIX_WARNING_PARTIAL_RESTORE      // Not all data was restored

Rank Roles
----------

.. code-block:: cpp

   // C++ API
   fenix::INITIAL_RANK    // No failures yet
   fenix::RECOVERED_RANK  // Was spare, now active
   fenix::SURVIVOR_RANK   // Survived a failure
   fenix::SPARE_RANK      // Currently a spare

   // C API
   FENIX_ROLE_INITIAL_RANK
   FENIX_ROLE_RECOVERED_RANK
   FENIX_ROLE_SURVIVOR_RANK
   FENIX_ROLE_SPARE_RANK

**Usage:**

.. code-block:: cpp

   if (fenix::role() == fenix::RECOVERED_RANK) {
     // I'm a recovered rank - restore state
     fenix::data::member_restore(GROUP_ID, MEMBER_ID);
   }

Constants
---------

**Data Recovery:**

.. code-block:: cpp

   // Subsets (specify which element ranges to checkpoint)
   FENIX_DATA_SUBSET_FULL          // All elements
   FENIX_DATA_SUBSET_EMPTY         // No elements (placeholder)
   FENIX_DATA_SUBSET_PRESTAGED     // Elements from member_stage()

   // Snapshots
   FENIX_DATA_SNAPSHOT_LATEST      // -1: Most recent snapshot
   FENIX_DATA_SNAPSHOT_ALL         // -2: Load from all snapshots

   // Restore options
   FENIX_DATA_RESTORE_INPLACE      // NULL: Restore to member's buffer
   FENIX_DATA_RESTORE_FULL         // INT_MAX: Restore all elements

   // Member attributes
   FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER
   FENIX_DATA_MEMBER_ATTRIBUTE_COUNT
   FENIX_DATA_MEMBER_ATTRIBUTE_DATATYPE

   // Policies
   FENIX_DATA_POLICY_IN_MEMORY_RAID  // IMR policy
   FENIX_DATA_POLICY_IMR             // Same as above

   // Other
   FENIX_RESIZEABLE                // Variable-size member count
   FENIX_DATA_MEMBER_ALL           // -1: All members

**Message Logging:**

.. code-block:: cpp

   FENIX_MLOG_NONE      // -1: No active log
   FENIX_MLOG_CONTINUE  // -1: Continue from latest region

Common Patterns
---------------

**Pattern 1: Basic Recovery with Checkpointing**

.. code-block:: cpp

   using namespace fenix;
   using namespace fenix::data;

   MPI_Comm res_comm;
   init({.out_comm = &res_comm, .spares = 3});

   // Setup data recovery
   const int GROUP_ID = 1, MEMBER_ID = 1;
   group_create(GROUP_ID, {.comm = res_comm});
   member_create(GROUP_ID, MEMBER_ID, data.data(), data.size(), MPI_DOUBLE);

   // Register recovery callback
   callback_register([&](MPI_Comm comm, int err) {
     if (role() == RECOVERED_RANK) {
       member_restore(GROUP_ID, MEMBER_ID);
     }
   });

   // Main loop with periodic checkpointing
   for (int iter = 0; iter < max_iter; iter++) {
     // Computation here

     if (iter % 10 == 0) {
       checkpoint(GROUP_ID, SUBSET_FULL);
     }
   }

   Fenix_Finalize();

**Pattern 2: Inline Recovery with Exceptions (C++)**

.. code-block:: cpp

   set_option(RESUME_MODE, THROW);

   try {
     // Main computation
     for (int iter = 0; iter < max_iter; iter++) {
       // MPI operations here
     }
   } catch (fenix::CommException& e) {
     // Recovery: repair communicator, restore data
     if (role() == RECOVERED_RANK) {
       data::member_restore(GROUP_ID, MEMBER_ID);
     }

     // Continue or restart loop
     goto restart_point;  // Or restructure as needed
   }

**Pattern 3: Message Logging + Checkpointing**

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   const int LOG_ID = 1;
   mlog::create(LOG_ID, res_comm, 100);  // Keep 100 regions
   mlog::activate(LOG_ID);

   for (int iter = 0; iter < max_iter; iter++) {
     // Checkpoint every 50 iterations
     if (iter % 50 == 0) {
       data::checkpoint(GROUP_ID, SUBSET_FULL);
     }

     // Log every iteration
     mlog::begin_region(LOG_ID, iter);
     // MPI communication here
   }

Running Fenix Applications
---------------------------

**Required mpiexec flags:**

.. code-block:: bash

   # ULFM fault tolerance must be enabled
   mpiexec --with-ft mpi -n 10 ./my_app

   # Common additional flags
   mpiexec --with-ft mpi \
           --map-by :oversubscribe \
           --mca async_mpi_finalize 1 \
           -n 10 ./my_app

**Testing with fault injection:**

.. code-block:: bash

   # Run with 7 active + 3 spare ranks
   mpiexec --with-ft mpi -n 10 ./my_app

   # In another terminal, kill a rank to test recovery
   kill -9 <pid_of_rank>

Quick Debugging
---------------

**Check Fenix status:**

.. code-block:: cpp

   printf("Fenix initialized: %d\n", fenix::initialized());
   printf("Current role: %d\n", fenix::role());
   printf("Last error: %d\n", fenix::error());
   printf("Remaining spares: %d\n", fenix::nspare());

**Check data group/member:**

.. code-block:: cpp

   if (!data::group_created(GROUP_ID)) {
     fprintf(stderr, "Error: Group %d not created\n", GROUP_ID);
   }

   if (!data::member_created(GROUP_ID, MEMBER_ID)) {
     fprintf(stderr, "Error: Member %d not created\n", MEMBER_ID);
   }

**Query snapshots:**

.. code-block:: cpp

   auto snapshots = data::group_snapshots(GROUP_ID);
   if (snapshots) {
     printf("Available timestamps: ");
     for (int ts : *snapshots) {
       printf("%d ", ts);
     }
     printf("\n");
   }

See Also
--------

- :doc:`quickstart` - Get started in 10 minutes
- :doc:`api/index` - Complete API reference
- :doc:`examples/index` - Working example programs
- :doc:`glossary` - Term definitions
- :doc:`troubleshooting` - Common problems and solutions
