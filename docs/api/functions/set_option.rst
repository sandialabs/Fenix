set_option
==========

.. operation:: local

Configure a global Fenix setting.

Each :c:type:`Fenix_Setting_name` will describe its function and valid options.
If called prior to Fenix_Init, the setting will apply to future Fenix_Inits.

.. c:function:: int Fenix_set_option(Fenix_Setting_name setting, unsigned option)

   :param Fenix_Setting_name setting: The setting to configure
   :param unsigned option: The new value
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: void fenix::set_option(SettingName name, int value)

   :param SettingName name: The setting to configure
   :param int value: The new value

.. note::
   The C++ overload accepts the value directly instead of through a pointer.

.. code-block:: c

   // C example
   Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_THROW);

.. code-block:: cpp

   // C++ example
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

.. seealso::
   :c:func:`Fenix_get_option`, :c:type:`Fenix_Setting_name`
