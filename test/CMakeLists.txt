# Any arguments to the executable should be listed at the end
function(fenix_mpi_test sourcefile ranks)
  get_filename_component(name ${sourcefile} NAME_WLE)
  set(exe ${name}_exe) 
  add_executable(${exe} ${sourcefile})
  target_link_libraries(${exe} fenix MPI::MPI_CXX)
  add_test(NAME ${name} COMMAND
    ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${ranks} ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:${exe}> ${MPIEXEC_POSTFLAGS} ${ARGN}
  )
endfunction()

add_subdirectory(subset)
add_subdirectory(request_tracking)
add_subdirectory(request_cancelled)
add_subdirectory(no_jump)
add_subdirectory(issend)
add_subdirectory(failed_spares)
add_subdirectory(exception_throw)
add_subdirectory(storev)
add_subdirectory(stage)
add_subdirectory(tasks)
