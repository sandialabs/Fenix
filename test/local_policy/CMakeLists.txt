#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

fenix_test(SRC local_basic.cpp)
fenix_test(SRC local_basic.cpp RANKS 6 TESTNAME local_basic_multirank)
fenix_test(SRC local_depth.cpp RANKS 6)
fenix_test(SRC local_stage.cpp RANKS 6)
fenix_test(SRC local_resizeable.cpp RANKS 6)
fenix_test(SRC local_recovery.cpp RANKS 6)
