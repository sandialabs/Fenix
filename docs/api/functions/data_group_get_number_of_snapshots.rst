group_get_number_of_snapshots
=============================

.. operation:: local

Get the number of locally-available snapshots in a data group.

.. c:function:: int Fenix_Data_group_get_number_of_snapshots(int group_id, int* number_of_snapshots)

   :param int group_id: [in] The data group to query. Must be a valid existing group.
   :param int* number_of_snapshots: [out] Pointer to store the count of locally-available snapshots for this group. May vary per rank if failures occurred during commit.
   :returns: FENIX_SUCCESS if successful, error code if group invalid

.. cpp:function:: std::optional<std::vector<int>> fenix::data::group_snapshots(int group_id)

   Query all locally-available snapshot timestamps in a data group.

   :param int group_id: [in] The data group to query
   :returns: std::optional containing vector of snapshot timestamps if group exists, std::nullopt otherwise

   This C++ convenience function retrieves all snapshot timestamps for a given data group in a single call,
   returning them as a vector. It provides a modern alternative to iterating through snapshots using the C
   API functions :c:func:`Fenix_Data_group_get_number_of_snapshots` and
   :c:func:`Fenix_Data_group_get_snapshot_at_position`.

   **Behavior:**

   * Returns a vector of integer timestamp identifiers for all snapshots in the group
   * Timestamps are returned in chronological order (oldest first)
   * If the group does not exist, returns std::nullopt (empty optional)
   * This is a local operation - no MPI communication
   * The snapshot list may vary across ranks if failures occurred during commit

   **When to use:**

   * When you need to examine all available snapshots for selective loading
   * When implementing custom snapshot management or pruning logic
   * When recovering from failures and need to identify which snapshots are available

   **Usage Example:**

   .. code-block:: cpp

      #include <fenix.hpp>
      #include <iostream>
      #include <vector>

      using namespace fenix::data;

      // Create a group and take multiple checkpoints
      group_create(0, {.depth = 3});
      member_create(0, 0, data.data(), count, MPI_INT);

      checkpoint(0, SUBSET_FULL);  // Creates snapshot with timestamp 0
      // ... modify data ...
      checkpoint(0, SUBSET_FULL);  // Creates snapshot with timestamp 1
      // ... modify data ...
      checkpoint(0, SUBSET_FULL);  // Creates snapshot with timestamp 2

      // Query all available snapshots
      auto snapshots_opt = group_snapshots(0);

      if (snapshots_opt.has_value()) {
          std::vector<int> timestamps = *snapshots_opt;
          std::cout << "Found " << timestamps.size() << " snapshots:\n";

          // Iterate through all available snapshots
          for (int ts : timestamps) {
              std::cout << "  Snapshot timestamp: " << ts << "\n";
          }

          // Load a specific older snapshot
          if (timestamps.size() >= 2) {
              int older_snapshot = timestamps[0];  // Oldest snapshot
              member_load(0, 0, older_snapshot);
          }
      } else {
          std::cerr << "Group does not exist\n";
      }

   **Common Pitfalls:**

   * **Not checking optional return value:** Always verify ``has_value()`` before dereferencing the optional.
     Accessing a non-existent group's snapshots will return std::nullopt, and dereferencing will cause undefined behavior.

   * **Assuming consistent snapshots across ranks:** After failures, different ranks may have different sets of
     available snapshots. The returned list is local to the calling rank. Use collective operations if you need
     to determine which snapshots are available on all ranks.

   * **Confusing with snapshot positions:** The C API uses 0-based positions where position 0 is the most recent
     snapshot. This function returns timestamps in chronological order (oldest first), which is the opposite ordering.

   * **Modifying while iterating:** Do not delete snapshots while iterating through the returned vector, as this
     modifies the internal group state. Copy the vector first if you plan to delete snapshots.

.. note::
   May include snapshots that are inconsistent across the group. After a failure, some ranks may have successfully
   committed a snapshot while others did not. Use this information to determine which data can be safely restored.

.. seealso::
   :c:func:`Fenix_Data_group_get_snapshot_at_position`, :c:func:`Fenix_Data_group_get_number_of_snapshots`,
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_load`, :c:func:`Fenix_Data_snapshot_delete`,
   :cpp:func:`fenix::data::group_members`
