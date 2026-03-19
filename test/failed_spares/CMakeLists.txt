#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

fenix_test(SRC fenix_failed_spares.c RANKS 6 ARGS 3 1 3 4)

find_package(Threads REQUIRED)
target_link_libraries(fenix_failed_spares Threads::Threads)
