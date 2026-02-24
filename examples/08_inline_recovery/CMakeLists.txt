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
target_compile_definitions(stencil_barriers PRIVATE
   FENIX_STENCIL_ENABLE_BARRIERS)

add_executable(stencil_bcasts stencil.cpp)
target_link_libraries(stencil_bcasts fenix ${MPI_C_LIBRARIES})
target_compile_definitions(stencil_bcasts PRIVATE
   FENIX_STENCIL_ENABLE_BCASTS)

add_executable(stencil_reduces stencil.cpp)
target_link_libraries(stencil_reduces fenix ${MPI_C_LIBRARIES})
target_compile_definitions(stencil_reduces PRIVATE
   FENIX_STENCIL_ENABLE_REDUCES)

add_executable(stencil_allreduces stencil.cpp)
target_link_libraries(stencil_allreduces fenix ${MPI_C_LIBRARIES})
target_compile_definitions(stencil_allreduces PRIVATE
   FENIX_STENCIL_ENABLE_ALLREDUCES)

add_executable(stencil_everything stencil.cpp)
target_link_libraries(stencil_everything fenix ${MPI_C_LIBRARIES})
target_compile_definitions(stencil_everything PRIVATE
   FENIX_STENCIL_ENABLE_BARRIERS FENIX_STENCIL_ENABLE_BCASTS
   FENIX_STENCIL_ENABLE_REDUCES  FENIX_STENCIL_ENABLE_ALLREDUCES)

if(BUILD_TESTING) 
   add_test(NAME inline_recovery 
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery PROPERTIES LABELS "Example")
   
   add_test(NAME inline_recovery_barriers
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil_barriers ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery_barriers PROPERTIES LABELS "Example")
   
   add_test(NAME inline_recovery_bcasts
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil_bcasts ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery_bcasts PROPERTIES LABELS "Example")
   
   add_test(NAME inline_recovery_reduces
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil_reduces ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery_reduces PROPERTIES LABELS "Example")
   
   add_test(NAME inline_recovery_allreduces
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil_allreduces ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery_allreduces PROPERTIES LABELS "Example")
   
   add_test(NAME inline_recovery_everything
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_PREFLAGS} stencil_everything ${MPIEXEC_POSTFLAGS})
   set_tests_properties(inline_recovery_everything PROPERTIES LABELS "Example")
endif()
