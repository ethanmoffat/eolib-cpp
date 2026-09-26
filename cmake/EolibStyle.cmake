# Code style targets (top-level builds only):
#   format        - reformat the hand-written sources with clang-format
#   format-check  - fail if any hand-written source is not formatted (used by CI)
#   tidy          - run clang-tidy on the runtime and generator sources (advisory)
#
# Generated protocol code is emitted in the project style by the generator, but is not required to be
# clang-format clean (long lines are not wrapped).

# scripts/install-deps installs the versions used by CI into .tools, which takes precedence.
set(eolib_style_tool_hints "${PROJECT_SOURCE_DIR}/.tools/bin" "${PROJECT_SOURCE_DIR}/.tools/Scripts")
find_program(
    EOLIB_CLANG_FORMAT_EXECUTABLE
    NAMES clang-format-18 clang-format
    NAMES_PER_DIR
    HINTS ${eolib_style_tool_hints}
    DOC "clang-format executable")
find_program(
    EOLIB_CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy-18 clang-tidy
    NAMES_PER_DIR
    HINTS ${eolib_style_tool_hints}
    DOC "clang-tidy executable")
set(EOLIB_CLANG_TIDY_EXTRA_ARGS "" CACHE STRING "Extra arguments for clang-tidy, e.g. --extra-arg=-isystem<dir>")

file(
    GLOB_RECURSE
    eolib_format_sources
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/generator/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/generator/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/tests/*.cpp"
    "${PROJECT_SOURCE_DIR}/tests/*.hpp")
list(FILTER eolib_format_sources EXCLUDE REGEX "/tests/eo-captured-packets/")

if(EOLIB_CLANG_FORMAT_EXECUTABLE)
    add_custom_target(
        format
        COMMAND "${EOLIB_CLANG_FORMAT_EXECUTABLE}" -i --style=file ${eolib_format_sources}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Formatting sources with clang-format"
        VERBATIM)
    add_custom_target(
        format-check
        COMMAND "${EOLIB_CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror --style=file ${eolib_format_sources}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Checking source formatting with clang-format"
        VERBATIM)
endif()

if(EOLIB_CLANG_TIDY_EXECUTABLE)
    file(GLOB_RECURSE eolib_tidy_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp"
         "${PROJECT_SOURCE_DIR}/generator/src/*.cpp")
    add_custom_target(
        tidy
        COMMAND "${EOLIB_CLANG_TIDY_EXECUTABLE}" -p "${PROJECT_BINARY_DIR}" --quiet ${EOLIB_CLANG_TIDY_EXTRA_ARGS}
                ${eolib_tidy_sources}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Running clang-tidy"
        VERBATIM)
    if(TARGET eolib_generate_protocol)
        add_dependencies(tidy eolib_generate_protocol)
    endif()
endif()
