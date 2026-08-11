Architecture
============

This guide provides a comprehensive explanation of Fenix's internal architecture,
design decisions, and how the library's components work together to enable fault
tolerance in MPI applications. It is intended for developers who want to understand
how Fenix works internally, contribute to the project, or make informed decisions
about integrating Fenix into complex applications.

----

Overview
--------

Fenix is built as a layer on top of MPI with ULFM (User-Level Failure Mitigation)
support. The architecture is designed around three key principles:

1. **Separation of concerns**: Process recovery, data recovery, and message recovery
   are independent components that can be used together or separately.
2. **Localized recovery**: Where possible, recovery operations are confined to
   subsets of ranks to minimize global coordination overhead.
3. **Flexible recovery patterns**: Applications can choose between longjmp-based,
   inline callback-based, or exception-based recovery depending on their needs.

The library maintains a global runtime state (``fenix_rt``) that tracks the
resilient communicator, spare rank pool, failure lists, and registered callbacks.
This state is carefully managed across recovery cycles to ensure consistency.

----

Three Core Components
---------------------

Process Recovery
~~~~~~~~~~~~~~~~

Process recovery is the foundation of Fenix. Its primary responsibility is to
maintain a resilient MPI communicator that survives rank failures.

**Spare Rank Architecture:**

At initialization, Fenix divides the world communicator into two groups:

- **Active ranks**: Participate in the application's computation
- **Spare ranks**: Held in reserve, waiting in a loop inside ``Fenix_Init``

Spare ranks do not execute application code until they are activated to replace
failed ranks. This design choice trades idle resources for fast recovery - when
a failure occurs, spare ranks are already running and can immediately participate
in communicator repair without the need to spawn new processes.

**Why spare ranks instead of dynamic process creation?**

The spare rank model was chosen for several reasons:

1. **Speed**: Spawning new MPI processes is slow and may not be possible in some
   HPC environments where process managers are restrictive.
2. **Simplicity**: Managing a fixed pool of pre-allocated ranks is simpler than
   coordinating dynamic process creation across potentially heterogeneous schedulers.
3. **Predictability**: Applications know at initialization time the maximum number
   of failures they can tolerate.

The tradeoff is resource efficiency - spare ranks consume CPU allocation but perform
no useful work until a failure occurs.

**Communicator Repair Process:**

When a failure is detected (typically when an MPI operation returns an error),
Fenix performs the following steps:

1. **Revocation**: The failed communicator is revoked using ULFM's
   ``MPIX_Comm_revoke``, ensuring all ranks learn about the failure even if they
   didn't directly communicate with the failed rank.

2. **Agreement**: Surviving ranks use ``MPIX_Comm_agree`` to achieve consensus on
   which recovery location to enter. This prevents deadlock when ranks detect
   failures at different points in the code.

3. **Shrinking**: The communicator is shrunk using ``MPIX_Comm_shrink`` to remove
   failed ranks and create a new temporary communicator.

4. **Spare activation**: If spare ranks are available, they are woken from their
   waiting loop and join the communicator repair. The new communicator is constructed
   to maintain the original size and rank IDs where possible.

5. **Rank mapping**: Fenix maintains a mapping between original rank IDs and current
   rank IDs. When spares are exhausted, some ranks may receive new IDs, but Fenix
   provides this information to the application.

Data Recovery
~~~~~~~~~~~~~

Data recovery provides an in-memory checkpoint/restart system with pluggable
redundancy policies. The architecture consists of four key abstractions:

**Data Groups:**

A data group represents a collection of data members that are committed together,
providing transaction semantics. Groups are associated with:

- An MPI communicator (the ranks participating in storage)
- A redundancy policy (how data is distributed and protected)
- A timestamp depth (how many checkpoints to retain)

Groups enable applications to maintain multiple independent checkpoint streams or
to checkpoint different data with different policies.

**Data Members:**

A data member describes a specific piece of application data within a group:

- Memory location and size
- MPI datatype
- Whether it's fixed-size or resizable
- Optional attributes (e.g., subset information)

Members can be stored, staged (prepared for storage without committing), and restored.
The library tracks which members have been stored at each timestamp.

**Data Subsets:**

Subsets enable partial checkpointing - storing only modified portions of large arrays.
This is critical for applications with large state vectors where only small portions
change between checkpoints.

Subsets are represented as collections of (start, end) index ranges. The library
can compute unions, intersections, and complements of subsets. During restore,
applications can query which subset was actually stored and resize their buffers
accordingly.

