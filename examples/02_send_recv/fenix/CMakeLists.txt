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
target_link_libraries(fenix_ring fenix ${MPI_C_LIBRARIES})
set_target_properties(fenix_ring PROPERTIES LINKER_LANGUAGE C)

fenix_test(EXAMPLE EXE fenix_ring RANKS 5 ARGS 1 2 FAIL_OUTPUT "FAILURE")
