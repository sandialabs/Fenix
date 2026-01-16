#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#
add_executable(fenix_subset_includes subset_includes.cpp)
target_link_libraries(fenix_subset_includes fenix)
add_test(subset_includes fenix_subset_includes)

add_executable(fenix_subset_subtraction subset_subtraction.cpp)
target_link_libraries(fenix_subset_subtraction fenix)
add_test(subset_subtraction fenix_subset_subtraction)

add_executable(fenix_subset_copy subset_copy.cpp)
target_link_libraries(fenix_subset_copy fenix)
add_test(subset_copy fenix_subset_copy)

add_executable(fenix_subset_addition subset_addition.cpp)
target_link_libraries(fenix_subset_addition fenix)
add_test(subset_addition fenix_subset_addition)
