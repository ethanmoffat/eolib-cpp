# Smoke test for consuming eolib from another CMake project.
#
#   Installed package: cmake -S tests/consumer -B build/consumer -DCMAKE_PREFIX_PATH=<eolib install prefix>
#   FetchContent:      cmake -S tests/consumer -B build/consumer -DEOLIB_CONSUMER_SOURCE_DIR=<eolib source dir>

cmake_minimum_required(VERSION 3.18)
project(eolib_consumer LANGUAGES CXX)

set(EOLIB_CONSUMER_SOURCE_DIR "" CACHE PATH "Consume eolib from this source directory with FetchContent")

if(EOLIB_CONSUMER_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(eolib SOURCE_DIR "${EOLIB_CONSUMER_SOURCE_DIR}")
    FetchContent_MakeAvailable(eolib)
    if(TARGET eolib_tests OR TARGET eolib_generator_tests)
        message(FATAL_ERROR "eolib tests should not be built when eolib is consumed as a subproject")
    endif()
else()
    find_package(eolib 0.1 CONFIG REQUIRED)
    message(STATUS "Found eolib ${eolib_VERSION_FULL} in ${eolib_DIR}")
endif()

add_executable(eolib_consumer main.cpp)
target_link_libraries(eolib_consumer PRIVATE eolib::eolib)

enable_testing()
add_test(NAME eolib_consumer COMMAND eolib_consumer)
