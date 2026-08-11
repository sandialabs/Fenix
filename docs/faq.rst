Frequently Asked Questions
===========================

Quick answers to common questions about Fenix. For more detailed information, follow the links to the full documentation.

.. contents:: Questions
   :local:
   :depth: 2

General / Getting Started
--------------------------

What is Fenix?
~~~~~~~~~~~~~~

Fenix is a software library compatible with the Message Passing Interface (MPI) that enables **fault recovery without application shutdown**. When MPI ranks fail, Fenix automatically repairs communicators and can recover application state, allowing your program to continue execution.

Unlike traditional checkpoint/restart that stops and restarts your entire application from scratch, Fenix enables recovery with minimal interruption by:

- Automatically repairing MPI communicators using spare ranks
- Providing optional in-memory checkpoint/restart for application data
- Supporting optional message logging and replay for seamless recovery

:doc:`More details → <introduction>`

Do I need to restart my application when a rank fails?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**No!** That's the key benefit of Fenix. Traditional fault tolerance requires stopping and restarting the entire application. Fenix enables **in-place recovery**:

1. When a rank fails, Fenix detects it during MPI operations
2. Spare ranks automatically replace failed ranks
3. The communicator is rebuilt transparently
4. Your application continues with minimal interruption

The application never shuts down, saving significant time especially for large-scale runs.

:doc:`More details → <guides/process-recovery>`

What is ULFM?
~~~~~~~~~~~~~

ULFM (User Level Failure Mitigation) is an extension to MPI that provides the low-level mechanisms for detecting and handling rank failures without aborting the entire application. Fenix is built on top of ULFM MPI.

**Key ULFM features Fenix uses:**

- **Failure detection** during MPI operations (MPI functions return error codes instead of aborting)
- **Communicator revocation** to propagate failure knowledge (marks communicator as invalid, forcing all ranks to learn about failures)
- **Non-interruptible collective operations** for safe recovery (special collectives that complete despite additional failures)
- **Communicator shrinking and repair primitives** (functions to rebuild communicators after failures)

You need **Open MPI 5.0 or later** built with ULFM support to use Fenix.

:doc:`Installation instructions → <installation>`

Can I use Fenix with C? C++?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Yes, both!**

- **C++ API**: Include ``fenix.hpp`` and use the ``fenix::`` namespace (e.g., ``fenix::init``)
- **C API**: Include ``fenix.h`` and use ``Fenix_*`` functions (e.g., ``Fenix_Init``)

The C++ API is recommended for new applications because it provides:

- Cleaner syntax with designated initializers
- Type-safe error checking with ``fenix::error()``
- Exception-based error handling option
- Modern callback interface with ``std::function``

Both APIs provide the same functionality and can be mixed in the same codebase.

:doc:`API Reference → <api/index>`

How does Fenix detect failures?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix detects failures through the ULFM MPI runtime. Failures are detected when:

- An MPI operation involves a failed rank
- The MPI runtime reports an error for that operation
- Fenix's error handler intercepts the error and begins recovery

**Important:** Failures can only be detected during MPI function calls. Applications with long periods of computation without communication should periodically call ``Fenix_Process_detect_failures()`` to allow timely recovery.

Detection is **not collectively consistent** - some ranks may detect a failure before others. Fenix uses communicator revocation to propagate failure knowledge to all ranks.

:doc:`More details → <guides/process-recovery>`

Installation and Setup
-----------------------

What are the requirements for using Fenix?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Required:**

- Open MPI 5.0 or later with ULFM support
- CMake 3.12 or later
- C++20 compatible compiler (GCC 10+, Clang 10+)
- MPI C and C++ compilers (mpicc, mpicxx)

**Optional:**

- Doxygen (for building API documentation)
- Sphinx (for building user documentation)
- Google Test (for running tests)

The most critical requirement is Open MPI 5+ with ULFM enabled. Many system MPI installations do not have ULFM, so you may need to build from source.

:doc:`Full installation guide → <installation>`

