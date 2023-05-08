#If we're using mpicc, we don't need to worry about the includes.
if("${CMAKE_C_COMPILER}" MATCHES ".*/?mpic")
    return()
endif()

include(CheckIncludeFile)
set(CMAKE_REQUIRED_QUIET ON)
check_include_file("mpi.h" MPI_HEADER_CLASH)
set(CMAKE_REQUIRED_QUIET OFF)

if(${MPI_HEADER_CLASH})
  if(TARGET fenix)
    message(WARNING "Fenix detected system MPI headers, attempting to force use of ${MPI_C_INCLUDE_DIRS}. Disable FENIX_PROPAGATE_INC_FIX to stop this behavior.")
  else()
    message(WARNING "Detected system MPI headers, attempting to force use of ${MPI_C_INCLUDE_DIRS}. Disable FENIX_SYSTEM_INC_FIX to stop this behavior.")
  endif()

  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.25")

    if(TARGET MPI::MPI_C)
      set_target_properties(MPI::MPI_C PROPERTIES SYSTEM "FALSE")
    endif()
    if(TARGET MPI::MPI_CXX)
      set_target_properties(MPI::MPI_CXX PROPERTIES SYSTEM "FALSE")
    endif()

  else()

    if(TARGET MPI::MPI_C)
      set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY INCLUDE_DIRECTORIES "${MPI_C_INCLUDE_DIRS}")
    endif()
    if(TARGET MPI::MPI_CXX)
      set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY INCLUDE_DIRECTORIES "${MPI_CXX_INCLUDE_DIRS}")
    endif()

    if(TARGET fenix)
      get_target_property(FENIX_INCLUDES fenix INTERFACE_INCLUDE_DIRECTORIES)
      list(REMOVE_ITEM FENIX_INCLUDES ${MPI_C_INCLUDE_DIRS})
      list(REMOVE_ITEM FENIX_INCLUDES ${MPI_CXX_INCLUDE_DIRS})
      set_target_properties(fenix PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FENIX_INCLUDES}")
    endif()
    
    if(TARGET MPI::MPI_C)
      set_target_properties(MPI::MPI_C PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
    endif()
    if(TARGET MPI::MPI_CXX)
      set_target_properties(MPI::MPI_CXX PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
    endif()

  endif()
endif()
