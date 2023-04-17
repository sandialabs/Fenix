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

if(FENIX_TESTS)
   add_test(NAME ring 
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 5 ${MPIEXEC_PREFLAGS} fenix_ring ${MPIEXEC_POSTFLAGS} 1 2)
   set_tests_properties(ring PROPERTIES
      FAIL_REGULAR_EXPRESSION "FAILURE") 
endif()
