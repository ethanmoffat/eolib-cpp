# eolib-protocol-gen: host tool that generates the protocol code from the eo-protocol XML files.
# It is a build-time tool only and is never installed or linked into eolib.
#
# This directory can also be built standalone (e.g. for the host when cross-compiling eolib).

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(eolib_protocol_gen LANGUAGES CXX)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    include(EolibCompilerOptions)
endif()

find_package(pugixml CONFIG QUIET)

if(NOT TARGET pugixml::pugixml)
    include(FetchContent)
    # Let option() in pugixml honor the variables set here.
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)
    set(PUGIXML_NO_EXCEPTIONS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        pugixml
        URL https://github.com/zeux/pugixml/archive/refs/tags/v1.14.tar.gz
        URL_HASH SHA256=610f98375424b5614754a6f34a491adbddaaec074e9044577d965160ec103d2e)
    # Always build pugixml statically so the generator has no runtime dependencies.
    set(_eolib_saved_build_shared_libs ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(pugixml)
    set(BUILD_SHARED_LIBS ${_eolib_saved_build_shared_libs})
endif()

add_library(eolib_protocol_gen_lib STATIC src/emitter.cpp src/model.cpp src/names.cpp src/types.cpp)
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
