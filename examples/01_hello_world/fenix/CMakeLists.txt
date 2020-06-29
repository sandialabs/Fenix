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

if(BUILD_TESTING)
   set(CMAKE_BUILD_TYPE Debug)
   add_executable(fenix_hello_world-debug fenix_hello_world.c)
   target_link_libraries(fenix_hello_world-debug fenix ${MPI_C_LIBRARIES})
   add_test(NAME hello_world 
      COMMAND mpirun -mca mpi_ft_detector_timeout 1 -np 3 fenix_hello_world-debug "1")
endif()
