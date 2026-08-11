checkpointv
===========

.. operation:: collective

Convenience function to checkpoint all members with varying subsets across ranks.

This C++ convenience wrapper is equivalent to calling :cpp:func:`fenix::data::checkpoint`
with all members marked for storev. It stores all data group members (each potentially
with different element ranges per rank), then commits atomically.

.. cpp:function:: int fenix::data::checkpointv(int group_id, const DataSubset& subset, int* time_stamp = nullptr)

   :param int group_id: [in] The data group to checkpoint. Must be a valid group created with :cpp:func:`fenix::data::group_create`.
   :param DataSubset subset: [in] Which element ranges of each member to checkpoint. Each rank may specify different ranges. Use ``FENIX_DATA_SUBSET_FULL`` to checkpoint all elements on all ranks.
   :param int* time_stamp: [out] The timestamp assigned to this checkpoint version. Use for later restore operations. Default: nullptr (ignore timestamp).
   :returns: FENIX_SUCCESS if successful, error code otherwise

**Purpose:**

``checkpointv`` is a convenience wrapper that simplifies checkpointing when all members
in a group need to support **rank-varying subsets** (different element ranges on different
ranks). It internally calls :cpp:func:`fenix::data::member_storev` for each member, then
:cpp:func:`fenix::data::commit` to make the checkpoint durable.

**When to Use:**

Use ``checkpointv`` when:

- All ranks in your application checkpoint **different element ranges** of the same data members
- You want domain decomposition where each rank owns different portions of a global array
- You're implementing overlapping ghost zones where ranks checkpoint their local portions
- You need collective verification that all ranks' subsets are consistent

Use :cpp:func:`fenix::data::checkpoint` instead when:

- All ranks checkpoint the **same element ranges** (more efficient - uses ``store`` instead of ``storev``)
- You need fine control over which members use ``store`` vs ``storev``
- You want to checkpoint members selectively

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Checkpoint completed successfully
- :c:enumerator:`FENIX_ERROR_INVALID_GROUPID` - Group does not exist
- :c:enumerator:`FENIX_ERROR_MEMBER_STAGING` - Failed to stage one or more members due to unclosed staging operation
- :c:enumerator:`FENIX_ERROR_COMMIT_BARRIER` - Failed during commit (likely due to rank failure)

**How It Works:**

Internally, ``checkpointv`` performs:

1. Retrieves the list of all members in the group (in creation order)
2. For each member, calls :cpp:func:`fenix::data::member_storev` with the provided subset

   - Each rank serializes its specified element range
   - Ranks exchange subset descriptors to verify consistency
   - Data is stored into resilient in-memory storage

3. Calls :cpp:func:`fenix::data::commit` to atomically finalize the checkpoint
4. Returns the new timestamp if requested

**Usage Example:**

.. code-block:: cpp

   // C++ example - Domain decomposition checkpoint
   #include <fenix.hpp>
   #include <vector>

   int main(int argc, char** argv) {
       fenix::init({.argc = &argc, .argv = &argv});

       int group_id = 1;
       int member_id = 100;

       // Create data group
       fenix::data::group_create(group_id);

       // Each rank has a local array
       std::vector<double> local_data(1000);

       // Register member
       fenix::data::member_create(
           group_id, member_id,
           local_data.data(), local_data.size(),
           MPI_DOUBLE
       );

       // Perform computation
       for (size_t i = 0; i < local_data.size(); i++) {
           local_data[i] = /* compute value */;
       }

       // Checkpoint with rank-varying subsets
       // Each rank checkpoints its entire local portion
       int timestamp;
       int ret = fenix::data::checkpointv(
           group_id,
           fenix::data::SUBSET_FULL,  // Each rank: all local elements
           &timestamp
       );

       if (ret == FENIX_SUCCESS) {
           std::cout << "Checkpoint " << timestamp << " created\n";
       }

       return 0;
   }

