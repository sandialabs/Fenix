find_package(Doxygen)

set(FENIX_DOCS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Documentation output directory")
set(FENIX_DOCS_MAN "YES" CACHE BOOL "Option to disable man page generation for CI builds")
set(FENIX_BRANCH "local" CACHE BOOL "Git branch being documented, or local if not building for Github Pages")

if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen not found, `make docs` disabled")
    return()
endif()

list(APPEND DOXYGEN_EXAMPLE_PATH markdown)
list(APPEND DOXYGEN_IMAGE_PATH images)

set(DOXYGEN_USE_MDFILE_AS_MAINPAGE markdown/Introduction.md)
set(DOXYGEN_LAYOUT_FILE DoxygenLayout.xml)
set(DOXYGEN_OUTPUT_DIRECTORY ${FENIX_DOCS_OUTPUT})

set(DOXYGEN_GENERATE_MAN ${FENIX_DOCS_MAN})

set(DOXYGEN_QUIET YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
set(DOXYGEN_WARN_IF_DOC_ERROR YES)
set(DOXYGEN_WARN_NO_PARAMDOC YES)
set(DOXYGEN_SHOW_INCLUDE_FILES NO)
set(DOXYGEN_WARN_IF_UNDOC_ENUM_VAL NO)

list(APPEND DOXYGEN_ALIASES "returnstatus=@return FENIX_SUCCESS if successful, any [return code](@ref ReturnCodes) otherwise.")
list(APPEND DOXYGEN_ALIASES "unimplemented=@qualifier UNIMPLEMENTED @brief @htmlonly <span class=\\\"mlabel\\\"> @endhtmlonly UNIMPLEMENTED @htmlonly </span> @endhtmlonly")

add_subdirectory(html)

doxygen_add_docs(docs
    markdown/Introduction.md fake_init.h ../include ../src
    ALL
    COMMENT "Generate Fenix documentation")
message(STATUS "Run `make docs` to build documentation")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man DESTINATION ${CMAKE_INSTALL_PREFIX})
