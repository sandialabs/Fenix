#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and  Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top DHARMA
#  directory.
#
configure_file (fenix-config.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/fenix-config.h" @ONLY)

set (Fenix_SOURCES 
hash.c
process_recovery.c
meta_data_recovery.c
data_recovery.c
fenix.c
mpi_override.c
util.c
opt.c
)

add_library( fenix STATIC ${Fenix_SOURCES}
  ${CMAKE_CURRENT_BINARY_DIR}/fenix-config.h)
#target_link_libraries( mpi  )
