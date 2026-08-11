Glossary
========

This glossary defines Fenix-specific terminology. Terms are organized alphabetically for quick reference.

.. glossary::

   Active Rank
      A rank that is currently participating in the application's computation, as opposed to a :term:`spare rank`. Active ranks include :term:`initial ranks <Initial Rank>`, :term:`survivor ranks <Survivor Rank>`, and :term:`recovered ranks <Recovered Rank>`.

      See also: :doc:`guides/process-recovery`

   Checkpoint
      The process of storing application state to resilient storage for later recovery. In Fenix, checkpointing involves creating a :term:`snapshot` by storing :term:`data members <Data Member>` and committing them to a :term:`data group`.

      A checkpoint consists of three steps:

      1. :term:`Stage` data members (optional, for performance)
      2. :term:`Store` data members to resilient storage
      3. :term:`Commit` to create an immutable snapshot

      Example:

      .. code-block:: cpp

         fenix::data::member_store(GROUP_ID, MEMBER_ID);
         fenix::data::commit(GROUP_ID);

      See also: :doc:`howto/checkpoint-data`

   Commit
      The operation that makes stored data members permanent by creating a :term:`snapshot`. Only committed data can be recovered after a failure. A commit is atomic - either all stored data in the group is committed together, or none is.

      Functions: :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_commit_barrier`, :c:func:`Fenix_Data_checkpoint`

      See also: :term:`Data Group`, :term:`Snapshot`

   Data Group
      A container for related :term:`data members <Data Member>` that are committed together as a transaction. Each group has:

      - A unique integer identifier (group_id)
      - A resilient communicator defining participating ranks
      - A redundancy policy (e.g., :term:`IMR Policy`)
      - A depth specifying how many :term:`snapshots <Snapshot>` to retain

      Multiple data groups can exist simultaneously with different policies.

      Functions: :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_group_delete`

      See also: :doc:`guides/data-recovery`

   Data Member
      An individual piece of application data registered with Fenix for checkpoint/restart. Members are identified by an integer member_id within their :term:`data group` and specify:

      - A memory buffer pointer
      - Element count (or :term:`FENIX_RESIZEABLE` for variable-size data)
      - MPI datatype for the elements
      - Optional serialization function for complex data structures

      Functions: :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Data_member_fcreate`

      See also: :term:`Data Group`, :term:`Subset`

   Depth
      The number of historical :term:`snapshots <Snapshot>` retained by a :term:`data group`, in addition to the most recent one. For example:

      - Depth 0: Keep only the latest snapshot (oldest are automatically deleted)
      - Depth 1: Keep latest + 1 previous snapshot (2 total)
      - Depth 5: Keep latest + 5 previous snapshots (6 total)

      Larger depth requires more memory but provides more recovery options.

      See also: :term:`Data Group`, :term:`Time Stamp`

   FENIX_RESIZEABLE
      A special count value used when creating a :term:`data member` whose size may change between checkpoints. Resizable members allow dynamic data structures (like vectors) to be checkpointed without knowing their size in advance.

      Important: After resizing, you must update the buffer pointer using :c:func:`Fenix_Data_member_attr_set`.

      Example:

      .. code-block:: cpp

         std::vector<double> data;
         fenix::data::member_create(GROUP_ID, MEMBER_ID,
                                    data.data(), FENIX_RESIZEABLE, MPI_DOUBLE);

         // After resizing
         data.resize(new_size);
         int flag;
         Fenix_Data_member_attr_set(GROUP_ID, MEMBER_ID,
                                    FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                                    data.data(), &flag);

      See also: :term:`Data Member`

   IMR Policy
      **In-Memory Redundancy Policy** - Fenix's built-in redundancy policy that stores checkpoint data in the memory of other ranks rather than on disk. Provides RAID-style redundancy:

      - **Mode 1** (RAID-1 style): Mirrors each rank's data to one partner. Memory: 2x per checkpoint.
      - **Mode 5** (RAID-5 style): Distributes data and parity across groups of ranks. Memory: (N/(N-1))x where N is group size.

      Constant: ``FENIX_DATA_POLICY_IN_MEMORY_RAID`` or ``FENIX_DATA_POLICY_IMR``

      See also: :doc:`guides/imr-policy`

   Initial Rank
      A rank that has not yet experienced any failures. All ranks start with this role when they first exit :c:func:`Fenix_Init`. After the first failure, ranks become either :term:`survivor ranks <Survivor Rank>` or :term:`recovered ranks <Recovered Rank>`.

      Role constant: ``FENIX_ROLE_INITIAL_RANK`` (C) or ``fenix::INITIAL_RANK`` (C++)

      See also: :term:`Rank Role`, :c:func:`Fenix_get_role`

   Inline Recovery
      A recovery pattern where control returns to the application at the point of failure (the failing MPI call) rather than jumping back to :c:func:`Fenix_Init`. Enabled by setting :c:macro:`FENIX_RESUME_MODE` to ``FENIX_RESUME_RETURN`` or ``FENIX_RESUME_THROW``.

      Advantages over :term:`longjmp recovery`:

      - No undefined behavior from longjmp
      - Proper C++ destructor invocation
      - More predictable with compiler optimizations
      - Better integration with exception handling

      See also: :doc:`tutorials/03-inline-recovery`, :doc:`howto/choose-recovery-pattern`

   Load
      The local operation of copying data from a :term:`snapshot` into application memory. Unlike :term:`restore`, load is not collective and does not repair the snapshot's resilient storage.

      Functions: :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_member_load_to`

      See also: :term:`Restore`, :term:`Snapshot`

   Longjmp Recovery
      The default recovery pattern where Fenix uses ``longjmp`` to return control to :c:func:`Fenix_Init` after a failure. This mimics traditional checkpoint/restart behavior but has undefined behavior in C++ and with certain compiler optimizations.

      Enabled by setting :c:macro:`FENIX_RESUME_MODE` to ``FENIX_RESUME_JUMP`` (default).

      Warning: Variables modified between ``Fenix_Init`` and the failure point should be declared ``volatile`` to avoid undefined behavior.

      See also: :term:`Inline Recovery`, :doc:`howto/choose-recovery-pattern`

   Message Logging
      Optional Fenix feature that records MPI communication patterns so they can be replayed after recovery, eliminating the need to recompute from the last checkpoint.

      Key concepts:

      - **Log**: A message logger created with :c:func:`Fenix_Mlog_create`
      - **Region**: A logical unit of work (e.g., iteration) marked with :c:func:`Fenix_Mlog_begin_region`
      - **Replay**: Automatic replay of logged messages via :c:func:`Fenix_Mlog_sync`

      Functions: :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_sync`

      See also: :doc:`howto/message-logging`

   RAID Policy
      See :term:`IMR Policy`

   Rank Role
      An enumeration indicating a rank's state relative to fault recovery:

      - :term:`FENIX_ROLE_INITIAL_RANK <Initial Rank>`: No failures yet
      - :term:`FENIX_ROLE_RECOVERED_RANK <Recovered Rank>`: Was a spare, now active
      - :term:`FENIX_ROLE_SURVIVOR_RANK <Survivor Rank>`: Survived a failure
      - :term:`FENIX_ROLE_SPARE_RANK <Spare Rank>`: Currently a spare rank

      Query with: :c:func:`Fenix_get_role` (C) or ``fenix::role()`` (C++)

      Note: The role is only guaranteed to be accurate immediately after recovery from a single failure. Applications that must be resilient to failures during recovery should not rely solely on the role.

      See also: :doc:`guides/process-recovery`

   Recovered Rank
      A :term:`spare rank` that has been activated to replace a failed rank. Recovered ranks:

      - Have no registered callbacks (callbacks are registered before failure)
      - Need to restore application state from checkpoints
      - May have a different rank ID than the failed rank they replace (if spares are depleted)

      Role constant: ``FENIX_ROLE_RECOVERED_RANK`` (C) or ``fenix::RECOVERED_RANK`` (C++)

      Example:

      .. code-block:: cpp

         if (fenix::role() == fenix::RECOVERED_RANK) {
           // I'm a recovered rank - restore state
           fenix::data::member_restore(GROUP_ID, MEMBER_ID);
         }

      See also: :term:`Rank Role`, :term:`Spare Rank`

   Region
      In :term:`message logging`, a logical unit of work (typically an iteration) whose messages are logged together. Regions are identified by monotonically increasing integers and marked with :c:func:`Fenix_Mlog_begin_region`.

      When recovering, ranks can sync to a specific region, replaying all messages from that point forward.

      See also: :term:`Message Logging`, :term:`Window`

   Repair
      The collective operation that rebuilds a :term:`data member`'s resilient storage after a failure. Repair uses the redundancy policy to reconstruct lost data from surviving ranks.

      Function: :c:func:`Fenix_Data_member_repair`

      Example:

      .. code-block:: cpp

         // After recovery, repair then load
         fenix::data::member_repair(GROUP_ID, MEMBER_ID);
         fenix::data::member_load(GROUP_ID, MEMBER_ID);

      See also: :term:`Restore`

   Resilient Communicator
      The MPI communicator returned by :c:func:`Fenix_Init` that supports fault tolerance.
      Unlike ``MPI_COMM_WORLD`` (which aborts the entire application on rank failures),
      this communicator can detect failures and be automatically repaired by Fenix.
      Any communicators derived from the resilient communicator (via ``MPI_Comm_split``,
      ``MPI_Comm_dup``, etc.) also inherit fault tolerance capabilities.

      Important: Use the resilient communicator instead of ``MPI_COMM_WORLD`` for all MPI
      operations in fault-tolerant applications. Operations on ``MPI_COMM_WORLD`` will not
      be protected by Fenix.

      Example:

      .. code-block:: cpp

         MPI_Comm res_comm;
         fenix::init({.out_comm = &res_comm, .spares = 3});

         // Use res_comm for all MPI operations
         MPI_Bcast(buffer, count, MPI_INT, 0, res_comm);

      See also: :doc:`quickstart`

   Restore
      The collective operation that combines :term:`repair` and :term:`load` - it repairs a :term:`data member`'s resilient storage and then loads the data into application memory.

      Function: :c:func:`Fenix_Data_member_restore`

      Restore is collective across the data group's ranks and may be matched remotely by :c:func:`Fenix_Data_member_repair`.

      See also: :term:`Load`, :term:`Repair`

   Snapshot
      An immutable point-in-time image of all :term:`data members <Data Member>` in a
      :term:`data group`, created by a :term:`commit` operation. A snapshot represents
      a consistent checkpoint - all members were stored together atomically. Each
      snapshot is identified by a :term:`time stamp` (user-controlled integer).

      Only committed (snapshot) data can be recovered after failures. Uncommitted data
      (from store operations without a following commit) cannot be recovered. The number
      of retained historical snapshots is controlled by the group's :term:`depth` parameter.

      Functions: :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_group_get_number_of_snapshots`

      See also: :term:`Commit`, :term:`Time Stamp`

   Spare Rank
      A rank reserved by Fenix to replace failed :term:`active ranks <Active Rank>`. Spare ranks:

      - Do not participate in computation until needed
      - Wait inside :c:func:`Fenix_Init` until activated or finalized
      - Become :term:`recovered ranks <Recovered Rank>` when replacing failed ranks
      - Can be configured to busy-wait, yield, or sleep (see :c:macro:`FENIX_SPARE_WAIT_MODE`)

      The number of spare ranks is specified in :c:func:`Fenix_Init`. When :c:func:`Fenix_Finalize` is called, unused spares automatically exit (or can be released for use, see :c:macro:`FENIX_SPARE_FINALIZE_MODE`).

      Role constant: ``FENIX_ROLE_SPARE_RANK`` (C) or ``fenix::SPARE_RANK`` (C++)

      See also: :doc:`guides/process-recovery`

   Stage
      The local operation of copying a :term:`data member`'s data into Fenix's internal buffer before :term:`storing <Store>` it to resilient storage. Staging is optional but can improve performance by decoupling data serialization from network communication.

      Functions: :c:func:`Fenix_Data_member_stage`, :c:func:`Fenix_Data_member_stage_inplace`

      Pattern:

      .. code-block:: cpp

         // Stage locally
         fenix::data::member_stage(GROUP_ID, MEMBER_ID);

         // Later: store the pre-staged data (collective)
         fenix::data::member_store(GROUP_ID, MEMBER_ID,
                                   fenix::data::SUBSET_PRESTAGED);

      See also: :term:`Store`

   Store
      The collective operation that copies a :term:`data member` (or :term:`subset` of it) into the :term:`data group`'s resilient storage. Stored data is not yet recoverable until :term:`committed <Commit>`.

      Multiple stores can be performed before a commit, allowing incremental or partial checkpointing.

      Functions: :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_storev`

      See also: :term:`Stage`, :term:`Commit`, :term:`Checkpoint`

   Subset
      A specification of which elements of a :term:`data member` to checkpoint or restore. Subsets enable partial checkpointing to reduce overhead. Defined by:

      - Number of blocks (contiguous ranges)
      - Start and end offsets for each block
      - Optional stride between blocks

      Special subsets:

      - ``FENIX_DATA_SUBSET_FULL``: All elements
      - ``FENIX_DATA_SUBSET_EMPTY``: No elements
      - ``FENIX_DATA_SUBSET_PRESTAGED``: Previously staged elements

      Functions: :c:func:`Fenix_Data_subset_create`, :c:func:`Fenix_Data_subset_createv`

      See also: :doc:`howto/partial-checkpoints`

   Survivor Rank
      An :term:`active rank` that was active before a failure and continued execution afterward. Survivor ranks:

      - Keep their same rank ID (if spares are available)
      - Have registered callbacks that will execute after recovery
      - May need to repair/restore checkpointed data

      Role constant: ``FENIX_ROLE_SURVIVOR_RANK`` (C) or ``fenix::SURVIVOR_RANK`` (C++)

      See also: :term:`Rank Role`, :term:`Recovered Rank`

   Time Stamp
      An integer identifier associated with each :term:`snapshot` in a :term:`data group`. Time stamps are:

      - User-controlled (you specify the starting value)
      - Automatically incremented with each :term:`commit`
      - Used to identify which snapshot to restore

      Special values:

      - ``FENIX_DATA_SNAPSHOT_LATEST`` (-1): Most recent snapshot
      - ``FENIX_DATA_SNAPSHOT_ALL`` (-2): Load from all available snapshots

      See also: :term:`Snapshot`, :term:`Depth`

   ULFM
      **User Level Failure Mitigation** - An extension to MPI (part of the MPI 4.x standard) that provides low-level primitives for detecting and handling rank failures without aborting the entire application.

      Key ULFM features:

      - Failure detection during MPI operations
      - Communicator revocation to propagate failure knowledge
      - Non-interruptible collectives for recovery coordination
      - Communicator shrinking and repair primitives

      Fenix is built on ULFM and requires **Open MPI 5.0+** with ULFM support (``--with-ft=mpi``).

      See also: :doc:`installation`, `ULFM Documentation <https://docs.open-mpi.org/en/v5.0.x/features/ulfm.html>`_

   Window
      In :term:`message logging`, the number of :term:`regions <Region>` retained in the log at once. Older regions are automatically deleted when the window fills. Specified as the ``depth`` parameter to :c:func:`Fenix_Mlog_create`.

      Trade-off: Larger windows provide more recovery flexibility but consume more memory.

      See also: :term:`Message Logging`, :term:`Region`

----

Cross-Reference
---------------

**By Topic:**

**Process Recovery:**
:term:`Resilient Communicator`,
:term:`Spare Rank`,
:term:`Active Rank`,
:term:`Initial Rank`,
:term:`Survivor Rank`,
:term:`Recovered Rank`,
:term:`Rank Role`,
:term:`ULFM`

**Data Recovery:**
:term:`Checkpoint`,
:term:`Data Group`,
:term:`Data Member`,
:term:`Store`,
:term:`Stage`,
:term:`Commit`,
:term:`Snapshot`,
:term:`Time Stamp`,
:term:`Depth`,
:term:`Subset`,
:term:`Load`,
:term:`Repair`,
:term:`Restore`,
:term:`FENIX_RESIZEABLE`,
:term:`IMR Policy`

**Message Recovery:**
:term:`Message Logging`,
:term:`Region`,
:term:`Window`

**Recovery Patterns:**
:term:`Inline Recovery`,
:term:`Longjmp Recovery`