How do I build Open MPI with ULFM support?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Download Open MPI 5.0
   wget https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.0.tar.gz
   tar xzf openmpi-5.0.0.tar.gz
   cd openmpi-5.0.0

   # Configure with fault tolerance enabled
   ./configure --prefix=$HOME/openmpi-5.0 \
     --with-ft=mpi \
     --enable-mpi-ft-mpi

   # Build and install
   make -j4
   make install

   # Add to PATH
   export PATH=$HOME/openmpi-5.0/bin:$PATH
   export LD_LIBRARY_PATH=$HOME/openmpi-5.0/lib:$LD_LIBRARY_PATH

:doc:`Complete instructions → <installation>`

Why do I get "unknown option --with-ft" error?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This means your Open MPI was not built with ULFM support. The ``--with-ft mpi`` flag is required to run Fenix applications, but only works if Open MPI was compiled with ``--with-ft=mpi`` during its build.

**Solution:** Build Open MPI from source with ULFM enabled (see above).

**Verification:**

.. code-block:: bash

   # Check for ULFM support
   ompi_info | grep -i ulfm
   ompi_info | grep -i fault

:doc:`Troubleshooting → <troubleshooting>`

Why does my program segfault in basic MPI calls?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Cause:** Multiple MPI versions on your system. Fenix compiled against one version but runtime uses another.

**Solution:** Enable the system include fix when building Fenix:

.. code-block:: bash

   cd build
   cmake ../ -DFENIX_SYSTEM_INC_FIX=ON
   make clean && make

This forces Fenix to use the correct MPI headers.

**Verification:**

.. code-block:: bash

   # Should show only ONE libmpi.so path
   ldd build/examples/08_inline_recovery/stencil_skeleton | grep mpi

:doc:`Troubleshooting → <troubleshooting>`

API and Usage
-------------

What's the difference between Fenix_Init and fenix::init?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

They provide the same functionality but with different syntax:

**C++ API (fenix::init):**

.. code-block:: cpp

   void fenix::init(fenix::InitOptions opts);

   // Usage with designated initializers
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 3});

The C++ API is much cleaner with designated initializers and doesn't require passing argc/argv explicitly.

**C API (Fenix_Init):**

.. code-block:: c

   int Fenix_Init(int* role, MPI_Comm parent, MPI_Comm* out_comm,
                  int* argc, char*** argv);

:doc:`API Reference → <api/process-recovery>`

How many spare ranks should I use?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Rule of thumb:** 5-10% of total ranks for large jobs.

**Considerations:**

- **Too few**: Risk running out during multiple failures
- **Too many**: Wastes compute resources sitting idle
- **Job length**: Longer jobs need more spares (higher failure probability)
- **Hardware reliability**: Less reliable systems need more spares

**Examples:**

- 100 ranks, 1 hour job: 2-3 spares
- 1000 ranks, 24 hour job: 50-100 spares
- 10000 ranks, 48 hour job: 500-1000 spares

Monitor your failure rates and adjust accordingly. You can always increase spares in future runs.

:doc:`Process recovery guide → <guides/process-recovery>`

What happens if I run out of spare ranks?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When more failures occur than available spare ranks, Fenix has two options:

1. **Shrink mode** (default): Continue with fewer ranks, but some ranks may have different rank IDs. Fenix sets the error code to ``FENIX_WARNING_SPARE_RANKS_DEPLETED``.

2. **Abort**: Application can choose to abort rather than continue with altered rank IDs.

**Best practices:**

- Check ``fenix::error()`` after recovery for the depleted warning
- Provision enough spares based on expected failure rate
- Consider checkpointing to disk when spares are low

:doc:`Troubleshooting → <troubleshooting>`

Can I use MPI_COMM_WORLD?
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Generally no.** You should use the resilient communicator returned by ``Fenix_Init`` instead.

Fenix can only detect and recover from failures on its resilient communicator and communicators derived from it. Using ``MPI_COMM_WORLD`` directly will bypass Fenix's fault tolerance.

**Correct usage:**

.. code-block:: cpp

   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 2});

   // Use res_comm instead of MPI_COMM_WORLD
   int rank;
   MPI_Comm_rank(res_comm, &rank);

:doc:`Quick Start → <quickstart>`

What MPI operations are supported?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**All standard MPI operations are supported** on Fenix resilient communicators:

