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

option(FENIX_EXAMPLES    "Builds example programs from the examples directory" OFF)
option(FENIX_TESTS       "Builds tests and test modes of files"                OFF)

#Solves an issue with some system environments putting their MPI headers before
#the headers CMake includes. Forces non-system MPI headers when incorrect headers
#detected in include path.
option(SYSTEM_INC_FIX  "Attempts to force overriding any system MPI headers" ON)


find_package(MPI REQUIRED)

#If we're using mpicc, we don't need to worry about the includes.
if("${CMAKE_C_COMPILER}" MATCHES ".*/?mpicc")
  set(SYSTEM_INC_FIX OFF)
endif()

if(${SYSTEM_INC_FIX})
  include(CheckIncludeFile)
  set(CMAKE_REQUIRED_QUIET ON)
  check_include_file("mpi.h" MPI_HEADER_CRASH)
  set(CMAKE_REQUIRED_QUIET OFF)
  
  if(${MPI_HEADER_CRASH})
    message(WARNING "Detected system MPI headers, attempting to force use of ${MPI_C_INCLUDE_DIRS}. Set SYSTEM_INC_FIX=OFF to stop this behavior.")

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.25")
      set_target_properties(MPI::MPI_C PROPERTIES SYSTEM "FALSE")
    else()
      foreach(MPI_DIR ${MPI_C_INCLUDE_DIRS})
        if(NOT ("${CMAKE_C_FLAGS}" MATCHES ".* -I${MPI_DIR}.*"))
          message(STATUS ${MPI_DIR})
          set(CMAKE_C_FLAGS " -I${MPI_DIR} ${CMAKE_C_FLAGS}" CACHE INTERNAL "Flags to append to build commands for C.")
        endif()
      endforeach()
      set_target_properties(MPI::MPI_C PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
    endif()
  endif()

endif() #SYSTEM_INC_FIX



add_subdirectory(src)

include(CTest)
list(APPEND MPIEXEC_PREFLAGS "--with-ft;mpi")

if(FENIX_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FENIX_TESTS)
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
