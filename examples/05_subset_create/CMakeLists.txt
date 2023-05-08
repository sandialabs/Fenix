#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(subset_create subset_create.c)
target_link_libraries(subset_create fenix ${MPI_C_LIBRARIES})

if(BUILD_TESTING) 
   add_test(NAME subset_create 
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 5 ${MPIEXEC_PREFLAGS} subset_create ${MPIEXEC_POSTFLAGS} 1)
   set_tests_properties(subset_create PROPERTIES
      FAIL_REGULAR_EXPRESSION "FAILURE") 
endif()