- Point-to-point: ``MPI_Send``, ``MPI_Recv``, ``MPI_Isend``, ``MPI_Irecv``, etc.
- Collectives: ``MPI_Bcast``, ``MPI_Reduce``, ``MPI_Allreduce``, ``MPI_Barrier``, etc.
- Non-blocking operations and requests
- One-sided communication (RMA)

**Limitations:**

- Communicators must be derived from the Fenix resilient communicator
- Multiple derived communicators should be used carefully (may cause deadlock)
- Message logging (for replay) requires specific activation

:doc:`API Reference → <api/index>`

Recovery Patterns
-----------------

What's the difference between longjmp and inline recovery?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Longjmp recovery (default):**

- Automatically jumps back to ``Fenix_Init`` after failure
- Mimics traditional checkpoint/restart pattern
- Simpler to implement
- Has undefined behavior with C++ objects and compiler optimizations
- May not call destructors properly

**Inline recovery (recommended for C++):**

- Returns error code from the failing MPI call
- Continues execution inline without jumping
- More predictable behavior
- Works well with C++ exceptions
- Requires checking MPI return codes

**Example: Inline with exceptions:**

.. code-block:: cpp

   // Set inline recovery with exceptions
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

   try {
     // Your application code
   } catch (fenix::CommException& e) {
     // Handle recovery here
   }

:doc:`Process recovery guide → <guides/process-recovery>`

Should I use longjmp or inline recovery?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Recommendations:**

- **C++ applications**: Use inline recovery with exceptions (``fenix::THROW``)
- **C applications**: Either works; inline gives more control, longjmp is simpler
- **Legacy code**: Longjmp may be easier to retrofit
- **New code**: Always use inline recovery

**Why inline is better:**

- No undefined behavior from longjmp
- Proper C++ destructor calls
- More predictable with compiler optimizations
- Better integration with exception handling
- Easier to reason about control flow

:doc:`How-to guide → <howto/choose-recovery-pattern>`

How do I register recovery callbacks?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Callbacks allow you to execute custom code after communicator recovery.

**C++ API:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     printf("Recovery callback invoked!\n");
     // Restore application state here
     fenix::data::member_restore(GROUP_ID, MEMBER_ID);
   });

**C API:**

.. code-block:: c

   void my_callback(MPI_Comm repaired, int err, void* context) {
     printf("Recovery callback invoked!\n");
   }

   Fenix_Callback_register(my_callback, my_context);

**Key points:**

- Callbacks execute after communicator repair, before control returns to application
- Multiple callbacks can be registered; they execute in reverse order
- Recovered ranks (former spares) will NOT have callbacks registered
- Callbacks must handle errors that occur during recovery operations

:doc:`API Reference → <api/process-recovery>`

What is the role of a rank?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix assigns each rank a role that indicates its recovery state:

- ``FENIX_ROLE_INITIAL_RANK`` / ``fenix::INITIAL_RANK``: No failures yet
- ``FENIX_ROLE_RECOVERED_RANK`` / ``fenix::RECOVERED_RANK``: Was a spare, now active
- ``FENIX_ROLE_SURVIVOR_RANK`` / ``fenix::SURVIVOR_RANK``: Survived a failure
- ``FENIX_ROLE_SPARE_RANK`` / ``fenix::SPARE_RANK``: Currently a spare (returned at finalize)

**Usage:**

.. code-block:: cpp

   if (fenix::role() == fenix::RECOVERED_RANK) {
     printf("I'm a recovered rank - need to restore state\n");
     fenix::data::member_restore(GROUP_ID, MEMBER_ID);
   }

Roles help the application customize behavior based on recovery state.

:doc:`API types → <api/types>`

Data Recovery
-------------

Do I need to checkpoint my data?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**It depends on your application:**

**You need data recovery if:**

- Your application has stateful data that changes over time
- Lost state would require recomputing from the beginning
- Recovery requires restoring specific values

**You may not need it if:**

- Application is stateless or easily reconstructed
- Data can be recovered from neighboring ranks (e.g., stencil codes)
- External storage already provides redundancy

