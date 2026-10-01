# Source file lists for the eolib targets. New source files are picked up automatically: CONFIGURE_DEPENDS re-runs the
# globs on each build, and CMake reconfigures when the set of files changes.
#
# Sets:
#   EOLIB_RUNTIME_SOURCES          - hand-written sources of the eolib library
#   EOLIB_GENERATOR_SOURCES        - sources of the generator library (everything except main.cpp)
#   EOLIB_GENERATOR_MAIN_SOURCE    - entry point of eolib-protocol-gen
#
# The test sources are found in tests/CMakeLists.txt, so they're only globbed when the tests are built.
#
# This file is also included by the standalone generator build, so paths are relative to this file rather than the
# project source directory.

get_filename_component(eolib_source_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

file(GLOB_RECURSE EOLIB_RUNTIME_SOURCES CONFIGURE_DEPENDS "${eolib_source_root}/src/*.cpp")

set(EOLIB_GENERATOR_MAIN_SOURCE "${eolib_source_root}/generator/src/main.cpp")
file(GLOB EOLIB_GENERATOR_SOURCES CONFIGURE_DEPENDS "${eolib_source_root}/generator/src/*.cpp")
list(REMOVE_ITEM EOLIB_GENERATOR_SOURCES "${EOLIB_GENERATOR_MAIN_SOURCE}")
