member_stage
============

.. operation:: local

Serialize a group member's data into the member's local store.

.. c:function:: int Fenix_Data_member_stage(int group_id, int member_id, const Fenix_Data_subset subset_specifier)

.. cpp:function:: int fenix::data::member_stage(int group_id, int member_id, const DataSubset& subset)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param DataSubset subset: Element ranges to stage. Default: SUBSET_FULL (all elements).
   :returns: FENIX_SUCCESS if successful, error code otherwise

A store operation can be broken into two parts: locally staging the data within Fenix, then
policy-specific operations to make the data resilient to faults. This function performs ONLY
the first part. Applications should subsequently make a store of this member to the
FENIX_DATA_SUBSET_PRESTAGED data subset. It is undefined behavior to commit a group with any
members with staged but unstored data.

.. warning::
   FENIX_DATA_SUBSET_ALL is invalid if member size is FENIX_RESIZEABLE unless the member has a serializer defined.


.. c:function:: int Fenix_Data_member_stage_inplace(int group_id, int member_id, void* buf, const Fenix_Data_subset subset)

.. cpp:function:: int fenix::data::member_stage_inplace(int group_id, int member_id, void* buf, const DataSubset& subset)

   :param int group_id: Group of the member to stage to
   :param int member_id: Member to stage to
   :param void* buf: [in] Buffer that Fenix takes ownership of
   :param DataSubset subset: Element ranges to stage. Default: SUBSET_FULL (all elements).
   :returns: FENIX_SUCCESS if successful, error code otherwise

The inplace variants of staging allow a user to pass off control of a
pre-serialized buffer for Fenix to use as the staging data. Any prior staged
but uncommitted data is lost.

The buffer is interpreted to start at element 0. For non-contiguous subsets,
the buffer is assumed to NOT be packed (i.e., the elements not included in the
subset are still present in the buffer, with undefined states). Buffer must have
been malloc'ed. Caller is not permitted to continue using the buffer space.


Manually Staging
----------------

.. c:function:: int Fenix_Data_member_stage_begin(int group_id, int member_id, FILE** fpp)

.. cpp:function:: int fenix::data::member_stage_begin(int group_id, int member_id, FILE** fp)

.. cpp:function:: int fenix::data::member_stage_begin(int group_id, int member_id, std::iostream** stream)

These manual variants of staging functions allow users to open a file or stream
to stage their data.

This function is equivalent to performing a :cpp:func:`member_stage_inplace`
with buf pointing to the written data and a subset of FENIX_DATA_SUBSET_FULL.
For resizable members, the subset is instead of the range
[0, staging_file_size/element_size] and it is an error if the staging file's
size is not divisible by the element size.

It is an error to call any staging, storing, loading, or restoring function
involving this member before a corresponding call to :cpp:func:`member_stage_end`.
File must not be closed by the user. It is an error to use the file or stream
after the corresponding :cpp:func:`member_stage_end`.

.. c:function:: int Fenix_Data_member_stage_end(int group_id, int member_id)

.. cpp:function:: int fenix::data::member_stage_end(int group_id, int member_id)

Ends a corresponding :cpp:func:`member_stage_begin`.

.. seealso::
   :c:func:`Fenix_Data_member_store`, :c:func:`Fenix_Data_member_stage_inplace`
