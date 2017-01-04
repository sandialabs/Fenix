#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top DHARMA
#  directory.
#
cmake_minimum_required (VERSION 2.7)
project (Fenix)
# The version number.
set (Fenix_VERSION_MAJOR 1)
set (Fenix_VERSION_MINOR 0)

# Set empty string for shared linking (we use static library only at this moment)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

# THis is include directories
include_directories( "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}"
  "${CMAKE_BINARY_DIR}/lib" "${CMAKE_BINARY_DIR}" )

  #ENABLE_TESTING
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
  #include(testref/TestAgainstReference)

  add_subdirectory(src)
  #add_subdirectory(test/subset_internal)
  add_subdirectory(examples/01_hello_world/fenix)
  add_subdirectory(examples/01_hello_world/mpi)
  add_subdirectory(examples/02_send_recv/fenix)
  add_subdirectory(examples/02_send_recv/mpi)
  add_subdirectory(examples/03_reduce/fenix)
  add_subdirectory(examples/03_reduce/mpi)
  add_subdirectory(examples/04_Isend_Irecv/fenix)
  add_subdirectory(examples/04_Isend_Irecv/mpi)
  add_subdirectory(examples/05_subset_create)
  add_subdirectory(examples/06_subset_createv)