**Data Buffers and Serialization:**

Internally, Fenix uses a serialization system to prepare data for distribution:

- ``mstream``: A memory stream abstraction for serializing data
- ``mfile``: A memory buffer for storing serialized data
- ``serializer``: Handles MPI datatype serialization including derived types

This abstraction layer allows the redundancy policies to work with opaque data
blobs, simplifying their implementation.

**Redundancy Policies:**

Policies implement the actual storage and recovery strategy. The IMR (In-Memory
RAID) policy is currently the primary implementation:

- **Mode 1 (Mirroring)**: Pairs ranks together. Each rank stores its own data plus
  its partner's data. For odd-sized communicators, one group of three uses chained
  storage. Can tolerate single failures within each pair.

  *Tradeoff*: 2x memory overhead, zero computation overhead, simple recovery logic.

- **Mode 5 (Parity)**: Forms groups of N ranks. Each rank stores its own data plus
  1/(N-1) parity information computed via XOR. Can tolerate single failures within
  each group.

  *Tradeoff*: N/(N-1) memory overhead (e.g., 1.5x for N=3), O(M) computation for
  parity, more complex recovery but better memory efficiency for large groups.

**Why multiple policies?**

Applications have different memory budgets and failure models:

- Memory-constrained applications benefit from Mode 5's efficiency
- Compute-constrained applications may prefer Mode 1's zero-computation approach
- Applications with known failure patterns (e.g., specific nodes more likely to fail)
  can tune the separation parameter to avoid collocating redundancy

**Localized Recovery:**

The IMR policy is designed for localized recovery. When failures occur:

1. Each storage group operates independently
2. Only ranks within a group that lost a member need to communicate
3. Groups without failures can immediately return from recovery operations
4. No global barriers are required

This means a single-node failure only slows down the small group of ranks
responsible for recovering that node's data, not the entire application.

Message Recovery
~~~~~~~~~~~~~~~~

Message logging enables recovery without rolling back to a checkpoint by replaying
lost in-flight messages. This is especially powerful for applications with expensive
computation between checkpoints.

**Sender-Based Logging:**

Fenix uses a sender-based logging approach:

- Each rank logs the messages it sends
- After failure, surviving ranks replay messages to recovered ranks
- Failed ranks' state comes from checkpoints; their messages come from senders

This design choice vs. receiver-based logging:

- **Pro**: Simpler to implement, no need to coordinate logging with receives
- **Pro**: Works naturally with MPI's non-deterministic message ordering
- **Con**: Requires all communication partners of failed ranks to participate in replay

**Region Management:**

Messages are organized into regions with generation numbers:

- Applications call ``mlog::begin_region(id)`` to start a new region
- All sends in that region are logged together
- Regions form a sliding window - old regions are automatically garbage collected
- The depth parameter controls how many regions to retain

Regions map naturally to application concepts like timesteps or iterations.

**Message Log Architecture:**

Three-level hierarchy:

1. **Message Logging Instance** (``mlog``): Top-level handle for a logging stream
2. **Communicator Log** (``comm_log``): Per-communicator logging state
3. **Rank Log** (``rank_log``): Per-source-rank message queue

Each rank log contains ``msg_log`` entries describing individual messages (destination,
tag, data buffer).

**Replay Mechanism:**

After failure, Fenix automatically synchronizes message logs:

1. Recovered ranks communicate which generation they recovered to
2. Surviving ranks identify which messages need to be replayed
3. Messages are resent in region order (not necessarily original order within a region)
4. Recovered ranks execute normal MPI receives and transparently get replayed messages

**MPI Function Interception:**

Fenix intercepts MPI communication functions via the PMPI profiling interface:

- Point-to-point sends are logged when the send completes
- Collective operations are handled carefully to avoid logging redundant data
- Non-blocking operations are tracked and logged upon completion

**Integration with Data Recovery:**

Message logs can be stored as data members in data groups. This enables:

- Checkpointing the message log itself
- Recovering the log after failure to enable replay
- Automatically garbage collecting logs older than the checkpoint

----

Recovery Mechanisms
-------------------

Fenix supports three recovery mechanisms, each with different tradeoffs:

Longjmp-Based Recovery
~~~~~~~~~~~~~~~~~~~~~~~

The traditional Fenix recovery pattern:

