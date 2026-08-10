member_istorev
==============

.. operation:: collective

Non-blocking version of member_storev.

.. c:function:: int Fenix_Data_member_istorev(int group_id, int member_id, const Fenix_Data_subset subset_specifier, Fenix_Request* request)

   :param int group_id: All ranks must provide the same group_id
   :param int member_id: All ranks must provide the same member_id
   :param Fenix_Data_subset subset_specifier: Which subset of the data to store (may vary rank-to-rank)
   :param Fenix_Request* request: Request handle for completion checking
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_istorev(int group_id, int member_id, const DataSubset& subset, Fenix_Request* request)

   :param int group_id: The group containing the member
   :param int member_id: The member to store
   :param DataSubset subset: Which subset to store
   :param Fenix_Request* request: Request handle
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is currently unimplemented.

.. seealso::
   :c:func:`Fenix_Data_member_storev`, :c:func:`Fenix_Data_member_istore`, :c:func:`Fenix_Data_wait`
