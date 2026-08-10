API Reference
=============

Complete unified API reference for Fenix C and C++ interfaces.

Functions are documented together showing both C and C++ interfaces:

- **C API**: Functions prefixed with ``Fenix_`` (e.g., ``Fenix_Init``)
- **C++ API**: Functions in the ``fenix::`` namespace (e.g., ``fenix::init``)

Both APIs share the same core functionality and types. The C++ API provides
modern conveniences while maintaining compatibility with the C API.

**Key differences:**

- **Error handling**: C uses return codes, C++ can use exceptions (with FENIX_RESUME_THROW)
- **Return values**: C++ functions often return values directly instead of through output parameters
- **Callbacks**: C++ uses std::function instead of function pointers with void* context
- **Collections**: C++ provides std::vector return values for query operations

.. toctree::
   :maxdepth: 2
   :titlesonly:

   common
   process-recovery
   data-recovery
   message-recovery
   types
   return-codes
   exceptions
