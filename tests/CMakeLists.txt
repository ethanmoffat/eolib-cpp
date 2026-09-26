find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(
    eolib_tests
    version_test.cpp)

target_link_libraries(eolib_tests PRIVATE eolib::eolib GTest::gtest_main eolib_compiler_options)
set_target_properties(eolib_tests PROPERTIES CXX_EXTENSIONS OFF)

gtest_discover_tests(eolib_tests)
