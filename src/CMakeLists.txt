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

configure_file(${CMAKE_SOURCE_DIR}/include/fenix.h
  "${CMAKE_BINARY_DIR}/include/fenix.h" COPYONLY)

configure_file(${CMAKE_SOURCE_DIR}/include/fenix_process_recovery.h
  "${CMAKE_BINARY_DIR}/include/fenix_process_recovery.h" COPYONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
fenix_data_version.c
fenix_data_buffer.c
fenix_comm_list.c
fenix_callbacks.c
fenix_request_store.c
globals.c
)

add_library( fenix STATIC ${Fenix_SOURCES}
  ${CMAKE_SOURCE_DIR}/include/fenix-config.h
  ${CMAKE_SOURCE_DIR}/include/fenix.h
  )
#target_link_libraries( mpi  )
