Integrate Fenix with CMake Projects
====================================

This guide shows you how to integrate Fenix into your CMake-based project using ``find_package``, ``target_link_libraries``, and related CMake features. Follow these best practices for a robust build system.

.. contents:: On this page
   :local:
   :depth: 2

Quick Start
-----------

Minimal CMakeLists.txt:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(my_fault_tolerant_app)

   # Find MPI
   find_package(MPI REQUIRED)

   # Find Fenix
   find_package(fenix REQUIRED)

   # Create executable
   add_executable(my_app main.cpp)

   # Link libraries
   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)

Build your project:

.. code-block:: bash

   mkdir build && cd build
   cmake .. -DCMAKE_PREFIX_PATH=/path/to/fenix/install
   make

Using find_package
------------------

Basic Usage
~~~~~~~~~~~

.. code-block:: cmake

   find_package(fenix REQUIRED)

**This provides:**

- ``fenix::fenix`` target - Main Fenix library
- ``fenix::mlog`` target - Message logging library
- Fenix include directories
- Fenix compile definitions

**If Fenix is not found:**

CMake will produce an error:

.. code-block:: text

   CMake Error at CMakeLists.txt:5 (find_package):
     Could not find a package configuration file provided by "fenix"

Specifying Fenix Location
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Method 1: CMAKE_PREFIX_PATH**

.. code-block:: bash

   cmake .. -DCMAKE_PREFIX_PATH=/opt/fenix

**Method 2: fenix_DIR**

.. code-block:: bash

   cmake .. -Dfenix_DIR=/opt/fenix/cmake

**Method 3: Environment variable**

.. code-block:: bash

   export CMAKE_PREFIX_PATH=/opt/fenix
   cmake ..

**Method 4: In CMakeLists.txt**

.. code-block:: cmake

   list(APPEND CMAKE_PREFIX_PATH "/opt/fenix")
   find_package(fenix REQUIRED)

Optional vs Required
~~~~~~~~~~~~~~~~~~~~

**Required (recommended):**

.. code-block:: cmake

   find_package(fenix REQUIRED)
   # Build fails if Fenix not found

**Optional:**

.. code-block:: cmake

   find_package(fenix QUIET)
   if(fenix_FOUND)
     # Enable Fenix features
     target_link_libraries(my_app PRIVATE fenix)
     target_compile_definitions(my_app PRIVATE HAVE_FENIX)
   else()
     message(WARNING "Fenix not found, fault tolerance disabled")
   endif()

Application code:

.. code-block:: cpp

   #ifdef HAVE_FENIX
     #include <fenix.hpp>
     fenix::init({.out_comm = &res_comm, .spares = 2});
   #else
     res_comm = MPI_COMM_WORLD;
   #endif

Version Requirements
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   find_package(fenix 1.0 REQUIRED)  # Require at least version 1.0

   # Or exact version
   find_package(fenix 1.0 EXACT REQUIRED)

Linking Libraries
-----------------

Using target_link_libraries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Modern CMake approach (recommended):**

.. code-block:: cmake

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)

**Visibility keywords:**

- ``PRIVATE`` - Only this target uses Fenix (most common)
- ``PUBLIC`` - This target and dependents use Fenix
- ``INTERFACE`` - Only dependents use Fenix (header-only wrappers)

**Example:**

.. code-block:: cmake

   # Application executable (uses Fenix)
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)

   # Shared library (exposes Fenix in its API)
   add_library(my_lib SHARED lib.cpp)
   target_link_libraries(my_lib PUBLIC fenix MPI::MPI_CXX)

   # Header-only utility library
   add_library(my_utils INTERFACE)
   target_link_libraries(my_utils INTERFACE fenix)

Message Logging Library
~~~~~~~~~~~~~~~~~~~~~~~

If using message logging features:

.. code-block:: cmake

   target_link_libraries(my_app PRIVATE fenix::mlog fenix MPI::MPI_CXX)

**Note:** ``fenix::mlog`` should be linked before ``fenix`` to ensure proper symbol resolution.

Include Directories
-------------------

Automatic Inclusion
~~~~~~~~~~~~~~~~~~~

When you link ``fenix``, include directories are added automatically:

.. code-block:: cmake

   target_link_libraries(my_app PRIVATE fenix)
   # No need for: include_directories(${FENIX_INCLUDE_DIRS})

**You can now use:**

.. code-block:: cpp

   #include <fenix.h>    // C API
   #include <fenix.hpp>  // C++ API

Manual Control (advanced)
~~~~~~~~~~~~~~~~~~~~~~~~~~

If you need explicit control:

.. code-block:: cmake

   get_target_property(FENIX_INCLUDE_DIRS fenix INTERFACE_INCLUDE_DIRECTORIES)
   target_include_directories(my_app PRIVATE ${FENIX_INCLUDE_DIRS})

Handling Dependencies
---------------------

MPI Dependency
~~~~~~~~~~~~~~

Fenix requires MPI. Find it first:

.. code-block:: cmake

   find_package(MPI REQUIRED)

   find_package(fenix REQUIRED)

   target_link_libraries(my_app PRIVATE
     fenix
     MPI::MPI_CXX  # For C++
     # Or MPI::MPI_C for C
   )

**Note:** Fenix transitively requires MPI, but it's best practice to explicitly find and link MPI in your project.

C++ Standard Requirement
~~~~~~~~~~~~~~~~~~~~~~~~~

Fenix requires C++20:

.. code-block:: cmake

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Or per-target
   add_executable(my_app main.cpp)
   target_compile_features(my_app PRIVATE cxx_std_20)

Complete CMakeLists.txt Examples
---------------------------------

Simple Application
~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(simple_fenix_app VERSION 1.0.0 LANGUAGES CXX)

   # C++20 required
   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Find dependencies
   find_package(MPI REQUIRED)
   find_package(fenix REQUIRED)

   # Build executable
   add_executable(simple_app main.cpp)
   target_link_libraries(simple_app PRIVATE fenix MPI::MPI_CXX)

   # Install
   install(TARGETS simple_app DESTINATION bin)

**Build:**

.. code-block:: bash

   mkdir build && cd build
   cmake .. -DCMAKE_PREFIX_PATH=/opt/fenix
   make
   ./simple_app

Multiple Executables
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(multi_app)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   find_package(MPI REQUIRED)
   find_package(fenix REQUIRED)

   # Common library used by all apps
   add_library(common STATIC
     common/utils.cpp
     common/checkpoint.cpp
   )
   target_link_libraries(common PUBLIC fenix MPI::MPI_CXX)

   # Application 1
   add_executable(app1 app1/main.cpp)
   target_link_libraries(app1 PRIVATE common)

   # Application 2 with message logging
   add_executable(app2 app2/main.cpp)
   target_link_libraries(app2 PRIVATE fenix::mlog common)

   # Application 3 - C API
   add_executable(app3 app3/main.c)
   target_link_libraries(app3 PRIVATE fenix MPI::MPI_C)

Library with Fenix
~~~~~~~~~~~~~~~~~~

Creating a library that uses Fenix:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(fenix_library VERSION 1.0.0)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   find_package(MPI REQUIRED)
   find_package(fenix REQUIRED)

   # Shared library
   add_library(mylib SHARED
     src/resilient_solver.cpp
     src/checkpoint_manager.cpp
   )

   # Public: exposed to library users
   target_link_libraries(mylib PUBLIC fenix MPI::MPI_CXX)

   target_include_directories(mylib PUBLIC
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
     $<INSTALL_INTERFACE:include>
   )

   # Install library
   install(TARGETS mylib EXPORT mylibTargets
     LIBRARY DESTINATION lib
     ARCHIVE DESTINATION lib
     RUNTIME DESTINATION bin
   )

   # Install headers
   install(DIRECTORY include/ DESTINATION include)

   # Export targets for find_package
   install(EXPORT mylibTargets
     FILE mylibTargets.cmake
     NAMESPACE mylib::
     DESTINATION cmake
   )

   # Create package config file
   include(CMakePackageConfigHelpers)
   configure_package_config_file(
     cmake/mylibConfig.cmake.in
     ${CMAKE_CURRENT_BINARY_DIR}/mylibConfig.cmake
     INSTALL_DESTINATION cmake
   )

   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mylibConfig.cmake
     DESTINATION cmake
   )

