# eolib-protocol-gen: host tool that generates the protocol code from the eo-protocol XML files.
# It is a build-time tool only and is never installed or linked into eolib.
#
# This directory can also be built standalone (e.g. for the host when cross-compiling eolib).

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.18)
    project(eolib_protocol_gen LANGUAGES CXX)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    include(EolibCompilerOptions)
    option(EOLIB_OFFLINE "Build without downloading dependencies (no internet connection)" OFF)
endif()

find_package(pugixml CONFIG QUIET)

if(NOT TARGET pugixml::pugixml)
    # pugixml is a single source file. Build it as part of the generator rather than adding its CMake project, so
    # none of its install rules or options leak into the eolib build.
    include(EolibFetchDependency)
    eolib_fetch_dependency(eolib_pugixml URL https://github.com/zeux/pugixml/archive/refs/tags/v1.16.tar.gz
                           SOURCE_SUBDIR do-not-add-subdirectory)
    add_library(eolib_pugixml STATIC "${eolib_pugixml_SOURCE_DIR}/src/pugixml.cpp")
    target_include_directories(eolib_pugixml SYSTEM PUBLIC "${eolib_pugixml_SOURCE_DIR}/src")
    set_target_properties(eolib_pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_library(pugixml::pugixml ALIAS eolib_pugixml)
endif()

add_library(eolib_protocol_gen_lib STATIC src/doc_comment.cpp src/emitter.cpp src/model.cpp src/names.cpp
                                                  src/property_emitter.cpp src/types.cpp)
target_include_directories(eolib_protocol_gen_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(eolib_protocol_gen_lib PUBLIC cxx_std_17)
target_link_libraries(eolib_protocol_gen_lib PUBLIC pugixml::pugixml PRIVATE eolib_compiler_options)
set_target_properties(eolib_protocol_gen_lib PROPERTIES CXX_EXTENSIONS OFF)

add_executable(eolib-protocol-gen src/main.cpp)
target_link_libraries(eolib-protocol-gen PRIVATE eolib_protocol_gen_lib eolib_compiler_options)
set_target_properties(eolib-protocol-gen PROPERTIES CXX_EXTENSIONS OFF)

# GCC 8 requires a separate library for std::filesystem.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(eolib_protocol_gen_lib PUBLIC stdc++fs)
endif()
