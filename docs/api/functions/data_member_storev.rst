member_storev
=============

.. operation:: collective

Store a member with varying subsets across ranks.

.. c:function:: int Fenix_Data_member_storev(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: All ranks must provide the same group_id
   :param int member_id: All ranks must provide the same member_id
   :param Fenix_Data_subset subset_specifier: Which subset of the data to store (may vary rank-to-rank)
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_storev(int group_id, int member_id, const DataSubset& subset)

   :param int group_id: The group containing the member
   :param int member_id: The member to store
   :param DataSubset subset: Which subset to store
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is currently unimplemented. As :c:func:`Fenix_Data_member_store`, but subsets
   may vary rank-to-rank.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_istorev`
