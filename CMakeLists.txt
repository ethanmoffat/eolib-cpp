cmake_minimum_required(VERSION 3.18)

# The numeric version is tracked in project(); pre-release identifiers (e.g. "beta.1", "rc.2") are tracked in
# EOLIB_VERSION_SUFFIX. Leave the suffix empty for a stable release. Both must match the release tag and CHANGELOG.md.
project(
    eolib
    VERSION 0.1.0
    DESCRIPTION "Core C++ library for writing Endless Online applications"
    HOMEPAGE_URL "https://github.com/ethanmoffat/eolib-cpp"
    LANGUAGES CXX)
set(EOLIB_VERSION_SUFFIX "beta.1")

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(EolibVersion)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(EOLIB_IS_TOP_LEVEL ON)
else()
    set(EOLIB_IS_TOP_LEVEL OFF)
endif()

option(EOLIB_BUILD_TESTS "Build the eolib unit tests" ${EOLIB_IS_TOP_LEVEL})
option(EOLIB_INSTALL "Generate install rules for eolib" ${EOLIB_IS_TOP_LEVEL})
option(EOLIB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(EOLIB_OFFLINE "Build without downloading dependencies (no internet connection)" OFF)

if(EOLIB_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # Keep executables and DLLs together so the tests can find eolib.dll in shared Windows builds.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
endif()

include(EolibCompilerOptions)

set(EOLIB_GENERATED_DIR "${PROJECT_BINARY_DIR}/generated")
set(EOLIB_GENERATED_INCLUDE_DIR "${EOLIB_GENERATED_DIR}/include")

add_subdirectory(src)

if(EOLIB_INSTALL)
    include(EolibInstall)
endif()

if(EOLIB_IS_TOP_LEVEL)
    include(EolibStyle)
endif()

if(EOLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