.. code-block:: text

    Application Start
          |
          v
    Fenix_Init  <---------------------+
          |                           |
          v                           |
    [Application work]                |
          |                           |
          v                           |
    MPI_Send --> FAILURE DETECTED     |
          |            |              |
          v            v              |
    [Continue]   Communicator Repair  |
          |            |              |
          |            v              |
          |      longjmp to init -----+
          v
    Fenix_Finalize

**How it works:**

1. ``Fenix_Init`` saves a ``jmp_buf`` using ``setjmp``
2. The error handler calls ``longjmp`` after communicator repair
3. Control returns to ``Fenix_Init`` as if the function just ran
4. Recovered ranks receive a different role flag

**Advantages:**

- Minimal code changes - application naturally re-initializes
- Simple mental model - mimics process restart

**Disadvantages:**

- Undefined behavior in C/C++ for non-trivial applications
- Destructors are not called for stack-allocated objects
- Compiler optimizations may break assumptions
- Cannot recover resources acquired after init

**When to use:**

Simple C applications where re-initialization from scratch is acceptable.

Inline Recovery with Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Non-jumping recovery allows finer-grained control:

.. code-block:: text

    Application Start
          |
          v
    Fenix_Init
          |
          v
    Register callback
          |
          v
    [Application work]
          |
          v
    MPI_Send --> FAILURE DETECTED
          |            |
          v            v
    returns error  Communicator Repair
          |            |
          |            v
          |      Invoke callbacks
          |            |
          +------------+
          |
          v
    Application checks error
          |
          v
    [Recovery logic]
          |
          v
    [Continue work]

**How it works:**

1. Initialize Fenix with non-jumping mode
2. Register callbacks to restore critical state
3. MPI operations return error codes after repair
4. Application detects error and executes recovery logic

**Advantages:**

- No undefined behavior
- Resources can be properly cleaned up
- Application can recover inline without full re-initialization
- Callbacks run with repaired communicator

**Disadvantages:**

- Must check return codes of MPI operations
- More invasive code changes
- Application must manage recovery logic explicitly

**When to use:**

Applications that can efficiently continue from their current state after repairing
data, or applications where full re-initialization is expensive.

Exception-Based Recovery (C++)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The recommended pattern for C++ applications:

.. code-block:: cpp

    Fenix_Init(/* non-jumping mode */);
    fenix::callback_register([](MPI_Comm comm, int err) {
        throw fenix::CommException(comm, err);
    });

    while (true) {
        try {
            // All application work
            break; // Normal exit
        } catch (fenix::CommException& e) {
            // Recovery logic
            restore_data();
        }
    }

**How it works:**

1. Initialize in non-jumping mode
2. Register callback that throws an exception
3. Exception propagates up to application catch block
4. Application recovers and continues

**Advantages:**

- Natural C++ pattern
- RAII works correctly - destructors are called
- Clean separation between normal and recovery logic
- No undefined behavior

**Disadvantages:**

- Requires C++ and exception support
- Small performance overhead for exception handling infrastructure

**When to use:**

Any C++ application. This is the recommended approach.

----

Interaction with MPI ULFM
-------------------------

Fenix is built on top of MPI with User-Level Failure Mitigation (ULFM) extensions.
Understanding this relationship is key to understanding Fenix's capabilities and
limitations.

What ULFM Provides
~~~~~~~~~~~~~~~~~~

ULFM extends MPI with four key capabilities:

1. **Error return**: Failed MPI operations return ``MPI_ERR_PROC_FAILED`` or
   ``MPI_ERR_REVOKED`` instead of aborting.

2. **Revocation** (``MPIX_Comm_revoke``): Propagates failure notification to all
   ranks in a communicator, even those that didn't directly communicate with the
   failed rank.

3. **Shrinking** (``MPIX_Comm_shrink``): Creates a new communicator excluding failed
   ranks. This operation is failure-resilient - it will succeed even if additional
   failures occur during its execution.

4. **Agreement** (``MPIX_Comm_agree``): Achieves consensus on an integer value across
   all ranks. Also failure-resilient and used for coordination.

How Fenix Uses ULFM
~~~~~~~~~~~~~~~~~~~~

**Error Handling:**

Fenix installs a custom MPI error handler on the resilient communicator:

.. code-block:: cpp

    MPI_Comm_create_errhandler(__fenix_test_MPI, &fenix_rt.mpi_errhandler);
    MPI_Comm_set_errhandler(*fenix_rt.world, fenix_rt.mpi_errhandler);

When an MPI operation fails, this handler:

