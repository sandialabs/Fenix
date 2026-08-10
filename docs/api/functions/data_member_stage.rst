member_stage
============

.. operation:: local

Serialize a group member's data into the member's local store.

.. c:function:: int Fenix_Data_member_stage(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param Fenix_Data_subset subset_specifier: Which subset of the data to stage
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_stage(int group_id, int member_id, const DataSubset& subset = SUBSET_FULL)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param DataSubset subset: Which subset to stage
   :returns: FENIX_SUCCESS if successful

.. note::
   A store operation can be broken into two parts: locally staging the data within Fenix, then
   policy-specific operations to make the data resilient to faults. This function performs ONLY
   the first part. Applications should subsequently make a store of this member to the
   FENIX_DATA_SUBSET_PRESTAGED data subset.

.. warning::
   FENIX_DATA_SUBSET_ALL is invalid if member size is FENIX_RESIZEABLE. FENIX_DATA_SUBSET_PRESTAGED
   is invalid. It is undefined behaviour to commit staged-but-not-stored data.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_stage_inplace`
