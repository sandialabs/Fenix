C++ API Core
============

The Fenix C++ API provides modern C++ wrappers around the C API. Most functions are overloads
that provide more convenient interfaces.

See :doc:`/api/c-api/index` for detailed documentation of the underlying C functions.

Namespace: fenix
----------------

The main ``fenix`` namespace contains C++ wrappers for process recovery functions.

Configuration
^^^^^^^^^^^^^

.. cpp:function:: void set_option(SettingName name, int value)

   C++ overload of :c:func:`Fenix_set_option`.

.. cpp:function:: int get_option(SettingName name)

   C++ overload of :c:func:`Fenix_get_option` returning the option directly.

Query Functions
^^^^^^^^^^^^^^^

.. cpp:function:: Role get_role()

   C++ overload of :c:func:`Fenix_get_role`.

.. cpp:function:: int get_error()

   C++ overload of :c:func:`Fenix_get_error`.

.. cpp:function:: int get_nspare()

   C++ overload of :c:func:`Fenix_get_nspare`.

.. cpp:function:: bool initialized()

   C++ overload of :c:func:`Fenix_Initialized` that directly returns true if initialized.

.. cpp:function:: bool finalized()

   C++ overload of :c:func:`Fenix_Finalized` that directly returns true if finalized.

Callbacks
^^^^^^^^^

.. cpp:function:: void register_callback(std::function<void(MPI_Comm, int)> callback)

   C++ overload of :c:func:`Fenix_Callback_register` using std::function.

Failure Detection
^^^^^^^^^^^^^^^^^

.. cpp:function:: void detect_failures(MPI_Comm& newcomm, int& error)

   C++ overload of :c:func:`Fenix_Process_detect_failures`.

.. cpp:function:: void throw_recent_error()

   Throw an exception for the most recent fault. Helpful for spares.

Namespace: fenix::data
----------------------

The ``fenix::data`` namespace contains C++ wrappers for data recovery functions.

Type Aliases
^^^^^^^^^^^^

.. cpp:type:: SerializeFileFunc

   Function pointer type for custom serialization without context pointer.

.. cpp:type:: SerializeStreamFunc

   Function pointer type for serialization using std::iostream instead of FILE*.

Data Member Operations
^^^^^^^^^^^^^^^^^^^^^^

.. cpp:function:: void member_create(int group_id, int member_id, SerializeStreamFunc serialize_fn, std::iostream& stream)

   C++ overload of :c:func:`Fenix_Data_member_fcreate` using std::iostream.

.. cpp:function:: void member_define(int group_id, int member_id, void* source_buffer, int count, MPI_Datatype datatype)

   C++ overload of :c:func:`Fenix_Data_member_define`.

.. cpp:function:: void member_stage(int group_id, int member_id, const DataSubset& subset)

   C++ overload of :c:func:`Fenix_Data_member_stage`.

.. cpp:function:: void member_stage_inplace(int group_id, int member_id, void* source_buffer, const DataSubset& subset)

   C++ overload of :c:func:`Fenix_Data_member_stage_inplace`.

.. cpp:function:: void member_stage_begin(int group_id, int member_id, std::iostream& stream)

   C++ overload of :c:func:`Fenix_Data_member_stage_begin` using std::iostream.

Constants
^^^^^^^^^

.. cpp:var:: const DataSubset& SUBSET_FULL

   Predefined subset representing all data.

.. cpp:var:: const DataSubset& SUBSET_EMPTY

   Predefined subset representing no data.

.. cpp:var:: const DataSubset& SUBSET_PRESTAGED

   Predefined subset for prestaged data.

.. cpp:var:: DataSubset SUBSET_IGNORE

   Predefined subset to ignore data.
