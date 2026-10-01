find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    include(EolibFetchDependency)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    eolib_fetch_dependency(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz)
endif()

find_package(nlohmann_json 3 CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    include(EolibFetchDependency)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    eolib_fetch_dependency(nlohmann_json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
endif()

include(GoogleTest)

# Test support code that builds packets from the eo-captured-packets properties, generated from the protocol XML.
get_property(eolib_generator_command GLOBAL PROPERTY EOLIB_GENERATOR_COMMAND)
get_property(eolib_generator_depends GLOBAL PROPERTY EOLIB_GENERATOR_DEPENDS)
set(eolib_test_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(eolib_test_properties_source "${eolib_test_generated_dir}/captured_packet_properties.cpp")
set(eolib_test_properties_stamp "${eolib_test_generated_dir}/captured_packet_properties.stamp")
add_custom_command(
    OUTPUT "${eolib_test_properties_stamp}"
    BYPRODUCTS "${eolib_test_properties_source}"
    COMMAND "${eolib_generator_command}" --mode test-properties --input "${EOLIB_PROTOCOL_XML_DIR}" --output
            "${eolib_test_generated_dir}" --stamp "${eolib_test_properties_stamp}"
    DEPENDS ${eolib_generator_depends}
    COMMENT "Generating eolib test properties code"
    VERBATIM)
add_custom_target(eolib_generate_test_properties DEPENDS "${eolib_test_properties_stamp}")

# Only the test directories are globbed, so the consumer smoke test and the submodules are left out.
file(
    GLOB_RECURSE
    eolib_test_sources
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/data/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/encrypt/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/packet/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/protocol/*.cpp")
file(GLOB eolib_root_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND eolib_test_sources ${eolib_root_test_sources})
file(GLOB eolib_windows_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/windows_*_test.cpp")
list(REMOVE_ITEM eolib_test_sources ${eolib_windows_test_sources})

add_executable(eolib_tests ${eolib_test_sources} "${eolib_test_properties_source}")
add_dependencies(eolib_tests eolib_generate_test_properties)

if(MSVC)
    # The generated properties source defines many functions.
    target_compile_options(eolib_tests PRIVATE /bigobj)
endif()

target_include_directories(eolib_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(eolib_tests
                           PRIVATE EOLIB_CAPTURED_PACKETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/eo-captured-packets")
target_link_libraries(eolib_tests PRIVATE eolib::eolib GTest::gtest_main nlohmann_json::nlohmann_json
                                          eolib_compiler_options)
set_target_properties(eolib_tests PROPERTIES CXX_EXTENSIONS OFF)

if(WIN32)
    target_sources(eolib_tests PRIVATE ${eolib_windows_test_sources})
endif()

gtest_discover_tests(eolib_tests)

# The generator tests require the generator library, which is unavailable when a prebuilt or host generator is used.
if(TARGET eolib_protocol_gen_lib)
    file(GLOB eolib_generator_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generator/*.cpp")
    add_executable(eolib_generator_tests ${eolib_generator_test_sources})
    target_link_libraries(eolib_generator_tests PRIVATE eolib_protocol_gen_lib GTest::gtest_main eolib_compiler_options)
    set_target_properties(eolib_generator_tests PROPERTIES CXX_EXTENSIONS OFF)
    gtest_discover_tests(eolib_generator_tests)
endif()
