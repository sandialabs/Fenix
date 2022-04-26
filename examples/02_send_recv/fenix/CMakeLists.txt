#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(fenix_ring fenix_ring.c)
target_link_libraries(fenix_ring fenix ${MPI_C_LIBRARIES} m )

if(BUILD_TESTING)
   set(CMAKE_BUILD_TYPE Debug)
   add_executable(fenix_ring-debug fenix_ring.c)
   target_link_libraries(fenix_ring-debug fenix ${MPI_C_LIBRARIES})
   add_test(NAME ring 
      COMMAND mpirun -mca mpi_ft_detector_timeout 1 -np 5 fenix_ring-debug 1 2)
   set_tests_properties(ring PROPERTIES
      FAIL_REGULAR_EXPRESSION "FAILURE") 
endif()
