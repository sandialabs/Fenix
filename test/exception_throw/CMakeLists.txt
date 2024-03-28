#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(fenix_exceptions fenix_exceptions.cpp)
target_link_libraries(fenix_exceptions fenix MPI::MPI_CXX)

add_test(NAME exception_throw 
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} fenix_exceptions ${MPIEXEC_POSTFLAGS})