**mylibConfig.cmake.in:**

.. code-block:: text

   @PACKAGE_INIT@

   include(CMakeFindDependencyMacro)

   find_dependency(MPI REQUIRED)
   find_dependency(fenix REQUIRED)

   include("${CMAKE_CURRENT_LIST_DIR}/mylibTargets.cmake")

   check_required_components(mylib)

**Using the library:**

.. code-block:: cmake

   find_package(mylib REQUIRED)
   target_link_libraries(my_app PRIVATE mylib::mylib)

With Optional Features
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(optional_fenix_app)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Required
   find_package(MPI REQUIRED)

   # Optional Fenix
   option(USE_FENIX "Enable fault tolerance with Fenix" ON)

   if(USE_FENIX)
     find_package(fenix QUIET)
     if(fenix_FOUND)
       message(STATUS "Fenix found: fault tolerance enabled")
       set(HAVE_FENIX 1)
     else()
       message(WARNING "Fenix not found: fault tolerance disabled")
       set(HAVE_FENIX 0)
     endif()
   else()
     set(HAVE_FENIX 0)
   endif()

   # Configure header
   configure_file(config.h.in config.h)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE MPI::MPI_CXX)

   if(HAVE_FENIX)
     target_link_libraries(my_app PRIVATE fenix)
   endif()

   target_include_directories(my_app PRIVATE
     ${CMAKE_CURRENT_BINARY_DIR}  # For config.h
   )

**config.h.in:**

.. code-block:: c

   #ifndef CONFIG_H
   #define CONFIG_H

   #cmakedefine HAVE_FENIX

   #endif

**main.cpp:**

.. code-block:: cpp

   #include "config.h"
   #include <mpi.h>

   #ifdef HAVE_FENIX
   #include <fenix.hpp>
   #endif

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     MPI_Comm comm;
   #ifdef HAVE_FENIX
     fenix::init({.out_comm = &comm, .spares = 2});
   #else
     comm = MPI_COMM_WORLD;
   #endif

     // Application code...

     MPI_Finalize();
     return 0;
   }

Subdirectory Integration
~~~~~~~~~~~~~~~~~~~~~~~~~

If including Fenix as a subdirectory (e.g., via git submodule):

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(my_app)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   find_package(MPI REQUIRED)

   # Option 1: Add Fenix subdirectory
   add_subdirectory(external/fenix)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)

   # Option 2: Use FetchContent (CMake 3.24+)
   include(FetchContent)
   FetchContent_Declare(
     fenix
     GIT_REPOSITORY https://github.com/your-org/fenix.git
     GIT_TAG        v1.0.0
   )
   FetchContent_MakeAvailable(fenix)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)

Best Practices
--------------

Modern CMake Targets
~~~~~~~~~~~~~~~~~~~~

**Do:**

.. code-block:: cmake

   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)

**Don't:**

.. code-block:: cmake

   # Old-style, avoid:
   link_libraries(fenix ${MPI_LIBRARIES})
   include_directories(${FENIX_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})

Generator Expressions
~~~~~~~~~~~~~~~~~~~~~

Use generator expressions for flexibility:

.. code-block:: cmake

   target_compile_definitions(my_app PRIVATE
     $<$<CONFIG:Debug>:FENIX_DEBUG>
     $<$<BOOL:${USE_FENIX}>:HAVE_FENIX>
   )

   target_compile_options(my_app PRIVATE
     $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
     $<$<CXX_COMPILER_ID:Clang>:-Weverything>
   )

Installation Paths
~~~~~~~~~~~~~~~~~~

Use GNUInstallDirs for standard paths:

.. code-block:: cmake

   include(GNUInstallDirs)

   install(TARGETS my_app
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   )

   install(FILES config.h
     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/myapp
   )

Build Type Handling
~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   # Set default build type
   if(NOT CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE Release CACHE STRING
         "Choose the type of build (Debug Release RelWithDebInfo MinSizeRel)"
         FORCE)
   endif()

   message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

Troubleshooting
---------------

Problem: fenix not found
~~~~~~~~~~~~~~~~~~~~~~~~

