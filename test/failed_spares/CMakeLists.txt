#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

#set(CMAKE_BUILD_TYPE Debug)
add_executable(fenix_failed_spares fenix_failed_spares.c)
target_link_libraries(fenix_failed_spares fenix ${MPI_C_LIBRARIES})
add_test(NAME failed_spares 
  COMMAND mpirun --enable-recovery --omca mpi_ft_enable true -n 6 fenix_failed_spares 3 1 3 4 )
