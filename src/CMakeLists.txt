#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and  Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

include(CheckSymbolExists)

set(Fenix_SOURCES
    fenix.cpp
    fenix_opt.cpp
    fenix_process_recovery.cpp
    fenix_util.cpp
    fenix_data_recovery.cpp
    fenix_data_component.cpp
    fenix_data_group.cpp
    fenix_data_policy_in_memory_raid.cpp
    fenix_data_member.cpp
    fenix_data_snapshot.cpp
    fenix_data_subset.cpp
    fenix_data_buffer.cpp
    fenix_callbacks.cpp)
add_library(fenix STATIC ${Fenix_SOURCES})

add_library(mlog STATIC)
add_library(Fenix::mlog ALIAS mlog)
target_link_libraries(mlog INTERFACE fenix)

add_subdirectory(data)
add_subdirectory(logging)

set(Fenix_INCLUDE ${CMAKE_SOURCE_DIR}/include)
FILE(GLOB_RECURSE Fenix_HEADERS ${Fenix_INCLUDE}/*.h ${Fenix_INCLUDE}/*.hpp)

configure_file(${Fenix_INCLUDE}/fenix-config.h.in fenix-config.h @ONLY)

# Set preprocessor definitions to use
set(Fenix_DEFINE "")
if (FENIX_C_CATCH_RUNTIME_EXCEPTIONS)
    set(Fenix_DEFINE "${Fenix_DEFINE} FENIX_C_CATCH_RUNTIME_EXCEPTIONS")
endif()
if (FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS)
    set(Fenix_DEFINE "${Fenix_DEFINE} FENIX_CPP_CATCH_RUNTIME_EXCEPTIONS")
endif()

list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists(mremap "sys/mman.h" FENIX_HAVE_MREMAP)
if (FENIX_HAVE_MREMAP)
    set(Fenix_DEFINE "${Fenix_DEFINE} FENIX_HAVE_MREMAP")
endif()

foreach(FENIX_LIB fenix mlog)
    target_sources(${FENIX_LIB} PUBLIC FILE_SET HEADERS
        BASE_DIRS ${Fenix_INCLUDE} ${CMAKE_CURRENT_BINARY_DIR}
        FILES ${Fenix_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/fenix-config.h)

    target_compile_features(${FENIX_LIB} PUBLIC cxx_std_20)
    target_link_libraries(${FENIX_LIB} PUBLIC MPI::MPI_CXX)
    if (NOT "${Fenix_DEFINE}" STREQUAL "")
        target_compile_definitions(${FENIX_LIB} PUBLIC ${Fenix_DEFINE})
    endif()
endforeach()

install(TARGETS fenix EXPORT fenix ARCHIVE LIBRARY FILE_SET HEADERS)
install(TARGETS mlog EXPORT fenix_modules ARCHIVE LIBRARY FILE_SET HEADERS)
install(EXPORT fenix FILE fenixTargets.cmake DESTINATION cmake)
install(EXPORT fenix_modules FILE fenixModuleTargets.cmake NAMESPACE Fenix DESTINATION cmake)