**Best practice:** Start with just process recovery, add data recovery only for critical state.

:doc:`Data recovery guide → <guides/data-recovery>`

How do I checkpoint data with Fenix?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Basic pattern:**

.. code-block:: cpp

   using namespace fenix::data;

   const int GROUP_ID = 1, MEMBER_ID = 1;

   // Create data group and member
   group_create(GROUP_ID, comm, timestamp, depth,
                FENIX_DATA_POLICY_IN_MEMORY_RAID, policy_params);
   member_create(GROUP_ID, MEMBER_ID, my_data.data(),
                 my_data.size(), MPI_DOUBLE);

   // During execution: checkpoint periodically
   member_store(GROUP_ID, MEMBER_ID, SUBSET_FULL);
   commit_barrier(GROUP_ID);

   // After recovery: restore
   member_restore(GROUP_ID, MEMBER_ID);

:doc:`How-to guide → <howto/checkpoint-data>`

What is a data group?
~~~~~~~~~~~~~~~~~~~~~

A **data group** is a container for related data members that are committed together as a transaction. It provides:

1. **Transaction semantics**: All members in a group are committed atomically
2. **Collective scope**: Defines which ranks participate in storage operations
3. **Policy configuration**: Each group can use a different redundancy policy

**Example:**

.. code-block:: cpp

   // Create group for iteration N data
   group_create(GROUP_ID, comm, iteration, depth,
                FENIX_DATA_POLICY_IN_MEMORY_RAID, params);

   // Add multiple members to the group
   member_create(GROUP_ID, STATE_ID, &state, ...);
   member_create(GROUP_ID, VELOCITY_ID, &velocity, ...);
   member_create(GROUP_ID, POSITION_ID, &position, ...);

   // Commit all together
   commit_barrier(GROUP_ID);

:doc:`Data recovery guide → <guides/data-recovery>`

Do I need to checkpoint everything?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**No!** Checkpoint only what's necessary for recovery:

**What to checkpoint:**

- Critical state that's expensive to recompute
- Data that changes over time
- Initial conditions if non-trivial to regenerate

**What NOT to checkpoint:**

- Temporary/scratch arrays
- Data that can be quickly recalculated
- Read-only input data (can be re-read from disk)
- Derived values that are cheap to compute

**Use partial checkpoints** to checkpoint only important element ranges. A **subset** specifies which elements to checkpoint instead of the entire array:

.. code-block:: cpp

   // Checkpoint only the boundary region
   Fenix_Data_subset boundary;
   Fenix_Data_subset_createv(2,
     (int[]){0, n-10},          // Start indices
     (int[]){10, n-1},          // End indices
     &boundary);

   member_store(GROUP_ID, MEMBER_ID, boundary);

:doc:`How-to guide → <howto/partial-checkpoints>`

What redundancy policies are available?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix currently provides the **In-Memory Redundancy (IMR)** policy with two modes:

**Mode 1 (RAID-1 style mirroring):**

- Ranks paired into partners
- Each rank stores its data + partner's data
- Memory usage: 2x per checkpoint
- Fast, simple, reliable if only one partner fails

**Mode 5 (RAID-5 style parity):**

- Ranks grouped into parity groups
- Each rank stores its data + parity portion
- Memory usage: GroupSize/(GroupSize-1)x per checkpoint
- Trades computation for memory savings
- Example: Group size 5 = 1.25x memory (vs 2x for Mode 1)

:doc:`IMR policy details → <guides/imr-policy>`

Can I use custom data recovery instead of Fenix data API?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Yes!** Fenix data recovery is optional. You can:

**Alternative approaches:**

1. **Use external libraries**: VeloC, SCR, HDF5, etc.
2. **Application-specific recovery**: Interpolate from neighbors, recalculate, etc.
3. **Disk-based checkpointing**: Traditional checkpoint files
4. **Hybrid**: Mix Fenix data recovery with other approaches

**Example: Neighbor-based recovery in stencil code:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     if (fenix::role() == fenix::RECOVERED_RANK) {
       // Request halo data from neighbors
       exchange_halos(repaired, my_data);
       // Interpolate interior from boundaries
       interpolate_interior(my_data);
     }
   });

