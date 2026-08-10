member_load_end
===============

.. operation:: local

Conclude a member_load_begin operation.

.. c:function:: int Fenix_Data_member_load_end(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_load_end(int group_id, int member_id)

   :param int group_id: Group of the member to end loading for
   :param int member_id: Member to end loading for
   :returns: FENIX_SUCCESS if successful

.. note::
   Throws (or returns) FENIX_ERROR_INVALID_LOGIC_CALL if there has not been a corresponding
   :c:func:`Fenix_Data_member_load_begin`.

.. seealso::
   :c:func:`Fenix_Data_member_load_begin`
