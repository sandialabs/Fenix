get_option
==========

.. operation:: local

Get the current value of a global Fenix setting.

.. c:function:: int Fenix_get_option(Fenix_Setting_name setting, unsigned* option)

   :param Fenix_Setting_name setting: The setting to query
   :param unsigned* option: Pointer where the value will be stored
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: unsigned fenix::get_option(SettingName name)

   :param SettingName name: The setting to query
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
