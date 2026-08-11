member_istorev
==============

.. operation:: collective

Non-blocking version of member_storev.

This function allows each rank to checkpoint **different element ranges** of its local data. For example, rank 0 might checkpoint elements 0-50 while rank 1 checkpoints elements 100-200.

.. c:function:: int Fenix_Data_member_istorev(int group_id, int member_id, const Fenix_Data_subset subset_specifier, Fenix_Request* request)

   :param int group_id: [in] The data group containing the member. All ranks must provide the same value.
   :param int member_id: [in] The member to store. All ranks must provide the same value.
   :param Fenix_Data_subset subset_specifier: [in] Which element ranges to checkpoint. Each rank may specify different ranges (varying subsets per rank).
   :param Fenix_Request* request: [out] Handle for checking completion with :c:func:`Fenix_Data_test` or :c:func:`Fenix_Data_wait`.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_istorev(int group_id, int member_id, const DataSubset& subset, Fenix_Request* request)

   :param int group_id: [in] The group containing the member (all ranks same value)
   :param int member_id: [in] The member to store (all ranks same value)
   :param DataSubset subset: [in] Element ranges to checkpoint (may vary per rank)
   :param Fenix_Request* request: [out] Request handle for completion checking
   :returns: FENIX_SUCCESS if successful

.. note::
   **Implementation status:**

   This function is **unimplemented for all IMR modes**. Calling this function
   will print a fatal error message and the behavior is undefined.

   - **IMR Mode 1 (Buddy)**: Not implemented
   - **IMR Mode 5 (Parity)**: Not implemented

   For production use, use the synchronous :c:func:`Fenix_Data_member_storev`
   followed by :c:func:`Fenix_Data_commit` instead (Mode 1 only).

.. seealso::
   :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_member_istore`, :c:func:`Fenix_Data_wait`
