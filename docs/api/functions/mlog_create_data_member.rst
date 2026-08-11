mlog_create_data_member
=======================

.. operation:: local

Create a data member that can be used to stage and restore a message log.

.. c:function:: int Fenix_Mlog_create_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: [in] The message log identifier to associate with this data member. The member can be used to checkpoint and restore this message log's state.
   :param int group_id: [in] The data group to create the member within. Must be a valid existing group.
   :param int member_id: [in] The unique identifier for the new member within the group. Fails if member already exists (non-idempotent).
   :returns: FENIX_SUCCESS if successful, error code if member exists or IDs invalid

.. cpp:function:: int fenix::mlog::create_data_member(int mlog_id, int group_id, int member_id)

   :param int mlog_id: [in] The mlog to link to this data member
   :param int group_id: [in] The data group to create the member within
   :param int member_id: [in] The ID to create the member as
   :returns: FENIX_SUCCESS if successful

**Usage Examples:**

.. code-block:: c

   // C example - Complete message log checkpointing workflow
   MPI_Comm fenix_comm;
   int mlog_id = 1;
   int group_id = 10;
   int member_id = 100;
   int time_stamp;

   // Create message log for the communicator
   int ret = Fenix_Mlog_create(mlog_id, &fenix_comm, 1000);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to create message log: %d\n", ret);
       return ret;
   }

   // Activate logging to start capturing messages
   Fenix_Mlog_activate(mlog_id);

   // Create data group for checkpointing (if not already created)
   int flag;
   int separation = 1;
   Fenix_Data_group_create(
       group_id, fenix_comm, 0, 3,
       FENIX_DATA_POLICY_IN_MEMORY_RAID,
       &separation, &flag
   );

   // Create data member linked to the message log
   ret = Fenix_Mlog_create_data_member(mlog_id, group_id, member_id);
   if (ret != FENIX_SUCCESS) {
       fprintf(stderr, "Failed to create mlog data member: %d\n", ret);
       return ret;
   }

   // Later: checkpoint the message log state along with application data
   Fenix_Data_member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);
   Fenix_Data_commit(group_id, &time_stamp);

   printf("Message log checkpointed at timestamp %d\n", time_stamp);

   // After recovery: restore message log state
   Fenix_Data_member_restore(group_id, member_id, FENIX_DATA_SUBSET_FULL,
                              time_stamp, NULL);

.. code-block:: cpp

   // C++ example with error handling
   MPI_Comm fenix_comm;
   int mlog_id = 1;
   int group_id = 10;
   int member_id = 100;

   try {
       // Create and activate message log
       fenix::mlog::create(mlog_id, &fenix_comm, 1000);
       fenix::mlog::activate(mlog_id);

       // Create data group and member
       int flag, separation = 1;
       fenix::data::group_create(
           group_id, fenix_comm, 0, 3,
           FENIX_DATA_POLICY_IN_MEMORY_RAID,
           &separation, &flag
       );

       fenix::mlog::create_data_member(mlog_id, group_id, member_id);

       // Checkpoint message log state
       fenix::data::member_store(group_id, member_id, FENIX_DATA_SUBSET_FULL);

       int time_stamp;
       fenix::data::commit(group_id, &time_stamp);
       std::cout << "Message log checkpointed at " << time_stamp << "\n";

   } catch (const fenix::Exception& e) {
       std::cerr << "Error: " << e.what() << "\n";
   }

**Notes:**

- Unlike :c:func:`Fenix_Mlog_define_data_member`, this function is **non-idempotent**: it fails if the member already exists.
- The message log must be created with :c:func:`Fenix_Mlog_create` before creating a data member for it.
- The data group must exist before creating members in it.
- After creating the member, use :c:func:`Fenix_Data_member_store` to checkpoint the message log state.
- Message log checkpointing enables recovery to a consistent communication state after failures.

.. seealso::
   :c:func:`Fenix_Mlog_define_data_member`, :c:func:`Fenix_Data_member_create`, :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_restore`
