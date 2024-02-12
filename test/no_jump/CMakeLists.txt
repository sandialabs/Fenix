#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(fenix_no_jump_test fenix_no_jump_test.c)
target_link_libraries(fenix_no_jump_test fenix MPI::MPI_C)

add_test(NAME no_jump COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 5 ${MPIEXEC_PREFLAGS} fenix_no_jump_test ${MPIEXEC_POSTFLAGS} "1")
