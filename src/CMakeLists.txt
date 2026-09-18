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

# Headers
###############
set(Fenix_INCLUDE ${CMAKE_SOURCE_DIR}/include)
FILE(GLOB_RECURSE Fenix_HEADERS ${Fenix_INCLUDE}/*.h ${Fenix_INCLUDE}/*.hpp)
configure_file(${Fenix_INCLUDE}/fenix-config.h.in fenix-config.h @ONLY)


# Preprocessor definitions
################################
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


# Core library
####################
add_library(fenix STATIC)
target_sources(fenix PRIVATE
    fenix.cpp
    fenix_opt.cpp
    fenix_process_recovery.cpp
    fenix_util.cpp
    fenix_callbacks.cpp)
target_sources(fenix PUBLIC FILE_SET HEADERS
    BASE_DIRS ${Fenix_INCLUDE} ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${Fenix_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/fenix-config.h)


# Message logging library
####################
add_library(mlog STATIC)
add_library(fenix::mlog ALIAS mlog)
target_link_libraries(mlog INTERFACE fenix)
# Private includes for mlog library, so we don't need to install headers twice
target_sources(mlog PRIVATE FILE_SET HEADERS
    BASE_DIRS ${Fenix_INCLUDE} ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${Fenix_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/fenix-config.h)


# Component directories
#######################
add_subdirectory(data)
add_subdirectory(logging)
add_subdirectory(mpixx)


# Compile and link options
##########################
foreach(FENIX_LIB fenix mlog)
    target_compile_features(${FENIX_LIB} PUBLIC cxx_std_20)
    target_link_libraries(${FENIX_LIB} PUBLIC MPI::MPI_CXX)
    if (NOT "${Fenix_DEFINE}" STREQUAL "")
        target_compile_definitions(${FENIX_LIB} PUBLIC ${Fenix_DEFINE})
    endif()
endforeach()


# Install
#########
install(TARGETS fenix EXPORT fenix ARCHIVE LIBRARY FILE_SET HEADERS)
install(TARGETS mlog EXPORT mlog ARCHIVE LIBRARY)
install(EXPORT fenix FILE fenixTargets.cmake DESTINATION cmake)
install(EXPORT mlog FILE fenix_mlogTargets.cmake NAMESPACE fenix:: DESTINATION cmake COMPONENT mlog)
