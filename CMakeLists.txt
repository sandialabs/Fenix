#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

cmake_minimum_required(VERSION 3.0.2)

project(Fenix)
# The version number.
set(FENIX_VERSION_MAJOR 1)
set(FENIX_VERSION_MINOR 0)

if("a$ENV{MPICC}" STREQUAL "a")
       message("[fenix] MPICC (MPI compiler) environment variable is not defined. Trying to find MPI compiler...")
       find_package(MPI REQUIRED)
       if(${MPI_C_FOUND})
              message("[fenix] MPI found! ${MPI_C_COMPILER}")
       else()
              message( FATAL_ERROR "[fenix] MPI not found :( Aborting! Export environment variable MPI_C_COMPILER and retry")
       endif()
       message("[fenix] MPI COMPILE FLAGS: ${MPI_C_COMPILE_FLAGS}")
       message("[fenix] MPI LINK FLAGS: ${MPI_C_LINK_FLAGS}")
       message("[fenix] MPI INCLUDE PATH: ${MPI_C_INCLUDE_PATH}")
       message("[fenix] MPI COMPILER: ${MPI_C_COMPILER}")
else()
       message("[fenix] MPICC has been passed: $ENV{MPICC}")
       set(MPI_C_COMPILER $ENV{MPICC})
endif()
SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})

# Set empty string for shared linking (we use static library only at this moment)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

# This is include directories
include_directories(
"${CMAKE_SOURCE_DIR}/include"
"${CMAKE_SOURCE_DIR}/src")

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
#add_subdirectory(test/request_tracking)
add_subdirectory(test/group_instantiation)
add_subdirectory(test/subset_merging)

#add_subdirectory(examples/01_hello_world/fenix)
#add_subdirectory(examples/01_hello_world/mpi)
#add_subdirectory(examples/02_send_recv/fenix)
#add_subdirectory(examples/02_send_recv/mpi)
#add_subdirectory(examples/03_reduce/fenix)
#add_subdirectory(examples/03_reduce/mpi)
#add_subdirectory(examples/04_Isend_Irecv/fenix)
#add_subdirectory(examples/04_Isend_Irecv/mpi)
#add_subdirectory(examples/05_subset_create)
#add_subdirectory(examples/06_subset_createv)
