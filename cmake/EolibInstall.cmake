# Install rules, CMake package configuration and CPack settings for eolib.

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(EOLIB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/eolib" CACHE STRING "Install directory for the eolib CMake package")

install(
    TARGETS eolib
    EXPORT eolibTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")
# Generated headers: export.hpp, version.hpp and the protocol code.
install(DIRECTORY "${EOLIB_GENERATED_INCLUDE_DIR}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")

if(MSVC AND BUILD_SHARED_LIBS)
    install(FILES $<TARGET_PDB_FILE:eolib> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()

install(
    EXPORT eolibTargets
    NAMESPACE eolib::
    FILE eolibTargets.cmake
    DESTINATION ${EOLIB_INSTALL_CMAKEDIR})

configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/eolibConfig.cmake.in"
                              "${PROJECT_BINARY_DIR}/eolibConfig.cmake" INSTALL_DESTINATION ${EOLIB_INSTALL_CMAKEDIR})

# Before 1.0.0, minor versions may contain breaking changes.
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(eolib_version_compatibility SameMinorVersion)
else()
    set(eolib_version_compatibility SameMajorVersion)
endif()

write_basic_package_version_file("${PROJECT_BINARY_DIR}/eolibConfigVersion.cmake" VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY ${eolib_version_compatibility})

install(FILES "${PROJECT_BINARY_DIR}/eolibConfig.cmake" "${PROJECT_BINARY_DIR}/eolibConfigVersion.cmake"
        DESTINATION ${EOLIB_INSTALL_CMAKEDIR})

install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/CHANGELOG.md" "${PROJECT_SOURCE_DIR}/README.md"
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

# CPack: prebuilt archives named eolib-<version>-<platform>.
string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" eolib_default_package_platform)
set(EOLIB_PACKAGE_PLATFORM "${eolib_default_package_platform}"
    CACHE STRING "Platform identifier used in package file names, e.g. windows-x64, linux-x64-musl")

set(CPACK_PACKAGE_NAME "eolib")
set(CPACK_PACKAGE_VENDOR "Ethan Moffat")
set(CPACK_PACKAGE_VERSION "${EOLIB_VERSION_FULL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "eolib-${EOLIB_VERSION_FULL}-${EOLIB_PACKAGE_PLATFORM}")
set(CPACK_PACKAGE_CHECKSUM SHA256)
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_SOURCE_GENERATOR "")

include(CPack)
