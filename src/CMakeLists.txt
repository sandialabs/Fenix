#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and  Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

set(Fenix_SOURCES
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
    fenix_data_buffer.cpp
    fenix_callbacks.cpp)
add_library(fenix STATIC ${Fenix_SOURCES})

set(Fenix_INCLUDE ${CMAKE_SOURCE_DIR}/include)
FILE(GLOB_RECURSE Fenix_HEADERS ${Fenix_INCLUDE}/*.h ${Fenix_INCLUDE}/*.hpp)
target_sources(fenix PUBLIC FILE_SET HEADERS
    BASE_DIRS ${Fenix_INCLUDE} FILES ${Fenix_HEADERS})

target_compile_features(fenix PUBLIC cxx_std_20)
target_link_libraries(fenix PUBLIC MPI::MPI_CXX)

if(FENIX_C_CATCH_RUNTIME_EXCEPTIONS)
    target_compile_definitions(fenix PUBLIC FENIX_C_CATCH_RUNTIME_EXCEPTIONS)
endif()
if(FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS)
    target_compile_definitions(fenix PUBLIC FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS)
endif()

configure_file(${Fenix_INCLUDE}/fenix-config.h.in fenix-config.h @ONLY)

install(TARGETS fenix EXPORT fenix ARCHIVE LIBRARY FILE_SET HEADERS)
install(EXPORT fenix FILE fenixTargets.cmake DESTINATION cmake)