Fenix only requires process recovery; data recovery is completely optional.

:doc:`How-to guide → <howto/custom-recovery>`

How often should I checkpoint?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Trade-off:** More frequent = less replay time, but more overhead.

**Guidelines:**

- **Compute-intensive**: Every 5-10 minutes of wall time
- **Communication-intensive**: More frequently (every 1-2 minutes)
- **Large data**: Less frequently to reduce overhead
- **With message logging**: Less frequently (messages can be replayed)

**Adaptive strategy:**

.. code-block:: cpp

   // Checkpoint every N iterations
   if (iteration % checkpoint_interval == 0) {
     fenix::data::commit_barrier(GROUP_ID);
   }

   // Adjust interval based on failure rate
   if (failures_detected > threshold) {
     checkpoint_interval /= 2;  // Checkpoint more often
   }

**Monitor overhead** and adjust based on your application's performance profile.

:doc:`Performance tuning → <howto/performance-tuning>`

Message Logging
---------------

What is message logging?
~~~~~~~~~~~~~~~~~~~~~~~~~

Message logging records MPI communication patterns so they can be automatically replayed after recovery. This eliminates the need to recompute from the last checkpoint.

**Benefits:**

- Reduced recovery time (replay messages instead of recomputing)
- Less frequent checkpointing needed
- Localized recovery (only failed ranks need to catch up)

**Cost:**

- Memory overhead for storing message logs
- Some performance overhead during normal execution
- Complexity in managing log regions

**Example:**

.. code-block:: cpp

   namespace mlog = fenix::mlog;

   mlog::create(LOG_ID, comm, num_regions);
   mlog::activate(LOG_ID);

   for (int iter = 0; iter < max_iter; iter++) {
     mlog::begin_region(LOG_ID, iter);
     // MPI communication here is automatically logged
     mlog::end_region(LOG_ID);
   }

:doc:`Message recovery API → <api/message-recovery>`

Should I use message logging?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Use message logging if:**

- Communication patterns are expensive to repeat
- You want faster recovery times
- You can afford memory overhead for logs
- Your application has clear iteration boundaries

**Skip message logging if:**

- Communication is minimal or cheap
- Memory is very constrained
- Application is primarily compute-bound
- Checkpointing alone provides sufficient recovery

**Recommendation:** Start without message logging, add it later if recovery time is too slow.

:doc:`How-to guide → <howto/message-logging>`

How does message logging work with checkpointing?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

They work together to minimize recovery time:

1. **Checkpoint periodically** (e.g., every 50 iterations)
2. **Log messages continuously** in smaller regions (e.g., every iteration)
3. **On recovery:**

   - Restore from last checkpoint
   - Replay logged messages from checkpoint to failure point
   - Continue execution

**Example:**

.. code-block:: cpp

   for (int iter = 0; iter < max_iter; iter++) {
     // Checkpoint every 50 iterations
     if (iter % 50 == 0) {
       fenix::data::commit_barrier(GROUP_ID);
     }

     // Log every iteration
     mlog::begin_region(LOG_ID, iter);
     do_mpi_communication();
     mlog::end_region(LOG_ID);
   }

This minimizes both checkpoint overhead and recovery time.

:doc:`Message recovery guide → <api/message-recovery>`

Performance
-----------

How much overhead does Fenix add?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Typical overhead:**

- **Process recovery only**: <1% (minimal - just error checking)
- **With data checkpointing**: 2-10% (depends on checkpoint frequency and size)
- **With message logging**: 5-15% (depends on communication intensity)

**Factors affecting overhead:**

- Checkpoint frequency (more frequent = higher overhead)
- Data size (larger checkpoints = more overhead)
- Message logging configuration
- Network performance
- Redundancy policy (Mode 1 vs Mode 5)

**Optimization strategies:**

1. Checkpoint less frequently
2. Use partial checkpoints (subsets)
3. Choose efficient redundancy policy
4. Tune message log window size
5. Build with Release mode

:doc:`Performance tuning → <howto/performance-tuning>`

How can I reduce checkpoint overhead?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Strategies:**

