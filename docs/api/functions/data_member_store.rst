member_store
============

.. operation:: "cohort collective"

Reliably store data into the local and remote staging areas.

.. c:function:: int Fenix_Data_member_store(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

.. cpp:function:: int fenix::data::member_store(int group_id, int member_id, const DataSubset& subset)

   :param int group_id: The data group containing the member
   :param int member_id: The member to store
   :param const DataSubset& subset: Which subset of the data to store (default: SUBSET_FULL)
   :returns: FENIX_SUCCESS if successful, error code otherwise

This function starts by locally staging the data subset (per
:cpp:func:member_stage), then ranks communicate the staged data to store it
reliably.

If the data subset is SUBSET_PRESTAGED, ranks instead communicate to reliably
store all data currently in the staging area (i.e., from a prior
:cpp:func:member_stage call).

It is undefined behavior if the subset of stored data varies between ranks
(including if the subset of prestaged data varies, when SUBSET_PRESTAGED is
passed).

The stored data is not available for recovery until the group is committed.


.. c:function:: int Fenix_Data_member_storev(int group_id, int member_id, const Fenix_Data_subset subset)

.. cpp:function:: int fenix::data::member_storev(int group_id, int member_id, const DataSubset& subset)

As above, but the subset of stored data can vary between ranks.

.. seealso::
   :c:func:`Fenix_Data_commit`, :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_create`, :doc:`/guides/data-recovery`