1. Checks if Fenix should ignore the error (for advanced use cases)
2. Revokes the communicator
3. Initiates the repair process
4. Either longjmps or returns an error based on configuration

**Agreement for Consistency:**

Before any non-interruptible collective operation (like ``MPIX_Comm_shrink``), Fenix
uses ``MPIX_Comm_agree`` to ensure all ranks agree on which recovery location to
enter:

.. code-block:: cpp

    int location_id = get_current_location();
    MPIX_Comm_agree(comm, &location_id);

If ranks disagree (because they detected failures at different points), all ranks
enter recovery. This prevents deadlock from ranks calling ULFM collectives in
different orders.

**Failure Detection:**

Fenix inherits ULFM's detection model:

- Failures are only detected during MPI operations
- Detection is not globally consistent - some ranks may learn about failures before
  others
- Revocation provides eventual consistency

This is why long computation phases benefit from periodic calls to
``Fenix_Process_detect_failures`` - it calls ``MPIX_Comm_agree`` to check for
failures even when no real communication is needed.

**Limitations:**

Fenix cannot detect or recover from:

- Failures on communicators not derived from the Fenix resilient communicator
- Silent data corruption
- Failures during non-MPI operations (though the next MPI call will detect it)

----

Design Decisions and Tradeoffs
-------------------------------

Why Spare Ranks?
~~~~~~~~~~~~~~~~

**Alternative considered**: Dynamic process spawning via ``MPI_Comm_spawn``

**Why spare ranks won:**

1. **Performance**: Spawning is slow (seconds to minutes on some systems)
2. **Portability**: Not all MPI implementations support spawn after init
3. **Simplicity**: No need to propagate state to newly spawned processes
4. **Predictability**: Fixed resource allocation known at start time

**Cost**: Wasted resources if failures are rare. For a 1000-rank job with 10 spare ranks,
1% of resources are idle. This is acceptable for most HPC applications where
reliability is more valuable than 1% efficiency.

Why Multiple Recovery Mechanisms?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**longjmp**: Historical default, works for simple C codes, familiar mental model.

**Inline**: Required for correct behavior in many real applications, enables
efficient partial recovery.

**Exceptions**: Best practice for C++, leverages language features.

Providing all three supports the diversity of MPI applications - from legacy Fortran
codes wrapped in C to modern C++ applications.

Why Multiple Redundancy Policies?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Applications have fundamentally different constraints:

- **Memory-bound** applications (large simulation state): Need Mode 5's efficiency
- **Compute-bound** applications: Prefer Mode 1 to avoid parity computation
- **I/O-bound** applications: May not use Fenix data recovery at all, preferring
  disk checkpoints

The policy system keeps the door open for future additions:

- Disk-based policies for very large state
- Hybrid policies that store hot data in memory and cold data on disk
- Application-specific policies that exploit structure in the data

Localized vs. Global Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix strongly prefers localized recovery where possible:

- **Data recovery**: Groups recover independently
- **Message recovery**: Only message partners of failed ranks participate in replay

**Why localize?**

At scale (10K+ ranks), coordinating all ranks for every failure is untenable:

- Network contention from all-to-all communication
- Probability of cascading failures during global barriers
- Wasted time on ranks that don't need to participate

**When global coordination is unavoidable:**

- Communicator repair (must agree on new communicator)
- Failure detection propagation (revocation is collective)
- Initial agreement on recovery location

These operations use ULFM's failure-resilient collectives to minimize the chance of
deadlock or cascading failures.

----

Internal Components and Data Structures
----------------------------------------

Global Runtime State (fenix_rt)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A singleton structure containing:

.. code-block:: cpp

    struct {
        MPI_Comm* world;              // Original world communicator
        MPI_Comm new_world;           // Current resilient communicator
        MPI_Comm* user_world;         // Pointer to output communicator

        int spare_ranks;              // Number of spare ranks reserved
        int num_initial_ranks;        // Original size of application

        Fenix_Rank_role role;         // Current rank's role
        int* fail_world;              // Array of failed rank IDs
        int fail_world_size;          // Number of failed ranks

        jmp_buf* recover_environment; // For longjmp-based recovery
        MPI_Errhandler mpi_errhandler; // Custom error handler
        MPI_Op agree_op;              // Custom agree operation

        DataRecovery* data_recovery;  // Data recovery state
        // ... additional state ...
    } fenix_rt;

This state persists across recovery cycles. Careful management is critical - for
example, ``new_world`` must be updated atomically during repair to avoid races with
concurrent MPI operations.

