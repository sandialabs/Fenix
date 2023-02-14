#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

cmake_minimum_required(VERSION 3.10.2)

project(Fenix C)
# The version number.
set(FENIX_VERSION_MAJOR 1)
set(FENIX_VERSION_MINOR 0)

option(BUILD_EXAMPLES  "Builds example programs from the examples directory"   OFF)
option(BUILD_TESTING   "Builds tests and test modes of files"                  OFF)

#Solves an issue with some system environments putting their MPI headers before
#the headers CMake includes.
option(CRAYPE_INC_FIX  "Adds detected MPI headers directly to this project"    ON)

find_package(MPI REQUIRED)

add_subdirectory(src)


include(CTest)
list(APPEND MPIEXEC_PREFLAGS "--with-ft;mpi")

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/fenix-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/fenix-config.h @ONLY
)

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
