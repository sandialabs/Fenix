member_stage_end
================

.. operation:: local

Conclude a member_stage_begin operation.

.. c:function:: int Fenix_Data_member_stage_end(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_stage_end(int group_id, int member_id)

   :param int group_id: Group of the member to end staging for
   :param int member_id: Member to end staging for
   :returns: FENIX_SUCCESS if successful

.. note::
   This function is equivalent to performing a :c:func:`Fenix_Data_member_stage_inplace` with
   buf pointing to the written data and a subset of FENIX_DATA_SUBSET_FULL. For resizable members,
   the subset is instead of the range [0, staging_file_size/element_size] and it is an error if
   the staging file's size is not divisible by the element size.

.. note::
   Throws (or returns) FENIX_ERROR_INVALID_LOGIC_CALL if there has not been a corresponding
   :c:func:`Fenix_Data_member_stage_begin`.

.. seealso::
   :c:func:`Fenix_Data_member_stage_begin`, :c:func:`Fenix_Data_member_stage_inplace`
