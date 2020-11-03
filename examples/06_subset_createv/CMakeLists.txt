#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(subset_createv subset_createv.c)
target_link_libraries(subset_createv fenix ${MPI_C_LIBRARIES})

if(BUILD_TESTING) 
   set(CMAKE_BUILD_TYPE Debug)
   add_executable(fenix_subset_createv-debug subset_createv.c)
   target_link_libraries(fenix_subset_createv-debug fenix ${MPI_C_LIBRARIES})
   add_test(NAME subset_createv 
      COMMAND mpirun -mca mpi_ft_detector_timeout 1 -np 5 fenix_subset_createv-debug 1)
   set_tests_properties(subset_createv PROPERTIES
      FAIL_REGULAR_EXPRESSION "FAILURE") 
endif()
