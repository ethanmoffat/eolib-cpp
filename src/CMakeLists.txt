include(GenerateExportHeader)

set(EOLIB_GENERATED_DIR "${PROJECT_BINARY_DIR}/generated")
set(EOLIB_GENERATED_INCLUDE_DIR "${EOLIB_GENERATED_DIR}/include")

include(EolibGenerate)

set(EOLIB_RUNTIME_SOURCES
    data/eo_reader.cpp
    data/eo_writer.cpp
    data/number_encoder.cpp
    data/string_encoder.cpp
    encrypt/data_encrypter.cpp
    encrypt/server_verifier.cpp
    packet/packet_sequencer.cpp
    packet/sequence_start.cpp
    version.cpp)

add_library(eolib ${EOLIB_RUNTIME_SOURCES} ${EOLIB_GENERATED_SOURCES})
add_dependencies(eolib eolib_generate_protocol)
add_library(eolib::eolib ALIAS eolib)

configure_file("${PROJECT_SOURCE_DIR}/cmake/version.hpp.in" "${EOLIB_GENERATED_INCLUDE_DIR}/eolib/version.hpp" @ONLY)

generate_export_header(
    eolib
    BASE_NAME EOLIB
    EXPORT_MACRO_NAME EOLIB_API
    EXPORT_FILE_NAME "${EOLIB_GENERATED_INCLUDE_DIR}/eolib/export.hpp")

target_include_directories(
    eolib
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${EOLIB_GENERATED_INCLUDE_DIR}>
           $<INSTALL_INTERFACE:include>)

target_compile_features(eolib PUBLIC cxx_std_17)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(eolib PUBLIC EOLIB_STATIC_DEFINE)
endif()

set_target_properties(
    eolib
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN ON
               VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
               EXPORT_NAME eolib)

target_link_libraries(eolib PRIVATE $<BUILD_INTERFACE:eolib_compiler_options>)
