commit
======

.. operation:: local "collective semantics"

Finalize a snapshot of all stored data of all members of a group.

This function assigns a timestamp to the stored data and saves it outside of the
snapshot staging area. Future stores will not impact the snapshot's data.

.. c:function:: int Fenix_Data_commit(int group_id, int* time_stamp)

.. cpp:function:: int fenix::data::commit(int group_id, int* time_stamp)

   :param int group_id: The data group to commit
   :param int* time_stamp: The timestamp assigned to this checkpoint version. Use this timestamp for later restore operations.
   :returns: FENIX_SUCCESS if successful, error code otherwise

**Usage Examples:**

.. code-block:: cpp

   // C++ example with error handling
   int group_id = 1;
   int time_stamp;

   try {
       // Store data
       fenix::data::member_store(group_id, member_1, FENIX_DATA_SUBSET_FULL);
       fenix::data::member_store(group_id, member_2, FENIX_DATA_SUBSET_FULL);
   } catch (const fenix::CommException& e) {
       std::cerr << "Checkpoint failed: " << e.what() << "\n";
       return;
   }

   // Commit does not communicate, no need to try/catch
   fenix::data::commit(group_id, &time_stamp);
   std::cout << "Checkpoint " << time_stamp << " created\n";

**Checkpoint Versioning:**

Commits create versioned checkpoints identified by timestamps. The group's ``depth``
parameter (from :c:func:`Fenix_Data_group_create`) determines how many versions are kept:

.. code-block:: c

   // Create group with depth=2 (keep 3 versions)
   Fenix_Data_group_create(group_id, comm, 0, 2, policy, policy_val, &flag);

   // Create checkpoints
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts1);  // ts1 = 0

   // ... work ...

   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts2);  // ts2 = 1

   // ... work ...

   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts3);  // ts3 = 2

   // ... work ...

   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &ts4);  // ts4 = 3, ts1 is deleted

   // Can restore from ts2, ts3, or ts4, but not ts1 (too old)

**Alternative: Combined Checkpoint:**

For simple cases where you want to store all members and commit in one step, use :c:func:`Fenix_Data_checkpoint`:

.. code-block:: c

   // Instead of:
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &time_stamp);

   // You can use:
   Fenix_Data_checkpoint(group_id, FENIX_DATA_SUBSET_FULL, 0, NULL, &time_stamp);

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_checkpoint`, :c:func:`Fenix_Data_commit_barrier`, :c:func:`Fenix_Data_group_create`, :doc:`/guides/data-recovery`
