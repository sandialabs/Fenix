member_stage_inplace
====================

.. operation:: local

Stage a member by taking ownership of a buffer to avoid a copy.

.. c:function:: int Fenix_Data_member_stage_inplace(int group_id, int member_id, void* buf, const Fenix_Data_subset subset)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param void* buf: The data buffer which Fenix will take ownership of
   :param Fenix_Data_subset subset: Which subset of the data to stage
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_stage_inplace(int group_id, int member_id, void* buf, const DataSubset& subset = SUBSET_FULL)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param void* buf: Buffer Fenix takes ownership of
   :param DataSubset subset: Which subset to stage
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
