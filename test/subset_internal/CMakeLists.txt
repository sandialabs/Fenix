#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#
set (CMAKE_BUILD_TYPE Debug)
add_executable(fenix_subset_internal_test fenix_subset_internal_test.c)
target_link_libraries(fenix_subset_internal_test fenix)

add_test(subset_internal fenix_subset_internal_test "100" "3" "5" "7" "10")
