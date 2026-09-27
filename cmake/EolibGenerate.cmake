# Wires the protocol code generator into the build.
#
# Defines:
#   EOLIB_PROTOCOL_XML_DIR       - directory containing the eo-protocol XML files
#   EOLIB_GENERATED_DIR          - root directory of the generated code (include/ and src/)
#   EOLIB_GENERATED_HEADERS      - list of generated headers
#   EOLIB_GENERATED_SOURCES      - list of generated sources
#   eolib_generate_protocol      - custom target that runs the generator
#
# Global properties (for use outside of this directory scope, e.g. by the tests):
#   EOLIB_GENERATOR_COMMAND      - the generator executable (may be a generator expression)
#   EOLIB_GENERATOR_DEPENDS      - dependencies of commands that run the generator (target or file, and XML files)
#
# The generator is built from source, unless EOLIB_GENERATOR_EXECUTABLE points to a prebuilt host executable. When
# cross-compiling without EOLIB_GENERATOR_EXECUTABLE, the generator is built for the host with ExternalProject.
#
# The set of generated files is computed at configure time from the XML files, following the generator's layout:
#   eolib/protocol/<dir>/enums.{hpp,cpp}           if the protocol file defines enums
#   eolib/protocol/<dir>/structs.{hpp,cpp}         if the protocol file defines structs
#   eolib/protocol/<dir>/packets.{hpp,cpp}         if the protocol file defines packets
#   eolib/protocol/<dir>/packet_factory.{hpp,cpp}  if the protocol file defines packets
#   eolib/protocol/<dir>.hpp                       umbrella header for each non-root protocol file
#   eolib/protocol.hpp                             umbrella header for all protocol files

set(EOLIB_PROTOCOL_XML_DIR "${PROJECT_SOURCE_DIR}/eo-protocol/xml" CACHE PATH "Directory of the eo-protocol XML files")
set(EOLIB_GENERATOR_EXECUTABLE "" CACHE FILEPATH "Prebuilt eolib-protocol-gen executable (optional)")

if(NOT EXISTS "${EOLIB_PROTOCOL_XML_DIR}/protocol.xml")
    message(
        FATAL_ERROR
            "eo-protocol XML files not found in ${EOLIB_PROTOCOL_XML_DIR}. "
            "Run 'git submodule update --init --recursive' or set EOLIB_PROTOCOL_XML_DIR.")
endif()

file(GLOB_RECURSE EOLIB_PROTOCOL_XML_FILES CONFIGURE_DEPENDS "${EOLIB_PROTOCOL_XML_DIR}/protocol.xml"
     "${EOLIB_PROTOCOL_XML_DIR}/*/protocol.xml")
list(REMOVE_DUPLICATES EOLIB_PROTOCOL_XML_FILES)

set(EOLIB_GENERATED_HEADERS "${EOLIB_GENERATED_DIR}/include/eolib/protocol.hpp")
set(EOLIB_GENERATED_SOURCES "")

foreach(xml_file IN LISTS EOLIB_PROTOCOL_XML_FILES)
    get_filename_component(xml_dir "${xml_file}" DIRECTORY)
    file(RELATIVE_PATH relative_dir "${EOLIB_PROTOCOL_XML_DIR}" "${xml_dir}")
    if(relative_dir STREQUAL "")
        set(output_dir "eolib/protocol")
    else()
        set(output_dir "eolib/protocol/${relative_dir}")
        list(APPEND EOLIB_GENERATED_HEADERS "${EOLIB_GENERATED_DIR}/include/${output_dir}.hpp")
    endif()

    file(READ "${xml_file}" xml_content)
    set(kinds "")
    if(xml_content MATCHES "<enum[ \t\r\n]")
        list(APPEND kinds enums)
    endif()
    if(xml_content MATCHES "<struct[ \t\r\n]")
        list(APPEND kinds structs)
    endif()
    if(xml_content MATCHES "<packet[ \t\r\n]")
        list(APPEND kinds packets packet_factory)
    endif()

    foreach(kind IN LISTS kinds)
        list(APPEND EOLIB_GENERATED_HEADERS "${EOLIB_GENERATED_DIR}/include/${output_dir}/${kind}.hpp")
        list(APPEND EOLIB_GENERATED_SOURCES "${EOLIB_GENERATED_DIR}/src/${output_dir}/${kind}.cpp")
    endforeach()
endforeach()

if(EOLIB_GENERATOR_EXECUTABLE)
    set(eolib_generator_command "${EOLIB_GENERATOR_EXECUTABLE}")
    set(eolib_generator_depends "${EOLIB_GENERATOR_EXECUTABLE}")
elseif(CMAKE_CROSSCOMPILING)
    include(ExternalProject)
    set(eolib_host_generator_dir "${PROJECT_BINARY_DIR}/host-generator")
    ExternalProject_Add(
        eolib_host_generator
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/generator"
        BINARY_DIR "${eolib_host_generator_dir}"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${eolib_host_generator_dir}/bin
                   -DEOLIB_OFFLINE=${EOLIB_OFFLINE}
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
        BUILD_BYPRODUCTS "${eolib_host_generator_dir}/bin/eolib-protocol-gen${CMAKE_HOST_EXECUTABLE_SUFFIX}"
        INSTALL_COMMAND "")
    set(eolib_generator_command "${eolib_host_generator_dir}/bin/eolib-protocol-gen${CMAKE_HOST_EXECUTABLE_SUFFIX}")
    set(eolib_generator_depends eolib_host_generator)
else()
    add_subdirectory("${PROJECT_SOURCE_DIR}/generator" "${PROJECT_BINARY_DIR}/generator")
    set(eolib_generator_command $<TARGET_FILE:eolib-protocol-gen>)
    set(eolib_generator_depends eolib-protocol-gen)
endif()

set(eolib_generator_stamp "${EOLIB_GENERATED_DIR}/protocol.stamp")

# The generator only rewrites files whose content changed, so the generated files are listed as byproducts of a stamp
# file. This avoids recompiling generated sources when the generator or XML changes without affecting the output.
add_custom_command(
    OUTPUT "${eolib_generator_stamp}"
    BYPRODUCTS ${EOLIB_GENERATED_HEADERS} ${EOLIB_GENERATED_SOURCES}
    COMMAND "${eolib_generator_command}" --input "${EOLIB_PROTOCOL_XML_DIR}" --output "${EOLIB_GENERATED_DIR}" --stamp
            "${eolib_generator_stamp}"
    DEPENDS ${eolib_generator_depends} ${EOLIB_PROTOCOL_XML_FILES}
    COMMENT "Generating eolib protocol code"
    VERBATIM)

add_custom_target(eolib_generate_protocol DEPENDS "${eolib_generator_stamp}")

set_property(GLOBAL PROPERTY EOLIB_GENERATOR_COMMAND "${eolib_generator_command}")
set_property(GLOBAL PROPERTY EOLIB_GENERATOR_DEPENDS ${eolib_generator_depends} ${EOLIB_PROTOCOL_XML_FILES})
