.. _return-codes:

Return Codes
============

.. c:type:: Fenix_Return_codes

   All possible return codes from Fenix C API functions. Errors are negative, warnings are positive.

   **C++ Exception Mapping:**

   When :c:enumerator:`FENIX_RESUME_THROW` is enabled, the C++ API throws
   :cpp:class:`fenix::CommException` instead of returning error codes. The exception's
   ``error`` member contains the corresponding return code value.

   See :doc:`exceptions` for C++ exception classes.

   **Success:**

   .. c:enumerator:: FENIX_SUCCESS

      The operation completed successfully (value: 0)

   **Errors (negative values):**

   .. c:enumerator:: FENIX_ERROR_UNINITIALIZED

      Fenix has not been initialized (value: -100)

   .. c:enumerator:: FENIX_ERROR_NOCATEGORY

      Error has no specific category

   .. c:enumerator:: FENIX_ERROR_CALLBACK_NOT_REGISTERED

      Attempted to pop a callback that was not registered

   .. c:enumerator:: FENIX_ERROR_GROUP_CREATE

      Failed to create data group

   .. c:enumerator:: FENIX_ERROR_MEMBER_CREATE

      Failed to create data member

   .. c:enumerator:: FENIX_ERROR_MEMBER_EXISTS

      Data member already exists

   .. c:enumerator:: FENIX_ERROR_MEMBER_STAGING

      Error during data member staging

   .. c:enumerator:: FENIX_ERROR_MEMBER_LOADING

      Error during data member loading

   .. c:enumerator:: FENIX_ERROR_COMMIT_BARRIER

      Error during commit barrier

   .. c:enumerator:: FENIX_ERROR_INVALID_GROUPID

      Invalid group ID provided

   .. c:enumerator:: FENIX_ERROR_INVALID_MEMBERID

      Invalid member ID provided

   .. c:enumerator:: FENIX_ERROR_INVALID_LOGIC_CALL

      Function called in invalid context

   .. c:enumerator:: FENIX_ERROR_INVALID_POLICY_NAME

      Invalid policy name provided

   .. c:enumerator:: FENIX_ERROR_INVALID_TIMESTAMP

      Invalid timestamp provided

   .. c:enumerator:: FENIX_ERROR_INVALID_TIMESTART

      Invalid time start value

   .. c:enumerator:: FENIX_ERROR_INVALID_DEPTH

      Invalid depth value

   .. c:enumerator:: FENIX_ERROR_INVALID_ATTRIBUTE_NAME

      Invalid attribute name

   .. c:enumerator:: FENIX_ERROR_INVALID_ATTRIBUTE_VALUE

      Invalid attribute value

   .. c:enumerator:: FENIX_ERROR_INVALID_POSITION

      Invalid position value

   .. c:enumerator:: FENIX_ERROR_INVALID_SUBSET

      Invalid subset specification (invalid element ranges or malformed subset)

   .. c:enumerator:: FENIX_ERROR_DATA_WAIT

      Error during data wait operation

   .. c:enumerator:: FENIX_ERROR_SUBSET_NUM_BLOCKS

      Invalid number of blocks in subset (must be >= 0)

   .. c:enumerator:: FENIX_ERROR_SUBSET_START_OFFSET

      Invalid start offset in subset (start must be <= end for each block)

   .. c:enumerator:: FENIX_ERROR_SUBSET_END_OFFSET

      Invalid end offset in subset (end must be >= start for each block)

   .. c:enumerator:: FENIX_ERROR_SUBSET_STRIDE

      Invalid stride in subset (stride must be > 0 if num_blocks > 1)

   .. c:enumerator:: FENIX_ERROR_NODATA_FOUND

      No data found for restore operation

   .. c:enumerator:: FENIX_ERROR_INTERN

      Internal error

   .. c:enumerator:: FENIX_ERROR_CANCELLED

      Operation was cancelled

   .. c:enumerator:: FENIX_ERROR_INVALID_SETTING_NAME

      Invalid setting name

   .. c:enumerator:: FENIX_ERROR_INVALID_SETTING_OPTION

      Invalid setting option value

   .. c:enumerator:: FENIX_ERROR_INVALID_MLOGID

      Invalid message log ID

   .. c:enumerator:: FENIX_ERROR_MLOG_EXISTS

      Message log already exists

   .. c:enumerator:: FENIX_ERROR_MLOG_LIBRARY_UNAVAILABLE

      Message logging library unavailable

   .. c:enumerator:: FENIX_ERROR_PROCESS_FAILURE

      Process failure detected

   **Warnings (positive values):**

   .. c:enumerator:: FENIX_WARNING_SPARE_RANKS_DEPLETED

      Spare ranks have been depleted, communicator was shrunk (value: 100)

   .. c:enumerator:: FENIX_WARNING_PARTIAL_RESTORE

      Data restore was only partially successful
