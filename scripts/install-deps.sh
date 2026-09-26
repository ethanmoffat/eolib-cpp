#!/usr/bin/env bash
#
# Installs the dependencies for building eolib on Linux (Ubuntu/Debian, RHEL/Fedora, Alpine) and macOS.
#
# Required: a C++17 compiler, make, git and CMake 3.21 or later. On Linux, this must be run as root, since packages are
# installed with the system package manager. On macOS, this must not be run as root, since packages are installed with
# Homebrew.
#
# Optional:
#   - clang-format and clang-tidy (the versions used by CI), installed with pip into a virtual environment in
#     <repo>/.tools. CMake finds them there for the format, format-check and tidy targets.
#   - System packages for pugixml, GoogleTest and nlohmann/json. Without them, CMake downloads these with
#     FetchContent at configure time.

SCRIPT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
REPO_ROOT="$(dirname "${SCRIPT_ROOT}")"

MIN_CMAKE_VERSION="3.21.0"
CMAKE_VERSION="3.31.6"
STYLE_TOOLS_VERSION="18.1.8"

SKIPCMAKE=false
SKIPSTYLETOOLS=false
SYSTEMLIBS=false
DRYRUN=false
HELP=false

function parse_options() {
    while [[ $# -gt 0 ]]
    do
        local option="${1}"
        case "${option}" in
            --skip-cmake)          SKIPCMAKE=true ;;
            --skip-style-tools)    SKIPSTYLETOOLS=true ;;
            --system-libs)         SYSTEMLIBS=true ;;
            --cmake-version)       CMAKE_VERSION="${2}"; shift ;;
            --dry-run)             DRYRUN=true ;;
            -h|--help)             HELP=true; break ;;
            *)
                >&2 echo "Error: unsupported option \"${option}\""
                HELP=true
                break
                ;;
        esac
        shift
    done
}

function display_usage() {
    echo "Usage:"
    echo "  install-deps.sh [options]"
    echo ""
    echo "All required dependencies and the style tools are installed by default. Options:"
    echo "  --skip-cmake              Skip installing CMake"
    echo "  --skip-style-tools        Skip installing clang-format/clang-tidy ${STYLE_TOOLS_VERSION} into .tools"
    echo "  --system-libs             Install pugixml, GoogleTest and nlohmann/json system packages, so CMake"
    echo "                            doesn't download them"
    echo "  --cmake-version <ver>     CMake version to download if the system CMake is too old [default: ${CMAKE_VERSION}]"
    echo "  --dry-run                 Print the commands without running them"
    echo "  -h --help                 Display this message"
    echo ""
    echo "On Linux, this script must be run as root (e.g. with sudo). On macOS, it must not be run as root."
}

# Runs a command, or only prints it if --dry-run is specified.
function run() {
    echo "+ $*"
    if [[ "${DRYRUN}" == "false" ]]; then
        "$@"
    fi
}

# Runs a command as the invoking user rather than root when run with sudo, so files in the repository aren't owned
# by root.
function run_as_user() {
    if [[ -n "${SUDO_USER}" && "${EUID}" -eq 0 ]]; then
        run sudo -u "${SUDO_USER}" "$@"
    else
        run "$@"
    fi
}

# Returns success if version $1 is at least version $2.
function version_at_least() {
    [[ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n 1)" == "$2" ]]
}

function detect_platform() {
    if [[ "$(uname -s)" == "Darwin" ]]; then
        PLATFORM_NAME="macos"
        PLATFORM_VERSION="$(sw_vers -productVersion)"
        return
    fi

    if [[ ! -f /etc/os-release ]]; then
        >&2 echo "Unsupported platform detected! /etc/os-release not found."
        exit 1
    fi

    local id id_like
    # shellcheck disable=SC1091
    id="$(. /etc/os-release && echo "${ID}")"
    # shellcheck disable=SC1091,SC2153
    id_like="$(. /etc/os-release && echo "${ID_LIKE}")"
    # shellcheck disable=SC1091
    PLATFORM_VERSION="$(. /etc/os-release && echo "${VERSION_ID}")"

    case " ${id} ${id_like} " in
        *" ubuntu "*|*" debian "*)              PLATFORM_NAME="debian" ;;
        *" alpine "*)                           PLATFORM_NAME="alpine" ;;
        *" rhel "*|*" centos "*|*" fedora "*)   PLATFORM_NAME="rhel" ;;
        *)
            >&2 echo "Unsupported platform detected: ${id}. Use Ubuntu/Debian, RHEL/Fedora, Alpine or macOS."
            exit 1
            ;;
    esac
}

