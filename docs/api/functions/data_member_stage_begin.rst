member_stage_begin
==================

.. operation:: local

Open a file for manually staging a member into.

.. c:function:: int Fenix_Data_member_stage_begin(int group_id, int member_id, FILE** fpp)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param FILE** fpp: Output location for the file pointer to be written to
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_stage_begin(int group_id, int member_id, FILE** fp)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param FILE** fp: Output file pointer
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_stage_begin(int group_id, int member_id, std::iostream** stream)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param std::iostream** stream: [out] Output stream pointer
   :returns: FENIX_SUCCESS if successful

.. note::
   It is an error to call any staging, storing, loading, or restoring function involving this
   member before a corresponding call to :c:func:`Fenix_Data_member_stage_end`. File must not
   be closed by the user. It is an error to use this file after the corresponding
   :c:func:`Fenix_Data_member_stage_end`.

.. seealso::
   :c:func:`Fenix_Data_member_stage_end`, :c:func:`Fenix_Data_member_stage`
