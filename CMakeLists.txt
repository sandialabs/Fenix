#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

cmake_minimum_required(VERSION 3.10.2)

project(Fenix C)
# The version number.
set(FENIX_VERSION_MAJOR 1)
set(FENIX_VERSION_MINOR 0)

option(BUILD_EXAMPLES    "Builds example programs from the examples directory" OFF)
option(BUILD_TESTING     "Builds tests and test modes of files" ON)
option(BUILD_DOCS      "Builds documentation if is doxygen found"              ON)
option(DOCS_ONLY       "Only build documentation"                              OFF)

#Solves an issue with some system environments putting their MPI headers before
#the headers CMake includes. Forces non-system MPI headers when incorrect headers
#detected in include path.
option(FENIX_SYSTEM_INC_FIX    "Attempts to force overriding any system MPI headers" ON)
option(FENIX_PROPAGATE_INC_FIX "Attempt overriding system MPI headers in linking projects" ON)


if(NOT DOCS_ONLY)
    find_package(MPI REQUIRED)

    if(${FENIX_SYSTEM_INC_FIX})
      include(cmake/systemMPIOverride.cmake)
    endif()

    add_subdirectory(src)

    include(CTest)
    list(APPEND MPIEXEC_PREFLAGS "--with-ft;mpi")

    if(BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()

    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()

endif()

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        list(APPEND DOXYGEN_EXAMPLE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/examples)
        list(APPEND DOXYGEN_EXAMPLE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/doc/examples)
        list(APPEND DOXYGEN_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/doc/img)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE doc/Introduction.md)
        set(DOXYGEN_TOC_INCLUDE_HEADINGS 0)
        set(DOXYGEN_DISABLE_INDEX YES)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_FULL_SIDEBAR NO)
        set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/doc/DoxygenStyle.css)
        set(DOXYGEN_LAYOUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/doc/DoxygenLayout.xml)
        set(DOXYGEN_OUTPUT_DIRECTORY doc)

        set(DOXYGEN_GENERATE_MAN YES)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX YES)

        set(DOXYGEN_QUIET YES)
        set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
        set(DOXYGEN_WARN_IF_DOC_ERROR YES)
        set(DOXYGEN_WARN_NO_PARAMDOC YES)
        set(DOXYGEN_SHOW_INCLUDE_FILES NO)
        set(DOXYGEN_WARN_IF_UNDOC_ENUM_VAL NO)
        list(APPEND DOXYGEN_ALIASES "returnstatus=@return FENIX_SUCCESS if successful, any [return code](@ref ReturnCodes) otherwise.")
        list(APPEND DOXYGEN_ALIASES "unimplemented=@qualifier UNIMPLEMENTED @brief @htmlonly <span class=\\\"mlabel\\\"> @endhtmlonly UNIMPLEMENTED @htmlonly </span> @endhtmlonly")

        doxygen_add_docs(doc 
            doc/Introduction.md doc/fake_init.h include src
            ALL
            COMMENT "Generate Fenix documentation")
        message(STATUS "Run `make doc` to build documentation")
        
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc DESTINATION ${CMAKE_INSTALL_PREFIX})

    else()
        message(STATUS "Doxygen not found, `make docs` disabled")
    endif()
endif()


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/fenix-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/fenix-config.h @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/systemMPIOverride.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/systemMPIOverride.cmake COPYONLY
)


include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/fenixConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/cmake/fenixConfig.cmake
                              INSTALL_DESTINATION cmake)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/cmake/fenixConfigVersion.cmake
                                 VERSION "${FENIX_VERSION_MAJOR}.${FENIX_VERSION_MINOR}"
                                 COMPATIBILITY SameMajorVersion)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/fenixConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/systemMPIOverride.cmake
    DESTINATION cmake
)
