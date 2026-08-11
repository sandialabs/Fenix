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

   .. cpp:function:: CommException(MPI_Comm comm, int fenix_error, int mpi_error)

      Construct a CommException.

      :param comm: The communicator where the error occurred
      :param fenix_error: The Fenix error code
      :param mpi_error: The MPI error code

   .. cpp:member:: MPI_Comm repaired_comm

      The repaired communicator after fault recovery.

   .. cpp:member:: const int fenix_err

      The Fenix error code (from :c:type:`Fenix_Return_codes`).

   .. cpp:member:: const int mpi_err

      The MPI error code from the failed operation.

Error Code Mapping
------------------

Each exception's ``fenix_err`` member contains the corresponding value from :doc:`return-codes`.
Common error codes:

- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` → -100
- :c:enumerator:`FENIX_ERROR_NOCATEGORY` → -101
- :c:enumerator:`FENIX_ERROR_GROUP_CREATE` → -103
- :c:enumerator:`FENIX_ERROR_CANCELLED` → -127

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
       std::cerr << "Fault detected on communicator" << std::endl;
       std::cerr << "Fenix error: " << e.fenix_err << std::endl;
       std::cerr << "MPI error: " << e.mpi_err << std::endl;
       // Recovery happens automatically before exception is thrown
   }

See :doc:`/guides/process-recovery` for more information on exception-based recovery.
