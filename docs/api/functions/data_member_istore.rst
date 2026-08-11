member_istore
=============

.. operation:: collective

Non-blocking version of member_store.

This function stages data asynchronously. All ranks must checkpoint the **same element ranges** (uniform subset). For example, if all ranks checkpoint elements 0-99, use this function. If different ranks need to checkpoint different element ranges, use :c:func:`Fenix_Data_member_istorev` instead.

.. c:function:: int Fenix_Data_member_istore(int group_id, int member_id, const Fenix_Data_subset subset_specifier, Fenix_Request* request)

   :param int group_id: [in] All ranks must provide the same group_id
   :param int member_id: [in] All ranks must provide the same member_id
   :param Fenix_Data_subset subset_specifier: [in] Which element ranges to checkpoint (must be the same on all ranks)
   :param Fenix_Request* request: [out] Request handle for completion checking
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_istore(int group_id, int member_id, const DataSubset& subset, Fenix_Request* request)

   :param int group_id: [in] The group containing the member
   :param int member_id: [in] The member to store
   :param DataSubset subset: [in] Which element ranges to checkpoint (must be the same on all ranks)
   :param Fenix_Request* request: [out] Request handle
   :returns: FENIX_SUCCESS if successful

.. important::
   **Subset uniformity requirement:** All ranks must checkpoint the **same element ranges**.

   - If all ranks checkpoint elements [0-99], use this function
   - If rank 0 checkpoints [0-50] and rank 1 checkpoints [100-200], use :c:func:`Fenix_Data_member_istorev` instead

.. note::
   **Implementation status:**

   This function is **unimplemented for all IMR modes**. Calling this function
   will print a fatal error message and the behavior is undefined.

   - **IMR Mode 1 (Buddy)**: Not implemented
   - **IMR Mode 5 (Parity)**: Not implemented

   For production use, use the synchronous :c:func:`Fenix_Data_member_store`
   followed by :c:func:`Fenix_Data_commit` instead.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_member_istorev`, :c:func:`Fenix_Data_wait`
