mlog_active
===========

.. operation:: local

Get the currently active message log.

.. c:function:: int Fenix_Mlog_active(int* mlog_id)

   :param int* mlog_id: [out] Pointer to store the currently active message log ID. Set to FENIX_MLOG_NONE if no message log is currently active.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: int fenix::mlog::active()

   :returns: The currently active message log ID, or FENIX_MLOG_NONE if none active

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Query succeeded, mlog_id contains the active log ID (or FENIX_MLOG_NONE if none active)
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized via :c:func:`Fenix_Init`

**Notes:**

- This is a purely local operation with no communication
- Returns FENIX_MLOG_NONE (set in mlog_id parameter) if no message log is active or if Fenix has been finalized
- The C++ version does not check initialization and may return undefined values if called before initialization

.. seealso::
   :c:func:`Fenix_Mlog_activate`, :c:func:`Fenix_Mlog_create`
