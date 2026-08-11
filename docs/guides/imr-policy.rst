In Memory Redundancy Policy (IMR)
==================================

IMR is referenced with the FENIX_DATA_POLICY_IN_MEMORY_RAID definition,
and takes as input an array of integers with the following usage:

* **Mode**: (1 or 5) Chooses storage mimicking the given RAID style. Mode 1 is like
  RAID-1 (mirroring), Mode 5 is like RAID-5 (parity encoding).
* **Separation**: Sets the rank separation for redundancy groups. This is the number
  of ranks between partner ranks. For example, separation=8 means rank 0 partners with
  rank 8, rank 1 with rank 9, etc. Choose a separation that ensures partner ranks are
  not on the same physical node or rack (to minimize correlated failures). If you have
  4 ranks per node, use separation=4 or higher.
* **GroupSize**: For Mode 5 only, sets the size of the parity groups (minimum 3,
  recommended 5-7). Larger groups save memory but increase recovery computation time.

The IMR policy is designed to localize recovery as much as possible, avoiding
global synchronization. Communication amongst group members is required during
recovery (to ensure all members agree on which ranks have recovered data, even
if another failure occurs during recovery). However, groups without recovering
ranks may complete their recovery operations locally without communicating further.
Groups operate independently and need not wait for ranks outside their group to
enter or exit recovery, enabling parallel recovery across groups.

* **Mode 1** (RAID-1 style mirroring): Groups ranks into pairs of partners.
  Rank N is paired with Rank (N+Separation). For example, with separation=4:
  rank 0↔4, rank 1↔5, rank 2↔6, rank 3↔7. For odd-size communicators, a single
  group of size 3 will also form using the first, middle, and last ranks.
  Each rank stores a copy of its own data and a complete copy of its partner's data.
  For groups of three, partner data storage is chained (0→1→2→0).
  Should both partners fail (or any two in groups of three) before recovery
  operations have completed, data will be unrecoverable.

  **Memory Usage**: Each rank stores a copy of its own data and of its
  partner's data for each timestamp, where checkpoint depth D
  stores D+1 checkpoints. Therefore for data size M,
  (D+1)*M*2 bytes are used.

  **Computation**: None.

* **Mode 5** (RAID-5 style parity): Groups ranks into parity groups of size GroupSize.
  Groups are formed by striding: Rank N, N+Separation, N+2*Separation, etc.
  For example, with separation=4 and GroupSize=5, group 0 contains ranks {0,4,8,12,16}.
  Each rank stores its own data plus a proportional share of parity information
  computed from the group's data (like RAID-5 disk striping).
  If any two or more ranks in the same group fail before recovery operations
  have completed, data will be unrecoverable (single-fault tolerant per group).

  **Memory Usage**: Each rank stores a copy of its own data and
  M/(GroupSize-1) parity bytes per timestamp. For example, GroupSize=5 means
  M/4 parity bytes. Therefore, (D+1)*M*(GroupSize/(GroupSize-1)) bytes are used.
  Example: GroupSize=5 means 1.25x memory overhead vs. Mode 1's 2.0x.

  **Computation**: O(M) XOR parity calculations during checkpoint and recovery.

These options enable users to trade reliability and computation for memory
space, which may be necessary for applications with large memory usage.

Implementation Notes
--------------------

.. warning::
   **Mode 5 Limitations:**

   Mode 5 (Parity) does **not support** :c:func:`Fenix_Data_member_storev`, which allows
   rank-varying subsets during checkpoint. If you need to checkpoint different portions
   of data on each rank, you must use Mode 1 (Buddy) or use :c:func:`Fenix_Data_member_store`
   with uniform subsets across all ranks.

   **Asynchronous Operations:**

   The asynchronous checkpoint functions (:c:func:`Fenix_Data_member_istore` and
   :c:func:`Fenix_Data_member_istorev`) are currently **unimplemented for both modes**.
   Use the synchronous alternatives :c:func:`Fenix_Data_member_store` and
   :c:func:`Fenix_Data_member_storev` (Mode 1 only) instead.

----

See Also
--------

- :doc:`data-recovery` - Overall data recovery concepts
- :doc:`../howto/checkpoint-data` - Checkpointing application data
- :doc:`../howto/partial-checkpoints` - Using data subsets
- :doc:`../api/data-recovery/group-management` - Data group API
- :doc:`architecture` - Overall Fenix architecture
