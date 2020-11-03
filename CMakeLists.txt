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

project(Fenix C)
# The version number.
set(FENIX_VERSION_MAJOR 1)
set(FENIX_VERSION_MINOR 0)

option(BUILD_EXAMPLES  "Builds example programs from the examples directory"   OFF)
option(BUILD_TESTING   "Builds tests and test modes of files"                  ON)


# Set empty string for shared linking (we use static library only at this moment)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -ggdb")

#ENABLE_TESTING
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
#include(testref/TestAgainstReference)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/fenix-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/fenix-config.h @ONLY
)


#Check for MPICC definition, if not try to find MPI
if(NOT "a$ENV{MPICC}" STREQUAL "a")
    #set(CMAKE_C_COMPILER ${MPI_C_COMPILER} CACHE STRING "The compiler CMake should use - often set to mpicc" FORCE)
    set(MPI_C_COMPILER $ENV{MPICC})
    set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
    
    message("[fenix] MPICC has been passed: $ENV{MPICC}")
else()
    message("[fenix] MPICC was not passed, searching for MPI")
    find_package(MPI REQUIRED)
    if(${MPI_C_FOUND})
        message("[fenix] Found MPICC: ${MPI_C_COMPILER}")
    else()
        message( FATAL_ERROR "[fenix] MPI not found :( Aborting!")
    endif()
endif()

#Helper function for linking with MPI only if needed
function(linkMPI TOLINK)
    #We only want to try to find MPI outrselves if it wasn't provided in MPICC by user
    if("a$ENV{MPICC}" STREQUAL "a")
           #find_package(MPI REQUIRED)
           target_link_libraries(${TOLINK} MPI::MPI_C)
    endif()    
endfunction(linkMPI)

add_subdirectory(src)

include(CMakePackageConfigHelpers)
configure_package_config_file(fenixConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/fenixConfig.cmake
                              INSTALL_DESTINATION cmake)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/fenixConfigVersion.cmake
                                 VERSION "${FENIX_VERSION_MAJOR}.${FENIX_VERSION_MINOR}"
                                 COMPATIBILITY SameMajorVersion)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fenixConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fenixConfigVersion.cmake
    DESTINATION cmake
)


include(CTest)

if(BUILD_EXAMPLES)
    add_subdirectory(examples/01_hello_world/fenix)
    add_subdirectory(examples/01_hello_world/mpi)
    add_subdirectory(examples/02_send_recv/fenix)
    add_subdirectory(examples/02_send_recv/mpi)
    add_subdirectory(examples/03_reduce/fenix)
    #add_subdirectory(examples/03_reduce/mpi)
    add_subdirectory(examples/04_Isend_Irecv/fenix)
    add_subdirectory(examples/04_Isend_Irecv/mpi)
    add_subdirectory(examples/05_subset_create)
    add_subdirectory(examples/06_subset_createv)

elseif(BUILD_TESTING)
    #Some examples are useful tests as well.
    add_subdirectory(examples/01_hello_world/fenix)
    add_subdirectory(examples/02_send_recv/fenix)
    add_subdirectory(examples/03_reduce/fenix)
    add_subdirectory(examples/05_subset_create)
    add_subdirectory(examples/06_subset_createv)
endif()

if(BUILD_TESTING)
    add_subdirectory(test/subset_internal)
    add_subdirectory(test/subset_merging)
    add_subdirectory(test/request_tracking)
    add_subdirectory(test/request_cancelled)
    add_subdirectory(test/no_jump)
    add_subdirectory(test/issend)
endif()
