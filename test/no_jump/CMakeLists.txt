#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

set(CMAKE_BUILD_TYPE Debug)
add_executable(fenix_no_jump_test fenix_no_jump_test.c)
target_link_libraries(fenix_no_jump_test fenix ${MPI_C_LIBRARIES})

add_test(NAME no_jump COMMAND mpirun -mca mpi_ft_detector_timeout 1 -np 5 fenix_no_jump_test "1")
