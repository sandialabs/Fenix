#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and  Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(test_mpixx_comm test_comm.cpp)
target_link_libraries(test_mpixx_comm PRIVATE fenix MPI::MPI_CXX)

add_executable(test_mpixx_datatype test_datatype.cpp)
target_link_libraries(test_mpixx_datatype PRIVATE fenix MPI::MPI_CXX)

if (BUILD_TESTING)
    add_test(NAME test_mpixx_comm COMMAND ${MPIEXEC} ${MPIEXEC_PREFLAGS}
             ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_POSTFLAGS} $<TARGET_FILE:test_mpixx_comm>)
    set_tests_properties(test_mpixx_comm PROPERTIES TIMEOUT 20)

    add_test(NAME test_mpixx_datatype COMMAND ${MPIEXEC} ${MPIEXEC_PREFLAGS}
             ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_POSTFLAGS} $<TARGET_FILE:test_mpixx_datatype>)
    set_tests_properties(test_mpixx_datatype PROPERTIES TIMEOUT 20)
endif()
