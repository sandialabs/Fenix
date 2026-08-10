#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

cmake_minimum_required(VERSION 3.23)

find_program(SPHINX_BUILD sphinx-build)

set(FENIX_DOCS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Documentation output directory")
set(FENIX_BRANCH "local" CACHE STRING "Git branch being documented, or local if not building for Github Pages")

if(NOT SPHINX_BUILD)
    message(STATUS "Sphinx not found, `make docs` disabled")
    message(STATUS "Install with: pip install sphinx sphinx_rtd_theme")
    return()
endif()

set(SPHINX_SOURCE_DIR ${CMAKE_SOURCE_DIR}/docs)
set(SPHINX_BUILD_DIR ${FENIX_DOCS_OUTPUT}/sphinx)
set(SPHINX_HTML_DIR ${SPHINX_BUILD_DIR}/html)

# Copy docs directory to build directory if building out-of-tree
if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    file(COPY ${SPHINX_SOURCE_DIR}
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
         PATTERN "_build" EXCLUDE
         PATTERN "*.pyc" EXCLUDE
         PATTERN "__pycache__" EXCLUDE)
    set(SPHINX_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
endif()

add_custom_target(docs
    COMMAND ${SPHINX_BUILD}
        -b html
        -W
        -d ${SPHINX_BUILD_DIR}/doctrees
        ${SPHINX_SOURCE_DIR}
        ${SPHINX_HTML_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building Sphinx HTML documentation"
    VERBATIM
)

message(STATUS "Run `make docs` to build Sphinx documentation")
message(STATUS "Output will be in: ${SPHINX_HTML_DIR}/index.html")

# Optional: Add man page target
add_custom_target(man
    COMMAND ${SPHINX_BUILD}
        -b man
        -d ${SPHINX_BUILD_DIR}/doctrees
        ${SPHINX_SOURCE_DIR}
        ${SPHINX_BUILD_DIR}/man
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building Sphinx man pages"
    VERBATIM
)

# Install HTML documentation
install(DIRECTORY ${SPHINX_HTML_DIR}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/fenix
        OPTIONAL)

# Install man pages if generated
install(DIRECTORY ${SPHINX_BUILD_DIR}/man/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man3
        OPTIONAL
        FILES_MATCHING PATTERN "*.3")
