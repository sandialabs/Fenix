#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

fenix_test(SRC comm_register_revokes_registered.cpp RANKS 5
  TESTNAME comm_register_revokes_registered)
fenix_test(SRC comm_register_skips_deregistered.cpp RANKS 5
  TESTNAME comm_register_skips_deregistered)
fenix_test(SRC comm_register_drops_freed.cpp RANKS 5
  TESTNAME comm_register_drops_freed)
fenix_test(SRC comm_register_does_not_copy_to_dup.cpp RANKS 5
  TESTNAME comm_register_does_not_copy_to_dup)