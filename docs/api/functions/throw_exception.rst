throw_exception
===============

.. operation:: local

**C++ API Only**

Throw an exception for the most recent fault.

This function is useful for spare ranks that want to use exception-based recovery
after being activated to replace a failed rank.

.. cpp:function:: void fenix::throw_exception()

   :throws: fenix::CommException with the error code from the most recent fault

.. note::
   This function has no C API equivalent. It is only available in the C++ API.

.. code-block:: cpp

   // C++ example (spare rank)
   if (my_role == fenix::RECOVERED_RANK) {
       try {
           fenix::throw_exception();
       } catch (const fenix::CommException& e) {
           // Handle recovery for this spare
       }
   }

.. seealso::
   :cpp:class:`fenix::CommException`, :c:enumerator:`FENIX_RESUME_THROW`