1. **Checkpoint less often**:

   .. code-block:: cpp

      if (iteration % 100 == 0)  // Was: % 10
        fenix::data::commit_barrier(GROUP_ID);

2. **Use partial checkpoints**:

   .. code-block:: cpp

      // Only checkpoint important subset
      member_store(GROUP_ID, MEMBER_ID, critical_region_subset);

3. **Reduce checkpoint depth**:

   .. code-block:: cpp

      // Keep only 1 snapshot instead of many
      group_create(GROUP_ID, comm, timestamp,
                   1,  // depth=1 keeps only latest
                   policy, params);

4. **Choose efficient policy**:

   - Mode 1 for small data (faster)
   - Mode 5 for large data (less memory)

5. **Overlap computation and checkpointing** (if possible)

:doc:`Optimization guide → <howto/optimize-checkpoints>`

Why is recovery taking so long?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Common causes:**

1. **Infrequent checkpoints**: More iterations to replay
2. **No message logging**: Must recompute instead of replay
3. **Large data restoration**: Slow network or large checkpoint
4. **Multiple simultaneous failures**: More ranks need recovery

**Solutions:**

1. **Checkpoint more frequently**:

   .. code-block:: cpp

      if (iteration % 5 == 0)  // Was: % 50
        commit_barrier(GROUP_ID);

2. **Enable message logging**:

   .. code-block:: cpp

      mlog::create(LOG_ID, comm, window_size);
      mlog::activate(LOG_ID);

3. **Optimize data transfer**: Use faster policy, smaller checkpoints
4. **Profile recovery**: Identify bottlenecks

:doc:`Troubleshooting → <troubleshooting>`

Troubleshooting
---------------

My program hangs at MPI_Init
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Cause:** Missing ``--with-ft mpi`` flag or MPI misconfiguration.

**Solution:**

.. code-block:: bash

   # Correct: Use --with-ft mpi
   mpiexec --with-ft mpi -n 4 ./my_app

   # If still hangs, test MPI directly
   mpiexec --with-ft mpi -n 2 hostname

If hostname test fails, your MPI installation has issues.

:doc:`Troubleshooting → <troubleshooting>`

Program crashes instead of recovering
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Common causes:**

1. **Not enough spares**: Increase spare count
2. **Error in callback**: Add error checking
3. **Message logging not activated**: Call ``mlog::activate()``

**Debug:**

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     if (fenix::error() != FENIX_SUCCESS) {
       printf("Recovery failed: %d\n", fenix::error());
       return;
     }
     // Recovery logic...
   });

:doc:`Troubleshooting → <troubleshooting>`

Recovered data is incorrect
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Common causes:**

1. **Buffer pointer changed**: Update member attribute after resize
2. **Wrong snapshot**: Specify which snapshot to restore
3. **Subset mismatch**: Ensure store/restore use same subset

**Solution for resizable data:**

.. code-block:: cpp

   data.resize(new_size);

   // Update buffer pointer after resize
   Fenix_Data_member_attr_set(
     group, member,
     FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
     data.data(), &flag
   );

:doc:`Troubleshooting → <troubleshooting>`

Tests fail or hang
~~~~~~~~~~~~~~~~~~

**Quick checks:**

.. code-block:: bash

   # Verify MPI works
   mpiexec --with-ft mpi -n 2 hostname

   # Run simple test
   cd build
   ctest -R 01_hello_world -V --timeout 20

   # Check for multiple MPI versions
   ldd ./my_app | grep libmpi

**If tests timeout:** Your MPI isn't configured for fault tolerance.

**If specific tests fail:** Run with verbose output and check logs:

.. code-block:: bash

   ctest -R test_name -V --timeout 20
   cat Testing/Temporary/LastTest.log

:doc:`Troubleshooting → <troubleshooting>`

Comparison with Other Approaches
---------------------------------

How does Fenix compare to VeloC or SCR?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**VeloC / SCR (disk-based checkpoint/restart):**

- Checkpoint to disk (parallel file system)
- Full application restart on failure
- Mature, well-tested
- Good for very large data (TB scale)
- Higher recovery overhead (minutes)

**Fenix (in-memory recovery):**

