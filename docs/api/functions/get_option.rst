get_option
==========

.. operation:: local

Get the current value of a global Fenix setting.

.. c:function:: int Fenix_get_option(Fenix_Setting_name setting, unsigned* option)

   :param Fenix_Setting_name setting: The setting name to query. See :c:type:`Fenix_Setting_name` for valid values.
   :param unsigned* option: Pointer to location where the current setting value will be stored. Must not be NULL.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: unsigned fenix::get_option(SettingName name)

   :param SettingName name: The setting name to query
   :returns: The current value of the setting

.. seealso::
   :c:func:`Fenix_set_option`, :c:type:`Fenix_Setting_name`
