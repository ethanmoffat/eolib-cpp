# The release source archive includes the eo-protocol submodule, which GitHub's generated archives do not.
vcpkg_download_distfile(
    ARCHIVE
    URLS "https://github.com/ethanmoffat/eolib-cpp/releases/download/v${VERSION}/eolib-${VERSION}-src.tar.gz"
    FILENAME "eolib-${VERSION}-src.tar.gz"
    SHA512 0)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

# The protocol code generator runs during the build and uses pugixml from vcpkg.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DEOLIB_BUILD_TESTS=OFF -DEOLIB_INSTALL=ON)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/eolib)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share"
     "${CURRENT_PACKAGES_DIR}/share/doc")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
