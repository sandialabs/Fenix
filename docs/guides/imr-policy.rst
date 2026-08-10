In Memory Redundancy Policy (IMR)
==================================

IMR is referenced with the FENIX_DATA_POLICY_IN_MEMORY_RAID definition,
and takes as input an array of integers with the following usage:

* **Mode**: (1 or 5) Chooses storage mimicking the given RAID style.
* **Separation**: Sets the rank separation for groups used to store redundant data.
  Users should choose a separation that attempts to ensure the ranks
  chosen for grouping are not colocated on nodes/racks to minimize the
  chance of multiple ranks in a group
* **GroupSize**: For Mode 5 only, sets the size of the parity groups, minimum 3.

The policy is designed to localize recovery as much as possible. Communication
amongst group members is required (as failure during recovery operations
can lead to inconsistent beliefs about which ranks have recovered data),
but groups without recovering ranks may then all recover locally rather
than communicating further. Groups need not wait for ranks outside of
their group to enter or exit recovery.

* **Mode 1**: Groups ranks into dyadically paired partners of Rank N and
  Rank (N+Separation). For odd-size communicators, a single
  group of size 3 will also form of the first, middle, and last
  ranks. Each rank stores a copy of its own data and a copy of
  its partner's. For groups of three, partner data storage is
  chained. Should both partners fail (or any two for groups of
  three) before recovery operations have completed, data will be
  unrecoverable.

  **Memory Usage**: Each rank stores a copy of its own data and of its
  partner's data for each timestamp, where checkpoint depth D
  stores D+1 checkpoints. Therefore for data size M,
  (D+1)*M*2 bytes are used.

  **Computation**: None.

* **Mode 5**: Groups ranks into parity groups of size GroupSize.
  Groups are formed of Rank N, N+Separation, N+2*Separation.
  If any two ranks in a group fail before recovery operations
  have completed, data will be unrecoverable.

  **Memory Usage**: Each rank stores a copy of its own data and
  M/(GroupSize-1) parity bytes per timestamp. Therefore,
  (D+1)*M*(GroupSize/(GroupSize-1)) bytes are used.

  **Computation**: O(M) parity bit calculations.

These options enable users to trade reliability and computation for memory
space, which may be necessary for applications with large memory usage.