function install_packages() {
    # Packages per platform: required, style tools (Python for pip), and the optional system libraries.
    local required style libs
    case "${PLATFORM_NAME}" in
        debian)
            required="g++ make git ca-certificates curl"
            style="python3 python3-venv"
            libs="libpugixml-dev libgtest-dev nlohmann-json3-dev"
            ;;
        rhel)
            required="gcc-c++ make git ca-certificates curl"
            style="python3"
            libs="pugixml-devel gtest-devel json-devel"
            ;;
        alpine)
            # Kitware's CMake binaries require glibc, so CMake always comes from the Alpine packages.
            required="bash build-base linux-headers git ca-certificates curl"
            if [[ "${SKIPCMAKE}" == "false" ]]; then
                required="${required} cmake"
            fi
            style="python3"
            libs="pugixml-dev gtest-dev nlohmann-json"
            ;;
        macos)
            # The Xcode command line tools provide the compiler (AppleClang), make and git.
            required=""
            if [[ "${SKIPCMAKE}" == "false" ]]; then
                required="cmake"
            fi
            style="python3"
            libs="pugixml googletest nlohmann-json"
            ;;
    esac

    local packages="${required}"
    if [[ "${SKIPSTYLETOOLS}" == "false" ]]; then
        packages="${packages} ${style}"
    fi
    if [[ "${SYSTEMLIBS}" == "true" ]]; then
        packages="${packages} ${libs}"
    fi

    local package_list
    read -r -a package_list <<< "${packages}"
    if [[ ${#package_list[@]} -eq 0 ]]; then
        return
    fi

    echo "Installing packages: ${package_list[*]}"
    case "${PLATFORM_NAME}" in
        debian)
            run apt-get update
            run env DEBIAN_FRONTEND=noninteractive apt-get install -y "${package_list[@]}"
            ;;
        rhel)
            if command -v dnf > /dev/null; then
                run dnf install -y "${package_list[@]}"
            else
                run yum install -y "${package_list[@]}"
            fi
            ;;
        alpine)
            run apk add --no-cache "${package_list[@]}"
            ;;
        macos)
            run brew install "${package_list[@]}"
            ;;
    esac
}

function check_macos_prerequisites() {
    if ! xcode-select -p > /dev/null 2>&1; then
        echo "Installing the Xcode command line tools..."
        run xcode-select --install
        echo "Re-run this script after the Xcode command line tools installation finishes."
        exit 0
    fi
    if ! command -v brew > /dev/null; then
        >&2 echo "Homebrew is required on macOS. Install it from https://brew.sh and re-run this script."
        exit 1
    fi
}

function install_cmake() {
    if [[ "${SKIPCMAKE}" == "true" || "${PLATFORM_NAME}" == "alpine" || "${PLATFORM_NAME}" == "macos" ]]; then
        return
    fi

    local current_version=""
    if command -v cmake > /dev/null; then
        current_version="$(cmake --version | head -n 1 | awk '{print $3}')"
    fi

    if [[ -n "${current_version}" ]] && version_at_least "${current_version}" "${MIN_CMAKE_VERSION}"; then
        echo "Found CMake ${current_version}"
        return
    fi

    local arch
    case "$(uname -m)" in
        x86_64)          arch="x86_64" ;;
        aarch64|arm64)   arch="aarch64" ;;
        *)
            >&2 echo "No CMake binaries are available for $(uname -m). Install CMake ${MIN_CMAKE_VERSION} or later" \
                "manually."
            exit 1
            ;;
    esac

    echo "Installing CMake ${CMAKE_VERSION} to /usr/local (found: ${current_version:-none})..."
    local installer="cmake-${CMAKE_VERSION}-linux-${arch}.sh"
    local download_dir="${TMPDIR:-/tmp}/eolib-cmake-$$"
    run mkdir -p "${download_dir}"
    run curl -fsSL -o "${download_dir}/${installer}" \
        "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${installer}"
    run sh "${download_dir}/${installer}" --skip-license --prefix=/usr/local --exclude-subdir
    run rm -rf "${download_dir}"
}

function install_style_tools() {
    if [[ "${SKIPSTYLETOOLS}" == "true" ]]; then
        return
    fi

    local tools_dir="${REPO_ROOT}/.tools"
    echo "Installing clang-format and clang-tidy ${STYLE_TOOLS_VERSION} to ${tools_dir}..."
    run_as_user python3 -m venv "${tools_dir}"
    run_as_user "${tools_dir}/bin/python" -m pip install --quiet --upgrade pip
    run_as_user "${tools_dir}/bin/python" -m pip install --quiet \
        "clang-format==${STYLE_TOOLS_VERSION}" "clang-tidy==${STYLE_TOOLS_VERSION}"
}

parse_options "$@"

if [[ "${HELP}" == "true" ]]; then
    display_usage
    exit 0
fi

set -e

detect_platform
echo "Detected platform: ${PLATFORM_NAME} ${PLATFORM_VERSION}"

if [[ "${DRYRUN}" == "false" ]]; then
    if [[ "${PLATFORM_NAME}" == "macos" && "${EUID}" -eq 0 ]]; then
        >&2 echo "This script must not be run as root on macOS, since Homebrew refuses to run as root."
        exit 1
    elif [[ "${PLATFORM_NAME}" != "macos" && "${EUID}" -ne 0 ]]; then
        >&2 echo "This script must be run as root, as it installs dependencies via the system package manager." \
            "Please run using sudo."
        exit 1
    fi
fi

if [[ "${PLATFORM_NAME}" == "macos" ]]; then
    check_macos_prerequisites
fi

install_packages
install_cmake
install_style_tools

echo ""
echo "Done. Initialize the submodules if you haven't already: git submodule update --init --recursive"
