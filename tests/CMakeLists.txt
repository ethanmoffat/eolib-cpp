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

find_package(nlohmann_json 3 CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

include(GoogleTest)

add_executable(
    eolib_tests
    data/eo_reader_test.cpp
    data/eo_writer_test.cpp
    data/number_encoder_test.cpp
    data/string_encoder_test.cpp
    encrypt/data_encrypter_test.cpp
    encrypt/server_verifier_test.cpp
    packet/packet_sequencer_test.cpp
    packet/sequence_start_test.cpp
    protocol/captured_packets_test.cpp
    protocol/generated_protocol_test.cpp
    protocol/pub_map_test.cpp
    protocol/serializable_test.cpp
    version_test.cpp)

target_include_directories(eolib_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(eolib_tests
                           PRIVATE EOLIB_CAPTURED_PACKETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/eo-captured-packets")
target_link_libraries(eolib_tests PRIVATE eolib::eolib GTest::gtest_main nlohmann_json::nlohmann_json
                                          eolib_compiler_options)
set_target_properties(eolib_tests PROPERTIES CXX_EXTENSIONS OFF)

gtest_discover_tests(eolib_tests)

# The generator tests require the generator library, which is unavailable when a prebuilt or host generator is used.
if(TARGET eolib_protocol_gen_lib)
    add_executable(eolib_generator_tests generator/names_test.cpp generator/validation_test.cpp)
    target_link_libraries(eolib_generator_tests PRIVATE eolib_protocol_gen_lib GTest::gtest_main eolib_compiler_options)
    set_target_properties(eolib_generator_tests PROPERTIES CXX_EXTENSIONS OFF)
    gtest_discover_tests(eolib_generator_tests)
endif()
