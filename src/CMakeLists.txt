#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and  Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

configure_file (${CMAKE_SOURCE_DIR}/include/fenix-config.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/fenix-config.h" @ONLY)

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
FILE(GLOB Fenix_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h*)

set (Fenix_SOURCES 
    fenix.cpp
    fenix_opt.cpp
    fenix_process_recovery.cpp
    fenix_util.cpp
    fenix_data_recovery.cpp
    fenix_data_group.cpp
    fenix_data_policy.cpp
    fenix_data_policy_in_memory_raid.cpp
    fenix_data_member.cpp
    fenix_data_subset.cpp
    fenix_callbacks.cpp
    globals.cpp
)

add_library(fenix STATIC ${Fenix_SOURCES})

target_compile_features(fenix PRIVATE cxx_std_17)

target_link_libraries(fenix PUBLIC MPI::MPI_CXX)

target_include_directories(fenix
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE "${CMAKE_SOURCE_DIR}/src")

install(TARGETS fenix
    EXPORT fenix
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)
install(EXPORT fenix
    FILE fenixTargets.cmake
    DESTINATION cmake)
install(FILES ${Fenix_HEADERS} DESTINATION include)