.. code-block:: cpp

   // C++ example - Overlapping ghost zones
   #include <fenix.hpp>

   void checkpoint_with_ghosts() {
       int group_id = 1;
       int member_id = 200;

       int rank, size;
       MPI_Comm_rank(MPI_COMM_WORLD, &rank);
       MPI_Comm_size(MPI_COMM_WORLD, &size);

       // Each rank owns elements based on decomposition
       int local_start = rank * 100;
       int local_count = 100 + 10;  // Including ghost zone

       std::vector<double> data(local_count);

       fenix::data::member_create(
           group_id, member_id,
           data.data(), data.size(),
           MPI_DOUBLE
       );

       // Define subset: each rank checkpoints different ranges
       fenix::DataSubset my_subset;
       my_subset.add_range(local_start, local_count);

       // Checkpoint with varying subsets
       fenix::data::checkpointv(group_id, my_subset);

       // Note: Each rank's subset is different!
       // Rank 0: [0, 110)
       // Rank 1: [100, 210)
       // Rank 2: [200, 310)
       // etc.
   }

**Comparison to checkpoint:**

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - checkpoint (mixed store/storev)
     - checkpointv (all storev)
   * - .. code-block:: cpp

          // Fine control over which use storev
          fenix::data::checkpoint(
              group_id,
              subset,
              {member_3, member_5},  // Only these use storev
              &timestamp
          );
     - .. code-block:: cpp

          // All members use storev
          fenix::data::checkpointv(
              group_id,
              subset,
              &timestamp
          );

**Relationship to C API:**

There is no direct C API equivalent. To achieve the same behavior in C, use:

.. code-block:: c

   // C equivalent of checkpointv
   int ret = Fenix_Data_checkpoint(
       group_id,
       subset,
       FENIX_STOREV_ALL,  // Special flag: all members use storev
       NULL,              // No array needed with STOREV_ALL
       &timestamp
   );

**IMR Policy Restrictions:**

.. warning::
   **IMR Mode 5 (Parity) does NOT support storev operations.** If your group uses
   ``FENIX_DATA_POLICY_IN_MEMORY_RAID_MODE5``, calling ``checkpointv`` will result
   in a fatal error.

   **Reason:** Parity-based redundancy requires all ranks to store the same element
   ranges to maintain XOR chunk alignment. Variable subsets break this alignment.

   **Workaround:** Use :cpp:func:`fenix::data::checkpoint` with uniform subsets
   (default ``store`` behavior) or switch to IMR Mode 1 (Buddy) which fully supports
   storev operations.

**Performance Considerations:**

- ``storev`` adds communication overhead for subset descriptor exchange
- Use :cpp:func:`fenix::data::checkpoint` with empty ``storev_ids`` when all ranks
  checkpoint identical ranges (more efficient)
- ``checkpointv`` is ideal for domain-decomposed applications where rank-varying
  subsets are necessary
- Member storage order follows creation order, not member ID order

**Common Pitfalls:**

- **Using with Mode 5 parity**: Will fail at runtime. Use Mode 1 (Buddy) or uniform subsets instead.
- **Empty subsets**: Some ranks using empty subsets may cause issues depending on policy. Test thoroughly.
- **Assuming member order**: Members are stored in the order they were created, which may differ from member IDs.
- **Ignoring timestamp**: Save the returned timestamp for later restoration with :cpp:func:`fenix::data::member_load`.
- **Subset inconsistency**: While storev allows different subsets per rank, the subsets must still be "compatible" for the chosen redundancy policy.

**Message Logging Support:**

This function supports inline recovery when message logging is active. If a failure
occurs during checkpoint, logged MPI operations can be replayed automatically depending
on the :cpp:func:`fenix::set_option` setting for :cpp:enumerator:`MLOG_RECOVERY_MODE`.

.. seealso::
   :cpp:func:`fenix::data::checkpoint`,
   :cpp:func:`fenix::data::member_storev`,
   :cpp:func:`fenix::data::member_store`,
   :cpp:func:`fenix::data::commit`,
   :cpp:func:`fenix::data::commit_barrier`
