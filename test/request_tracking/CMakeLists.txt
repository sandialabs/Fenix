#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(fenix_request_tracking_test fenix_request_tracking_test.c)
target_link_libraries(fenix_request_tracking_test fenix MPI::MPI_C)

add_test(NAME request_tracking 
   COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 3 ${MPIEXEC_PREFLAGS} fenix_request_tracking_test ${MPIEXEC_POSTFLAGS})
