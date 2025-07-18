#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(resizeable resizeable.cpp)
target_link_libraries(resizeable fenix ${MPI_C_LIBRARIES})

target_compile_features(resizeable PRIVATE cxx_std_20)

if(BUILD_TESTING) 
   add_test(NAME resizeable 
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 5 ${MPIEXEC_PREFLAGS} resizeable ${MPIEXEC_POSTFLAGS} 1)
   set_tests_properties(resizeable PROPERTIES
      FAIL_REGULAR_EXPRESSION "FAILURE" LABELS "Example") 
endif()
