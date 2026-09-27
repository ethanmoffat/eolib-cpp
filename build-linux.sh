#!/usr/bin/env bash
#
# Convenience script for building on Linux (and macOS).

function main() {
    set -e

    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

    local build_dir=""
    local build_mode="Release"
    local install_dir="${script_dir}/install"
    local opt_clean="false"
    local opt_help="false"
    local opt_install="true"
    local opt_offline="OFF"
    local opt_package="false"
    local opt_test="false"
    local shared="OFF"
    local werror="OFF"

    local option
    while [[ "$#" -gt 0 ]]
    do
        option="$1"
        case "${option}" in
            -d|--debug)
                build_mode="Debug"
                ;;
            -c|--clean)
                opt_clean="true"
                ;;
            -b|--build-dir)
                build_dir="$2"
                shift
                ;;
            -i|--install-dir)
                install_dir="$2"
                shift
                ;;
            -n|--no-install)
                opt_install="false"
                ;;
            -o|--offline)
                opt_offline="ON"
                ;;
            -t|--test)
                opt_test="true"
                ;;
            -p|--package)
                opt_package="true"
                ;;
            -s|--shared)
                shared="ON"
                ;;
            -w|--werror)
                werror="ON"
                ;;
            -h|--help)
                opt_help="true"
                break
                ;;
            *)
                echo "Error: unsupported option \"${option}\""
                display_usage
                return 1
                ;;
        esac
        shift
    done

    if [[ "${opt_help}" == "true" ]]; then
        display_usage
        return 0
    fi

    if ! command -v cmake > /dev/null; then
        echo "Error: CMake is not installed. Please install CMake 3.20 or later before running this script."
        return 1
    fi

    if [[ -z "${build_dir}" ]]; then
        build_dir="${script_dir}/build/$(echo "${build_mode}" | tr '[:upper:]' '[:lower:]')"
    fi

    local jobs
    jobs="$(nproc 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 2)"

    echo ""
    echo "Build mode: ${build_mode}"
    echo "Build directory: ${build_dir}"
    if [[ "${opt_install}" == "true" ]]; then
        echo "Install directory: ${install_dir}"
    fi
    echo "Shared library: ${shared}"
    echo "Warnings as errors: ${werror}"
    echo "Offline: ${opt_offline}"

    if [[ "${opt_clean}" == "true" ]]; then
        echo ""
        echo "Cleaning build and install directories"
        if [[ -d "${build_dir}" ]]; then
            rm -rf "${build_dir}"
        fi
        if [[ "${opt_install}" == "true" && -d "${install_dir}" ]]; then
            rm -rf "${install_dir}"
        fi
    fi

    local cmake_macros=()
    cmake_macros+=("-DCMAKE_BUILD_TYPE=${build_mode}")
    cmake_macros+=("-DCMAKE_INSTALL_PREFIX=${install_dir}")
    cmake_macros+=("-DBUILD_SHARED_LIBS=${shared}")
    cmake_macros+=("-DEOLIB_BUILD_TESTS=ON")
    cmake_macros+=("-DEOLIB_WARNINGS_AS_ERRORS=${werror}")
    cmake_macros+=("-DEOLIB_OFFLINE=${opt_offline}")

    echo ""
    cmake -S "${script_dir}" -B "${build_dir}" "${cmake_macros[@]}"
    cmake --build "${build_dir}" --parallel "${jobs}"

    if [[ "${opt_test}" == "true" ]]; then
        echo ""
        ctest --test-dir "${build_dir}" --output-on-failure --parallel "${jobs}"
    fi

    if [[ "${opt_install}" == "true" ]]; then
        echo ""
        cmake --install "${build_dir}"
    fi

    if [[ "${opt_package}" == "true" ]]; then
        echo ""
        pushd "${build_dir}" > /dev/null
        cpack
        popd > /dev/null
    fi

    return 0
}

function display_usage() {
    echo "Usage:"
    echo "  build-linux.sh [options]"
    echo ""
    echo "Options:"
    echo "  -d --debug                    Build with debug symbols."
    echo "  -c --clean                    Clean before building."
    echo "  -b <dir> --build-dir <dir>    Build directory [default: build/release or build/debug]."
    echo "  -i <dir> --install-dir <dir>  Install directory [default: install]."
    echo "  -n --no-install               Build without local install."
    echo "  -o --offline                  Build without downloading dependencies. Reuses the dependencies"
    echo "                                downloaded by an earlier build in the same build directory."
    echo "  -t --test                     Execute tests."
    echo "  -p --package                  Create the release archives with CPack."
    echo "  -s --shared                   Build a shared library instead of a static library."
    echo "  -w --werror                   Treat compiler warnings as errors."
    echo "  -h --help                     Display this message."
}

main "$@"