**Error:**

.. code-block:: text

   CMake Error: Could not find a package configuration file provided by "fenix"

**Solutions:**

1. Set CMAKE_PREFIX_PATH:

   .. code-block:: bash

      cmake .. -DCMAKE_PREFIX_PATH=/opt/fenix

2. Verify Fenix installation:

   .. code-block:: bash

      ls /opt/fenix/cmake/fenixConfig.cmake

3. Check fenix was installed with ``make install``.

Problem: Undefined references to Fenix symbols
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Error:**

.. code-block:: text

   undefined reference to `Fenix_Init'

**Solutions:**

1. Ensure correct link order:

   .. code-block:: cmake

      target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX)
      # Not: target_link_libraries(my_app PRIVATE MPI::MPI_CXX fenix)

2. For message logging, link mlog first:

   .. code-block:: cmake

      target_link_libraries(my_app PRIVATE fenix::mlog fenix MPI::MPI_CXX)

3. Verify target type:

   .. code-block:: cmake

      message(STATUS "Fenix target: ${fenix_FOUND}")
      get_target_property(FENIX_LOC fenix LOCATION)
      message(STATUS "Fenix library: ${FENIX_LOC}")

Problem: Wrong MPI headers included
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:**

- Segfaults in MPI calls
- MPI type mismatches

**Solution:**

Fenix's MPI header fix should propagate automatically if ``FENIX_PROPAGATE_INC_FIX=ON`` was used when building Fenix. Verify:

.. code-block:: bash

   grep FENIX_PROPAGATE_INC_FIX /opt/fenix/cmake/fenixConfig.cmake

Problem: C++20 not enabled
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Error:**

.. code-block:: text

   error: 'concept' does not name a type

**Solution:**

.. code-block:: cmake

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

Or:

.. code-block:: cmake

   target_compile_features(my_app PRIVATE cxx_std_20)

Problem: Link errors with static libraries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Error:**

.. code-block:: text

   undefined reference to pthread_create

**Solution:**

Link required system libraries:

.. code-block:: cmake

   find_package(Threads REQUIRED)
   target_link_libraries(my_app PRIVATE fenix MPI::MPI_CXX Threads::Threads)

Testing Integration
-------------------

Add tests to your project:

.. code-block:: cmake

   enable_testing()

   add_executable(test_fenix test/test_fenix.cpp)
   target_link_libraries(test_fenix PRIVATE fenix MPI::MPI_CXX)

   add_test(NAME test_fenix
     COMMAND ${MPIEXEC_EXECUTABLE}
             ${MPIEXEC_NUMPROC_FLAG} 4
             ${MPIEXEC_PREFLAGS}
             --with-ft mpi
             $<TARGET_FILE:test_fenix>
   )

   set_tests_properties(test_fenix PROPERTIES
     TIMEOUT 30
     PROCESSORS 4
   )

Run tests:

.. code-block:: bash

   cd build
   ctest -V

Packaging
---------

CPack for Distribution
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   include(CPack)

   set(CPACK_PACKAGE_NAME "my_fenix_app")
   set(CPACK_PACKAGE_VERSION_MAJOR 1)
   set(CPACK_PACKAGE_VERSION_MINOR 0)
   set(CPACK_PACKAGE_VERSION_PATCH 0)

   set(CPACK_GENERATOR "TGZ;DEB")

Create package:

.. code-block:: bash

   make package

Verification Checklist
----------------------

After integrating Fenix, verify:

- [ ] Project builds without errors
- [ ] ``find_package(fenix REQUIRED)`` succeeds
- [ ] Fenix headers are found: ``#include <fenix.hpp>`` compiles
- [ ] Linking succeeds without undefined symbols
- [ ] MPI fault tolerance flags are used (``--with-ft mpi``)
- [ ] Tests pass
- [ ] Installation works (``make install``)
- [ ] Installed binaries run correctly

See Also
--------

- :doc:`configure-cmake` - Configuring Fenix build options
- :doc:`/troubleshooting` - Common problems and solutions
- :doc:`migrate-existing-app` - Adding Fenix to existing applications
- `CMake Documentation <https://cmake.org/documentation/>`_
