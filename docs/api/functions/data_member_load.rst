member_load
===========

.. operation:: local

Load a member's snapshot data into user's data.

.. c:function:: int Fenix_Data_member_load(int group_id, int member_id, int time_stamp, Fenix_Data_subset* data_found)

.. cpp:function:: int fenix::data::member_load(int group_id, int member_id, int time_stamp, DataSubset& data_found)

   :param int group_id: The group containing the member
   :param int member_id: The member to load 
   :param int time_stamp: Timestamp of snapshot to load. Default: FENIX_DATA_SNAPSHOT_ALL
   :param DataSubset data_found: Subset successfully loaded. Default: SUBSET_IGNORE
   :returns: FENIX_SUCCESS if successful, FENIX_WARNING_PARTIAL_RESTORE if only some elements were available, or an error code.

Attempts to load up to ATTRIBUTE_COUNT elements into ATTRIBUTE_BUFFER. For members without
a serializer, data is loaded by directly copying memory. Otherwise, data is loaded by calls
to this member's serializer.

Loads from the snapshot specified by timestamp. If FENIX_DATA_SNAPSHOT_LATEST is
passed, loads from the latest committed snapshot. If FENIX_DATA_SNAPSHOT_ALL is
passed, loads each element from the most recent available snapshot it was staged
to.

If a snapshot at the specified timestamp is not locally available, the function
raises ``FENIX_ERROR_NODATA_FOUND``. If only some elements are available, the
function returns ``FENIX_WARNING_PARTIAL_RESTORE``.

For the C API, ``data_found`` must point to an uninitialized subset that the
caller is then responsible for freeing, unless ``FENIX_DATA_SUBSET_IGNORE`` is
passed.

If the given snapshot is not found (or no snapshot is found), raises
FENIX_ERROR_NODATA_FOUND.


.. c:function:: int Fenix_Data_member_load_to(int group_id, int member_id, void* target, int target_count, int time_stamp, Fenix_Data_subset* found_data)

.. cpp:function:: int fenix::data::member_load(int group_id, int member_id, void* target, int target_count, int time_stamp, DataSubset& data_found)

   :param void* target: Location to load to
   :param int target_count: Number of elements to load

As above, but attempts to load elements [0, target_count) into target. If target
is FENIX_DATA_RESTORE_INPLACE, attempts to load to member's current
ATTRIBUTE_BUFFER. If target_count is FENIX_DATA_RESTORE_FULL, assumes target has
space to load all available elements.


Manually Loading
----------------

.. c:function:: int Fenix_Data_member_load_begin(int group_id, int member_id, FILE** fpp, int time_stamp, Fenix_Data_subset* found_data)

.. cpp:function:: int fenix::data::member_load_begin(int group_id, int member_id, FILE** fp, int time_stamp)

   :param FILE** fp: Location to store file pointer to be read from

.. cpp:function:: int fenix::data::member_load_begin(int group_id, int member_id, std::iostream** strm, int time_stamp, DataSubset& data_found)

   :param std::iostream** strm: Location to store stream pointer to be read from.

The load_begin functions open a file or stream for reading a given snapshot from
directly. Consequently, time_stamp cannot be FENIX_DATA_SNAPSHOT_ALL.

It is an error to call any staging, storing, loading, or restoring function
involving this member before a corresponding call to
:cpp:func:`member_load_end`. Returned file or stream is read-only and must not
be closed by the user. The value of any data outside the found_data subset is
undefined.


.. c:function:: int Fenix_Data_member_load_end(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_load_end(int group_id, int member_id)

Concludes a :cpp:func:`member_load_begin`. Raises FENIX_ERROR_INVALID_LOGIC_CALL
if the member is not currently open. It is undefined behavior to continue using
the file or stream returns by :cpp:func:`member_load_begin`.

.. seealso::
   :c:func:`Fenix_Data_member_restore`, :c:func:`Fenix_Data_member_repair`
