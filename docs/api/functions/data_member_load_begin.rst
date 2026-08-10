member_load_begin
=================

.. operation:: local

Open a file for reading a member's committed data from.

.. c:function:: int Fenix_data_member_load_begin(int group_id, int member_id, FILE** fpp, int time_stamp, Fenix_Data_subset* found_data)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param FILE** fpp: Output location for the file pointer to be written to
   :param int time_stamp: Time stamp of the snapshot to load (must not be FENIX_DATA_SNAPSHOT_ALL)
   :param Fenix_Data_subset* found_data: Subset of the elements successfully loaded
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_load_begin(int group_id, int member_id, FILE** fp, int time_stamp = FENIX_DATA_SNAPSHOT_LATEST, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param FILE** fp: Output file pointer
   :param int time_stamp: Time stamp of the snapshot to load
   :param DataSubset data_found: Subset successfully loaded
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: int fenix::data::member_load_begin(int group_id, int member_id, std::iostream** strm, int time_stamp = FENIX_DATA_SNAPSHOT_LATEST, DataSubset& data_found = SUBSET_IGNORE)

   :param int group_id: The group of the member to load
   :param int member_id: The member to load
   :param std::iostream** strm: [out] Output stream pointer
   :param int time_stamp: Time stamp of the snapshot to load
   :param DataSubset data_found: Subset successfully loaded
   :returns: FENIX_SUCCESS if successful

.. note::
   As :c:func:`Fenix_Data_member_load`, but opens a file to read data from instead of directly
   loading the data. It is an error to call any staging, storing, loading, or restoring function
   involving this member before a corresponding call to :c:func:`Fenix_Data_member_load_end`.
   Returned file is read-only and must not be closed by the user. The value of any data outside
   the found_data subset is undefined.

.. seealso::
   :c:func:`Fenix_Data_member_load_end`, :c:func:`Fenix_Data_member_load`
