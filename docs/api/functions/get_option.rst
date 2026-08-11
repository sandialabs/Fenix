get_option
==========

.. operation:: local

Get the current value of a global Fenix setting.

.. c:function:: int Fenix_get_option(Fenix_Setting_name setting, unsigned* option)

   :param Fenix_Setting_name setting: [in] The setting name to query (e.g., FENIX_RESUME_MODE, FENIX_RECOVERY_MODE). See :c:type:`Fenix_Setting_name` for valid values.
   :param unsigned* option: [out] Pointer to location where the current setting value will be stored. Must not be NULL.

   **Return Codes:**

   .. list-table::
      :widths: 30 70
      :header-rows: 0

      * - :c:enumerator:`FENIX_SUCCESS`
        - Setting value successfully retrieved
      * - :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
        - Fenix has not been initialized via :c:func:`Fenix_Init`
      * - :c:enumerator:`FENIX_ERROR_INTERN`
        - Invalid setting name or internal error (setting value >= FENIX_SETTING_NAME_MAXCODE or setting value in valid range but not handled in implementation)

.. cpp:function:: unsigned fenix::get_option(SettingName name)

   :param SettingName name: [in] The setting name to query
   :returns: The current value of the setting

.. note::
   The C++ overload returns the value directly instead of through an output parameter.

.. code-block:: c

   // C example
   int mode;
   Fenix_get_option(FENIX_RESUME_MODE, &mode);

.. code-block:: cpp

   // C++ example
   unsigned mode = fenix::get_option(fenix::RESUME_MODE);

.. seealso::
   :c:func:`Fenix_set_option`, :c:type:`Fenix_Setting_name`
