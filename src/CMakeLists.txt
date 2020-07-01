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

#configure_file(${CMAKE_SOURCE_DIR}/include/fenix.h
#  "${CMAKE_BINARY_DIR}/include/fenix.h" COPYONLY)

#configure_file(${CMAKE_SOURCE_DIR}/include/fenix_process_recovery.h
#  "${CMAKE_BINARY_DIR}/include/fenix_process_recovery.h" COPYONLY)

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
FILE(GLOB Fenix_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)

set (Fenix_SOURCES 
fenix.c
fenix_mpi_override.c
fenix_opt.c
fenix_process_recovery.c
fenix_util.c
fenix_data_recovery.c
fenix_data_group.c
fenix_data_policy.c
fenix_data_policy_in_memory_raid.c
fenix_data_member.c
fenix_data_subset.c
fenix_comm_list.c
fenix_callbacks.c
globals.c
)

add_library( fenix STATIC ${Fenix_SOURCES})

#if("a$ENV{MPICC}" STREQUAL "a")
#       message("[fenix] MPICC (MPI compiler) environment variable is not defined. Trying to find MPI compiler...")
#       find_package(MPI REQUIRED)
#       target_link_libraries(fenix MPI::MPI_C)
#else()
#       message("[fenix] MPICC has been passed: $ENV{MPICC}")
#       set(MPI_C_COMPILER $ENV{MPICC})
#       SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
#endif()

linkMPI(fenix)

target_link_libraries(fenix ${MPI_C_LIBRARIES})
if(MPI_COMPILE_FLAGS)
    set_target_properties(fenix PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()
if(MPI_LINK_FLAGS)
    set_target_properties(fenix PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

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

#target_link_libraries( mpi  )
