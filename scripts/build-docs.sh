#!/usr/bin/env bash
#
# Builds the HTML API reference for one version of eolib-cpp with Doxygen.
#
# The Doxygen configuration and pages (docs/) always come from this repository, while the headers come from --source.
# This lets the release workflow build the docs of older release tags that predate the docs configuration.
#
# For local development, the EOLIB_BUILD_DOCS CMake option adds an equivalent "docs" target to a normal build, and
# scripts/serve-docs.sh builds and serves the docs site on localhost.

set -euo pipefail

SCRIPT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
REPO_ROOT="$(dirname "${SCRIPT_ROOT}")"

SOURCE_DIR="${REPO_ROOT}"
OUTPUT_DIR=""
BUILD_DIR=""
VERSION=""
DOXYGEN=""
WARNINGS_AS_ERRORS=NO
ARCHIVE=""

function display_usage() {
    echo "Usage:"
    echo "  build-docs.sh [options]"
    echo ""
    echo "Options:"
    echo "  --source <dir>          eolib-cpp source tree to document [default: this repository]"
    echo "  --output <dir>          Directory to write the HTML docs to (replaced) [default: <build-dir>/html]"
    echo "  --build-dir <dir>       CMake build directory used to generate the protocol headers"
    echo "                          [default: <source>/build/docs]"
    echo "  --version <version>     Version shown in the docs [default: EOLIB_VERSION_STRING of the source]"
    echo "  --doxygen <path>        Doxygen executable [default: .tools/bin/doxygen, then doxygen on PATH]"
    echo "  --archive <file>        Also create a .tar.gz of the docs, e.g. eolib-<version>-docs.tar.gz"
    echo "  --warnings-as-errors    Fail if Doxygen reports any warnings"
    echo "  -h --help               Display this message"
}

while [[ $# -gt 0 ]]
do
    case "${1}" in
        --source)               SOURCE_DIR="$(cd "${2}" && pwd)"; shift ;;
        --output)               OUTPUT_DIR="${2}"; shift ;;
        --build-dir)            BUILD_DIR="${2}"; shift ;;
        --version)              VERSION="${2}"; shift ;;
        --doxygen)              DOXYGEN="${2}"; shift ;;
        --archive)              ARCHIVE="${2}"; shift ;;
        --warnings-as-errors)   WARNINGS_AS_ERRORS=FAIL_ON_WARNINGS ;;
        -h|--help)              display_usage; exit 0 ;;
        *)
            >&2 echo "Error: unsupported option \"${1}\""
            display_usage
            exit 1
            ;;
    esac
    shift
done

if [[ -z "${DOXYGEN}" ]]; then
    if [[ -x "${REPO_ROOT}/.tools/bin/doxygen" ]]; then
        DOXYGEN="${REPO_ROOT}/.tools/bin/doxygen"
    elif command -v doxygen > /dev/null; then
        DOXYGEN="$(command -v doxygen)"
    else
        >&2 echo "Error: doxygen not found. Install it with: sudo ./scripts/install-deps.sh --docs"
        exit 1
    fi
fi

BUILD_DIR="${BUILD_DIR:-${SOURCE_DIR}/build/docs}"
mkdir -p "${BUILD_DIR}"
BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"
OUTPUT_DIR="${OUTPUT_DIR:-${BUILD_DIR}/html}"

echo "Generating the protocol headers in ${BUILD_DIR}..."
cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DEOLIB_BUILD_TESTS=OFF -DEOLIB_INSTALL=OFF \
    > /dev/null
cmake --build "${BUILD_DIR}" --target eolib_generate_protocol > /dev/null

GENERATED_DIR="${BUILD_DIR}/generated/include"
if [[ -z "${VERSION}" ]]; then
    VERSION="$(sed -n 's/^#define EOLIB_VERSION_STRING "\(.*\)"$/\1/p' "${GENERATED_DIR}/eolib/version.hpp")"
fi

CONFIG_DIR="${BUILD_DIR}/docs-config"
mkdir -p "${CONFIG_DIR}"
rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"
OUTPUT_DIR="$(cd "${OUTPUT_DIR}" && pwd)"

function configure() {
    sed -e "s|@EOLIB_DOCS_VERSION@|${VERSION}|g" \
        -e "s|@EOLIB_DOCS_CONFIG_DIR@|${REPO_ROOT}/docs|g" \
        -e "s|@EOLIB_DOCS_MAINPAGE@|${CONFIG_DIR}/index.md|g" \
        -e "s|@EOLIB_DOCS_SOURCE_DIR@|${SOURCE_DIR}|g" \
        -e "s|@EOLIB_DOCS_GENERATED_DIR@|${GENERATED_DIR}|g" \
        -e "s|@EOLIB_DOCS_OUTPUT_DIR@|${OUTPUT_DIR}|g" \
        -e "s|@EOLIB_DOCS_WORK_DIR@|${CONFIG_DIR}|g" \
        -e "s|@EOLIB_DOCS_WARN_AS_ERROR@|${WARNINGS_AS_ERRORS}|g" \
        "${1}" > "${2}"
}
configure "${REPO_ROOT}/docs/Doxyfile.in" "${CONFIG_DIR}/Doxyfile"
configure "${REPO_ROOT}/docs/index.md.in" "${CONFIG_DIR}/index.md"

echo "Building the docs for eolib ${VERSION} with $("${DOXYGEN}" --version | cut -d' ' -f1)..."
(cd "${CONFIG_DIR}" && "${DOXYGEN}" Doxyfile)
echo "${VERSION}" > "${OUTPUT_DIR}/version.txt"

if [[ -n "${ARCHIVE}" ]]; then
    # The archive has a single top-level folder named after the archive, which build-site.sh strips when extracting.
    name="$(basename "${ARCHIVE}" .tar.gz)"
    staging="$(mktemp -d)"
    cp -R "${OUTPUT_DIR}" "${staging}/${name}"
    tar -czf "${ARCHIVE}" -C "${staging}" "${name}"
    rm -rf "${staging}"
    echo "Created ${ARCHIVE}"
fi

echo "Built the docs in ${OUTPUT_DIR}"