Data Recovery State
~~~~~~~~~~~~~~~~~~~

Hierarchical structure:

.. code-block:: text

    DataRecovery
      |
      +-- vector<DataGroup>
            |
            +-- Communicator info
            +-- Policy instance
            +-- Timestamp depth
            +-- vector<DataMember>
                  |
                  +-- Buffer pointer
                  +-- MPI_Datatype
                  +-- Size / count
                  +-- Attributes
                  +-- Staged subsets

Each data group maintains independent state. Groups can be created and destroyed
multiple times (e.g., recreated after each failure). Members within a group are
identified by integer IDs.

The policy instance is opaque to the core data recovery logic. Policies implement
a virtual interface:

.. code-block:: cpp

    class DataPolicy {
        virtual void store(members, subset) = 0;
        virtual void commit(timestamp) = 0;
        virtual void restore(members, timestamp) = 0;
    };

Message Logging State
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

    MessageLog (instance)
      |
      +-- depth (number of regions to retain)
      +-- active (whether logging is enabled)
      +-- CommLog (per-communicator)
            |
            +-- generation (current region ID)
            +-- map<int, RankLog> (source rank -> log)
                  |
                  +-- vector<MsgLog> (messages from that rank)
                        |
                        +-- dest, tag, count, datatype
                        +-- buffer (serialized data)

Logs are garbage collected using a sliding window. When entering region N, regions
older than N - depth are freed.

Coordination Protocols
~~~~~~~~~~~~~~~~~~~~~~

**Store/Commit Protocol:**

1. Application calls ``member_store`` (local operation)
2. Data is serialized and staged but not sent
3. Application calls ``commit`` (collective operation)
4. Policy distributes data according to its strategy
5. All ranks agree on successful commit (using ULFM agree)
6. Timestamp is incremented

This protocol minimizes communication by batching all stores in a commit.

**Restore Protocol:**

1. Recovered rank calls ``member_restore``
2. Policy identifies which ranks hold redundant copies
3. Data is fetched from survivors (may require parity computation)
4. Data is deserialized into application buffer
5. Rank participates in any in-flight commit operations

Restored data is always from the latest committed timestamp by default.

**Failure During Recovery:**

If failures occur during store/commit/restore:

- The operation fails atomically
- The communicator is revoked and repaired
- The operation is retried with the new communicator
- Applications can wrap recovery operations in retry loops

----

Performance Characteristics
---------------------------

**Failure-Free Overhead:**

- Process recovery: ~0 overhead (error handler is not invoked)
- Data recovery: O(M) for store/commit where M is data size, determined by policy
- Message logging: O(N) per send where N is message size (memory copy)

**Recovery Time:**

- Communicator repair: O(log P) where P is number of ranks (tree-based collectives)
- Data restore: O(M/B) where B is bandwidth, plus policy-specific computation
- Message replay: O(N * R) where N is messages to replay and R is average message size

**Scalability:**

Fenix has been tested to thousands of ranks. The localized recovery design ensures
that recovery overhead does not grow with the total number of ranks, only with the
size of the affected groups.

**Failure Rates:**

Spare rank model supports consecutive failures up to the number of spare ranks. After
spare ranks are exhausted:

- Communicator continues to shrink
- Some ranks may change IDs (library warns application)
- Further failures continue to work but application size decreases

----

Summary
-------

Fenix's architecture reflects a careful balance of concerns:

- **Simplicity for users**: High-level API hides complexity
- **Flexibility**: Multiple recovery mechanisms and redundancy policies
- **Performance**: Localized recovery and minimal failure-free overhead
- **Correctness**: Careful use of ULFM primitives to avoid deadlock

The spare rank model, separation of concerns, and pluggable policies create a
foundation that supports a wide range of MPI applications while keeping the door
open for future enhancements like new redundancy strategies or integration with
external checkpoint systems.

Understanding this architecture enables informed decisions about integrating Fenix,
tuning its configuration for specific applications, and potentially contributing
new capabilities to the library.

----

See Also
--------

- :doc:`process-recovery` - Process recovery mechanisms in detail
- :doc:`data-recovery` - Data recovery concepts
- :doc:`imr-policy` - In-Memory Redundancy policy
- :doc:`/howto/migrate-existing-app` - Migrating existing MPI applications
- :doc:`/tutorials/index` - Step-by-step tutorials
- :doc:`/api/index` - Complete API reference
