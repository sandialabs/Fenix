member_stage_inplace
====================

.. operation:: local

Stage a member by taking ownership of a buffer to avoid a copy.

.. c:function:: int Fenix_Data_member_stage_inplace(int group_id, int member_id, void* buf, const Fenix_Data_subset subset)

   :param int group_id: [in] The data group containing the member. Must be a valid group ID.
   :param int member_id: [in] The member to stage data for.
   :param void* buf: [in] Data buffer containing elements to stage. Fenix takes ownership of this buffer and may overwrite, reallocate, or free it at any time (including during this call). The pointer may become invalid after this function returns.
   :param Fenix_Data_subset subset: [in] Which element ranges to stage from buf. Use FENIX_DATA_SUBSET_FULL to stage all elements. Buffer must contain all elements from 0 to max(member's count, subset's end) even if subset is non-contiguous.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::data::member_stage_inplace(int group_id, int member_id, void* buf, const DataSubset& subset = SUBSET_FULL)

   :param int group_id: [in] Group of the member to stage to
   :param int member_id: [in] Member to stage to
   :param void* buf: [in] Buffer Fenix takes ownership of (may become invalid)
   :param DataSubset subset: [in] Element ranges to stage. Default: SUBSET_FULL.
   :returns: FENIX_SUCCESS if successful

.. note::
   As :c:func:`Fenix_Data_member_stage`, but takes ownership of buf to possibly avoid a copy.
   Fenix takes this buf as the new location to stage all data to. Any prior staged but uncommitted
   data is lost. Even if subset is not contiguous or begins after 0, buf must contain all elements
   from 0 to the maximum of (member's count, subset's end). Elements not belonging to the subset
   may be written to with subsequent calls to :c:func:`Fenix_Data_member_stage`.

.. warning::
   There is no guarantee that the pointer to buf will remain valid after this call. Fenix may
   overwrite, reallocate, or free this buffer at any time, including before returning from this function.

.. seealso::
   :c:func:`Fenix_Data_member_stage`, :c:func:`Fenix_Data_member_store`
