member_storev
=============

.. operation:: collective

Store a member with varying subsets across ranks.

Unlike :c:func:`Fenix_Data_member_store`, which requires all ranks to use the **same subset** (e.g., all ranks checkpoint elements 0-99), this function allows each rank to checkpoint **different element ranges**. For example, rank 0 might checkpoint elements 0-50 while rank 1 checkpoints elements 100-200.

.. c:function:: int Fenix_Data_member_storev(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: All ranks must provide the same group_id
   :param int member_id: All ranks must provide the same member_id
   :param Fenix_Data_subset subset_specifier: Which portion of this rank's data to checkpoint. Each rank may specify different element ranges.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_storev(int group_id, int member_id, const DataSubset& subset)

   :param int group_id: The group containing the member
   :param int member_id: The member to store
   :param DataSubset subset: Which element ranges to checkpoint (may differ per rank)
   :returns: FENIX_SUCCESS if successful

**Key Difference from member_store:**

- **member_store**: All ranks must use the **same subset** (uniform)
- **member_storev**: Each rank can use a **different subset** (varying)

.. warning::
   **Implementation status:**

   - **IMR Mode 1 (Buddy)**: Fully implemented and production-ready
   - **IMR Mode 5 (Parity)**: **Not supported** - will print fatal error "IMR mode 5 cannot storev"

   If you need rank-varying subsets with Mode 5, you must switch to Mode 1 or use
   :c:func:`Fenix_Data_member_store` with uniform subsets across all ranks.

   The asynchronous version :c:func:`Fenix_Data_member_istorev` is unimplemented for all modes.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_istorev`