- Checkpoint to memory on other ranks
- No application restart needed
- In-place recovery
- Lower recovery overhead (seconds)
- Limited by available memory

**When to use each:**

- **Fenix**: Fast recovery, moderate data size, frequent failures
- **VeloC/SCR**: Massive data, rare failures, need persistence across job failures
- **Both**: Fenix for fast in-memory, periodic VeloC checkpoints for durability

**They can complement each other:** Use Fenix for frequent small failures, VeloC for rare catastrophic failures.

How does Fenix compare to full application restart?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - Traditional Restart
     - Fenix
   * - Application shutdown
     - Yes (entire job)
     - No
   * - Time to recover
     - Minutes to hours
     - Seconds to minutes
   * - Resource allocation
     - May need to requeue
     - Uses existing allocation
   * - Failed rank replacement
     - Restart all ranks
     - Replace only failed ranks
   * - Checkpoint location
     - Disk (PFS)
     - Memory (other ranks)
   * - Overhead during execution
     - Disk I/O
     - Memory + network
   * - Maturity
     - Very mature
     - Active development

**Key benefit:** Fenix avoids the expensive teardown/restart cycle.

What about MPI spare processes without Fenix?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ULFM provides the low-level primitives (``MPIX_Comm_shrink``, ``MPIX_Comm_spawn``, etc.), but applications must:

1. Manually detect failures
2. Manually rebuild communicators
3. Manually redistribute data
4. Handle all edge cases and race conditions

**Fenix automates this complexity:**

- Automatic failure detection and propagation
- Automatic communicator repair with spare management
- Built-in data redundancy policies
- Message logging infrastructure
- Tested error handling and edge cases

**You can use ULFM directly** for maximum control, but Fenix significantly reduces development effort.

Is Fenix suitable for production HPC applications?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Yes, with caveats:**

**Fenix is production-ready for:**

- Applications where failure recovery provides value
- Runs where occasional failures are expected
- Environments with Open MPI 5+ support

**Consider carefully if:**

- Your application runs for <1 hour (failures unlikely)
- System has very low failure rates
- MPI 5+ with ULFM not available
- Application has complex MPI usage patterns

**Production use recommendations:**

1. Thoroughly test recovery on your specific application
2. Monitor failure rates and adjust spare count
3. Start with conservative checkpoint frequency
4. Have fallback plan if recovery fails (disk checkpoints)
5. Profile performance overhead in your environment

Several HPC centers and research groups are actively using Fenix in production.

Getting More Help
-----------------

Where can I find more examples?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix includes numerous examples in the ``examples/`` directory:

- **Example 01**: Hello World - basic process recovery
- **Example 08**: Inline Recovery - modern pattern with stencil skeleton
- **Data recovery examples**: Checkpointing patterns
- **Message logging examples**: Communication replay

Build with ``-DBUILD_EXAMPLES=ON`` to compile them.

:doc:`Examples documentation → <examples/index>`

Where should I report bugs or ask questions?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**GitHub Issues:** https://github.com/sandialabs/Fenix/issues

**What to include:**

- Fenix version (``git rev-parse HEAD``)
- Open MPI version (``mpiexec --version``)
- OS and compiler versions
- Minimal reproducible example
- Full error message and backtrace

**Before reporting:**

1. Check :doc:`troubleshooting` guide
2. Search existing GitHub issues
3. Try the latest Fenix version

How do I stay updated on Fenix development?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **GitHub repository**: https://github.com/sandialabs/Fenix
- **Watch releases**: Get notified of new versions
- **Read commit messages**: Track recent changes
- **Check the documentation**: Updated with each release

Fenix is under active development. New features and improvements are added regularly.

What if my question isn't answered here?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Try these resources:**

1. :doc:`Troubleshooting Guide <troubleshooting>` - Common problems and solutions
2. :doc:`API Reference <api/index>` - Complete function documentation
3. :doc:`Guides <guides/index>` - Conceptual explanations
4. :doc:`How-To Guides <howto/index>` - Task-focused recipes
5. :doc:`Examples <examples/index>` - Working code examples

**Still stuck?** Open an issue on GitHub with your specific question. Include context about what you're trying to accomplish.
