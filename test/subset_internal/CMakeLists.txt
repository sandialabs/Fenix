#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top DHARMA
#  directory.
#

add_executable(fenix_subset_internal_test fenix_subset_internal_test.c)
target_link_libraries(fenix_subset_internal_test fenix)
