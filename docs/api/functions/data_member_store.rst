member_store
============

.. operation:: collective

Store a data member to the checkpoint.

.. c:function:: int Fenix_Data_member_store(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: The group containing the member
   :param int member_id: The member to store
   :param Fenix_Data_subset subset_specifier: Which subset of the data to store (use FENIX_DATA_SUBSET_ALL for all data)
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_store(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

   :param int group_id: The group containing the member
   :param int member_id: The member to store
   :param Fenix_Data_subset subset_specifier: Which subset of the data to store
   :returns: FENIX_SUCCESS if successful

.. seealso::
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_restore`
