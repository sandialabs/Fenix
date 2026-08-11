commit_barrier
==============

.. operation:: collective

Commit stored data with globally consistent synchronization.

This is the fault-tolerant version of :c:func:`Fenix_Data_commit`. It uses
:c:func:`MPIX_Comm_agree` to ensure all non-failed ranks reach the commit point
before proceeding. This is **critical for detecting failures** that occur between
store operations and commit.

**When to use commit_barrier instead of commit:**

- Use commit_barrier when you need to guarantee that all ranks successfully stored
  their data before committing the snapshot
- If a rank fails after your last MPI collective but before commit, only
  commit_barrier will detect it
- Regular commit may proceed with incomplete data if failures occur during the
  checkpoint workflow

.. c:function:: int Fenix_Data_commit_barrier(int group_id, int* time_stamp)

   :param int group_id: [in] The data group to commit with failure detection. Must be a valid group created with :c:func:`Fenix_Data_group_create`.
   :param int* time_stamp: [out] The timestamp assigned to this checkpoint version. Use for later restore operations. Pass FENIX_TIME_STAMP_IGNORE if not needed.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::commit_barrier(int group_id, int* time_stamp = nullptr)

   :param int group_id: [in] The data group to commit with failure detection
   :param int* time_stamp: [out] The timestamp assigned to this checkpoint. Default: nullptr (ignore timestamp).
   :returns: FENIX_SUCCESS if successful

.. note::
   This function does not function as a traditional barrier. The commit will proceed if all
   non-failed ranks reach the barrier. This allows for commits to be made when a rank fails
   after storing all of its data into resilient storage.

.. seealso::
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_checkpoint`, :c:func:`Fenix_Data_member_store`
