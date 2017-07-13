#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#
cmake_minimum_required (VERSION 2.6)
project (Fenix)
# The version number.
set (FENIX_VERSION_MAJOR 1)
set (FENIX_VERSION_MINOR 0)

find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})

# Set empty string for shared linking (we use static library only at this moment)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

# This is include directories
include_directories(
"${CMAKE_SOURCE_DIR}/include"
"${CMAKE_SOURCE_DIR}"
"${CMAKE_SOURCE_DIR}/src"
"${CMAKE_BINARY_DIR}/lib"
"${CMAKE_BINARY_DIR}" )

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -ggdb")

#ENABLE_TESTING
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
#include(testref/TestAgainstReference)

add_subdirectory(src)
#add_subdirectory(test/subset_internal)
add_subdirectory(test/request_tracking)
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
