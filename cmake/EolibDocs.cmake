# Adds the docs target, which builds the HTML API reference into <build>/docs/html with Doxygen. The same
# configuration (docs/Doxyfile.in) is used by scripts/build-docs.sh, which builds the docs published for each release.
#
# scripts/install-deps --docs installs the Doxygen version used by CI into .tools/bin, which takes precedence.

find_program(
    EOLIB_DOXYGEN_EXECUTABLE
    NAMES doxygen
    HINTS "${PROJECT_SOURCE_DIR}/.tools/bin"
    DOC "Doxygen executable")
if(NOT EOLIB_DOXYGEN_EXECUTABLE)
    message(FATAL_ERROR "EOLIB_BUILD_DOCS requires Doxygen. Install it with scripts/install-deps.sh --docs "
                        "(or install-deps.ps1 -Docs), or set EOLIB_DOXYGEN_EXECUTABLE.")
endif()

set(EOLIB_DOCS_VERSION "${EOLIB_VERSION_FULL}")
set(EOLIB_DOCS_CONFIG_DIR "${PROJECT_SOURCE_DIR}/docs")
set(EOLIB_DOCS_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
set(EOLIB_DOCS_GENERATED_DIR "${EOLIB_GENERATED_INCLUDE_DIR}")
set(EOLIB_DOCS_WORK_DIR "${PROJECT_BINARY_DIR}/docs")
set(EOLIB_DOCS_OUTPUT_DIR "${EOLIB_DOCS_WORK_DIR}/html")
set(EOLIB_DOCS_MAINPAGE "${EOLIB_DOCS_WORK_DIR}/index.md")
set(EOLIB_DOCS_WARN_AS_ERROR NO)

configure_file("${EOLIB_DOCS_CONFIG_DIR}/Doxyfile.in" "${EOLIB_DOCS_WORK_DIR}/Doxyfile" @ONLY)
configure_file("${EOLIB_DOCS_CONFIG_DIR}/index.md.in" "${EOLIB_DOCS_MAINPAGE}" @ONLY)

add_custom_target(
    docs
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${EOLIB_DOCS_OUTPUT_DIR}"
    COMMAND "${EOLIB_DOXYGEN_EXECUTABLE}" "${EOLIB_DOCS_WORK_DIR}/Doxyfile"
    WORKING_DIRECTORY "${EOLIB_DOCS_WORK_DIR}"
    DEPENDS eolib_generate_protocol
    COMMENT "Building the API reference in ${EOLIB_DOCS_OUTPUT_DIR}"
    VERBATIM)
