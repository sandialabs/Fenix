set_option
==========

.. operation:: local

Configure a global Fenix setting.

.. c:function:: int Fenix_set_option(Fenix_Setting_name setting, unsigned option)

   :param Fenix_Setting_name setting: Setting to configure
   :param unsigned option: Value to assign to the setting
   :returns: :c:enumerator:`FENIX_SUCCESS` if successful

.. cpp:function:: void fenix::set_option(fenix::SettingName setting, unsigned option)

   :param SettingName setting: Setting to configure
   :param unsigned option: Value to assign to the setting

If called prior to :c:func:`Fenix_Init`, the setting applies to future
initializations. Settings configured during initialization override prior
configurations. Unless otherwise noted, settings should be consistent across
ranks.

See :doc:`/api/options` for the available settings and their valid options.

.. seealso::
   :c:func:`Fenix_get_option`, :c:func:`Fenix_Init`, :c:type:`Fenix_Setting_name`, :doc:`/api/options`
