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

# Try to find sphinx-build, fall back to python -m sphinx
find_program(SPHINX_BUILD sphinx-build)

if(NOT SPHINX_BUILD)
    # Try using Python module directly
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -m sphinx --version
            RESULT_VARIABLE SPHINX_MODULE_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
        if(SPHINX_MODULE_RESULT EQUAL 0)
            set(SPHINX_BUILD ${Python3_EXECUTABLE} -m sphinx)
            message(STATUS "Using Python module: python3 -m sphinx")
        endif()
    endif()
endif()

set(FENIX_DOCS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Documentation output directory")
set(FENIX_BRANCH "local" CACHE STRING "Git branch being documented, or local if not building for Github Pages")

if(NOT SPHINX_BUILD)
    message(STATUS "Sphinx not found, `make docs` disabled")
    message(STATUS "Install with: pip install sphinx sphinx_rtd_theme")
    message(STATUS "Or set PYTHONPATH to include sphinx installation")
    return()
endif()

set(SPHINX_SOURCE_DIR ${CMAKE_SOURCE_DIR}/docs)
set(SPHINX_BUILD_DIR ${FENIX_DOCS_OUTPUT}/sphinx)
set(SPHINX_HTML_DIR ${SPHINX_BUILD_DIR}/html)

# Collect all documentation source files for dependency tracking
# Always use the source directory for both tracking and building
file(GLOB_RECURSE SPHINX_SOURCE_FILES
    ${SPHINX_SOURCE_DIR}/*.rst
    ${SPHINX_SOURCE_DIR}/*.md
    ${SPHINX_SOURCE_DIR}/conf.py
    ${SPHINX_SOURCE_DIR}/_static/*
    ${SPHINX_SOURCE_DIR}/_templates/*
)

# Create a timestamp file that Sphinx updates on each build
set(SPHINX_TIMESTAMP ${SPHINX_BUILD_DIR}/.sphinx_timestamp)

# Build directly from source directory, not a copy
# This ensures changes are always picked up
add_custom_command(
    OUTPUT ${SPHINX_TIMESTAMP}
    COMMAND ${SPHINX_BUILD}
        -b html
        -W
        -d ${SPHINX_BUILD_DIR}/doctrees
        ${SPHINX_SOURCE_DIR}
        ${SPHINX_HTML_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${SPHINX_TIMESTAMP}
    DEPENDS ${SPHINX_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building Sphinx HTML documentation"
    VERBATIM
)

add_custom_target(docs ALL
    DEPENDS ${SPHINX_TIMESTAMP}
)

message(STATUS "Run `make docs` to build Sphinx documentation")
message(STATUS "Output will be in: ${SPHINX_HTML_DIR}/index.html")

# Optional: Add man page target
# Use SPHINX_SOURCE_DIR which always points to the source directory
set(SPHINX_MAN_TIMESTAMP ${SPHINX_BUILD_DIR}/.sphinx_man_timestamp)

add_custom_command(
    OUTPUT ${SPHINX_MAN_TIMESTAMP}
    COMMAND ${SPHINX_BUILD}
        -b man
        -d ${SPHINX_BUILD_DIR}/doctrees
        ${SPHINX_SOURCE_DIR}
        ${SPHINX_BUILD_DIR}/man
    COMMAND ${CMAKE_COMMAND} -E touch ${SPHINX_MAN_TIMESTAMP}
    DEPENDS ${SPHINX_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building Sphinx man pages"
    VERBATIM
)

add_custom_target(man
    DEPENDS ${SPHINX_MAN_TIMESTAMP}
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
