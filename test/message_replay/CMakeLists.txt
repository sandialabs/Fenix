#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(fenix_message_logging_test fenix_message_logging_test.cxx)
target_link_libraries(fenix_message_logging_test fenix MPI::MPI_CXX)

add_test(NAME request_cancelled COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 5 ${MPIEXEC_PREFLAGS} fenix_message_logging_test ${MPIEXEC_POSTFLAGS} "1")
