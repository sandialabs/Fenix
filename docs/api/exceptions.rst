C++ Exceptions
==============

The Fenix C++ API provides exception types for error handling when
:c:enumerator:`FENIX_RESUME_THROW` is configured.

**Note:** These exception classes are C++ only and have no C equivalent. The C API uses
return codes (see :doc:`return-codes`).

Exception Hierarchy
-------------------

.. cpp:class:: fenix::CommException

   Base exception class for Fenix communication errors.

   Thrown when :c:enumerator:`FENIX_RESUME_THROW` is configured and a fault occurs.

   .. cpp:function:: CommException(MPI_Comm comm, int error_code)

      Construct a CommException.

      :param comm: The communicator where the error occurred
      :param error_code: The Fenix error code

   .. cpp:function:: const char* what() const noexcept

      Get the error message.

      :returns: Error message string

   .. cpp:function:: MPI_Comm get_comm() const

      Get the communicator where the error occurred.

      :returns: MPI communicator

   .. cpp:function:: int get_error_code() const

      Get the Fenix error code.

      :returns: Error code from :c:type:`Fenix_Return_codes`

Error Code Mapping
------------------

Each exception's ``get_error_code()`` returns the corresponding value from :doc:`return-codes`.
Common mappings:

- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` → CommException with -100
- :c:enumerator:`FENIX_ERROR_CANCELLED` → CommException with -101
- :c:enumerator:`FENIX_ERROR_GROUP_CREATE` → CommException with -200

See :doc:`return-codes` for the complete list of error codes.

Usage
-----

When using :c:enumerator:`FENIX_RESUME_THROW` mode, exceptions are thrown on failures:

.. code-block:: cpp

   #include <fenix.hpp>

   try {
       // MPI operations
       MPI_Send(...);
   } catch (const fenix::CommException& e) {
       // Handle failure
       std::cerr << "Fault detected: " << e.what() << std::endl;
       // Recovery happens automatically before exception is thrown
   }

See :doc:`/guides/process-recovery` for more information on exception-based recovery.
