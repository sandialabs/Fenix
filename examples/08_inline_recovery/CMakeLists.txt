#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

add_executable(stencil stencil.cpp)
target_link_libraries(stencil fenix ${MPI_C_LIBRARIES})

add_executable(stencil_barriers stencil.cpp)
target_link_libraries(stencil_barriers fenix ${MPI_C_LIBRARIES})
target_compile_definitions(
   stencil_barriers PRIVATE FENIX_STENCIL_ENABLE_BARRIERS
)

if(BUILD_TESTING) 
   add_test(NAME inline_recovery 
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery PROPERTIES LABELS "Example")
   
   add_test(NAME inline_recovery_barriers
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil_barriers ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery_barriers PROPERTIES LABELS "Example")
endif()
