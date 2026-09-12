#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

# Test 1: 3 failures, 1 spare -> should shrink by 2
fenix_test(SRC fenix_shrink_test.cpp RANKS 7 ARGS 1 1 2 3
  TESTNAME shrink_more_failures_than_spares)

# Test 2: 2 failures, 0 spares -> should shrink by 2
fenix_test(SRC fenix_shrink_test.cpp RANKS 6 ARGS 0 1 3
  TESTNAME shrink_no_spares)

# Test 3: 4 failures, 2 spares -> should shrink by 2
fenix_test(SRC fenix_shrink_test.cpp RANKS 8 ARGS 2 0 2 4 5
  TESTNAME shrink_multiple_unrecovered)

# Test 4: Early failure before Fenix_Init, 2 failures with 1 spare -> shrink by 1
fenix_test(SRC fenix_early_failure_test.cpp RANKS 6 ARGS 1 1 2
  TESTNAME shrink_early_failure_with_spare)

# Test 5: Early failure before Fenix_Init, 2 failures with 0 spares -> shrink by 2
fenix_test(SRC fenix_early_failure_test.cpp RANKS 5 ARGS 0 1 3
  TESTNAME shrink_early_failure_no_spare)

# Link pthread to tests that use threading (not early_failure tests)
find_package(Threads REQUIRED)
foreach(test_target shrink_more_failures_than_spares shrink_no_spares shrink_multiple_unrecovered)
  if(TARGET ${test_target})
    target_link_libraries(${test_target} Threads::Threads)
  endif()
endforeach()
