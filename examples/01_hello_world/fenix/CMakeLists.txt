#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(fenix_hello_world fenix_hello_world.c)
target_link_libraries(fenix_hello_world fenix ${MPI_C_LIBRARIES})
set_target_properties(fenix_hello_world PROPERTIES LINKER_LANGUAGE C)

if(BUILD_TESTING)
   add_test(NAME hello_world 
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 3 ${MPIEXEC_PREFLAGS} fenix_hello_world ${MPIEXEC_POSTFLAGS} "1")
endif()
