# eolib-cpp

[![Build](https://github.com/ethanmoffat/eolib-cpp/actions/workflows/build.yml/badge.svg)](https://github.com/ethanmoffat/eolib-cpp/actions/workflows/build.yml)

Core C++ library for writing Endless Online applications.

The protocol code is generated at build time from the [eo-protocol](https://github.com/cirras/eo-protocol) XML
specification, which is included as a git submodule. The API follows the conventions of
[eolib-dotnet](https://github.com/ethanmoffat/eolib-dotnet) and [eolib-go](https://github.com/ethanmoffat/eolib-go).

## Features

Read and write the following EO data structures:

- Client packets
- Server packets
- Endless Map Files (EMF)
- Client pub files (items, npcs, spells, classes)
- Server pub files (talk, shops, drops, inns, skillmasters)

Utilities:

- Data reader/writer
- Number/string encoding
- Data encryption
- Packet sequencing

## Requirements

- A C++17 compiler (GCC 8+, Clang 7+, AppleClang 11+, MSVC 2019+)
- CMake 3.18+ (3.21+ for `CMakePresets.json`)

Supported platforms are Windows (x64, x86), Linux (glibc and musl) and macOS (arm64, x64).

## Installation

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    eolib
    URL https://github.com/ethanmoffat/eolib-cpp/releases/download/v0.1.0-alpha.1/eolib-0.1.0-alpha.1-src.tar.gz
    URL_HASH SHA512=<contents of eolib-0.1.0-alpha.1-src.tar.gz.sha512>)
FetchContent_MakeAvailable(eolib)

target_link_libraries(my_app PRIVATE eolib::eolib)
```

Use the release source archive instead of GitHub's generated "Source code" archives, which do not include the
`eo-protocol` submodule. A `GIT_REPOSITORY`/`GIT_TAG` declaration also works, because FetchContent clones submodules
by default. When eolib is consumed as a subproject, its tests and install rules are disabled by default.

### Prebuilt packages

Each [release](https://github.com/ethanmoffat/eolib-cpp/releases) has static library archives named
`eolib-<version>-<platform>` for `windows-x64`, `windows-x86`, `linux-x64-gnu`, `linux-x64-musl`, `macos-arm64` and
`macos-x64`. Extract one and point CMake at it:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/eolib-<version>-<platform>
```

```cmake
find_package(eolib 0.1 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE eolib::eolib)
```

### vcpkg

An overlay port is in [`ports/eolib`](ports/README.md):

```sh
vcpkg install eolib --overlay-ports=/path/to/eolib-cpp/ports
```

## Usage

```cpp
#include <eolib/eolib.hpp>

using namespace eolib;
using namespace eolib::protocol::net;

client::WalkPlayerClientPacket packet;
packet.walk_action.direction = protocol::Direction::Up;
packet.walk_action.coords.x = 5;
packet.walk_action.coords.y = 7;

data::EoWriter writer;
packet.Serialize(writer);

data::EoReader reader(writer.ToByteArray());
std::unique_ptr<Packet> received = client::PacketFactory::Deserialize(packet.Family(), packet.Action(), reader);
```

See [docs/getting-started.md](docs/getting-started.md) for more.

## Building from source

```sh
git clone --recurse-submodules https://github.com/ethanmoffat/eolib-cpp.git
cd eolib-cpp
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Without presets:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
cmake --install build --prefix install
```

### Options

| Option | Default | Description |
|---|---|---|
| `EOLIB_BUILD_TESTS` | `ON` when top-level | Build the unit, generator and captured packet tests |
| `EOLIB_INSTALL` | `ON` when top-level | Generate install rules and the CMake package |
| `EOLIB_WARNINGS_AS_ERRORS` | `OFF` | Treat compiler warnings as errors |
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library |
| `EOLIB_PROTOCOL_XML_DIR` | `eo-protocol/xml` | The protocol XML directory to generate code from |
| `EOLIB_GENERATOR_EXECUTABLE` | (empty) | A prebuilt `eolib-protocol-gen` to use instead of building one |
| `EOLIB_PACKAGE_PLATFORM` | host platform | The platform suffix used in CPack package names |

### Code generation

`eolib-protocol-gen` (in `generator/`) is a host tool that reads the protocol XML and writes C++ headers and sources
to `<build>/generated`. It runs automatically as part of the build and only rewrites files whose contents change.
When cross-compiling, the generator is built for the host with `ExternalProject`, or you can pass
`EOLIB_GENERATOR_EXECUTABLE`. Generated code is not committed to the repository, but it is installed with the
library.

### Code style

Hand-written code is formatted with clang-format 18 (`.clang-format`) and checked with clang-tidy (`.clang-tidy`):

```sh
cmake --build build --target format        # reformat sources
cmake --build build --target format-check  # fail on formatting differences
cmake --build build --target tidy          # run clang-tidy
```

Generated code is not format-checked.

## Versioning and releases

eolib-cpp uses [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`, with an optional `-alpha.N`,
`-beta.N` or `-rc.N` suffix). Changes are tracked in [CHANGELOG.md](CHANGELOG.md), following
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

To release a new version:

1. Move the `[Unreleased]` changelog entries to a new `## [x.y.z-suffix] - YYYY-MM-DD` section.
2. Update `VERSION` in `project()` and `EOLIB_VERSION_SUFFIX` in `CMakeLists.txt`. Leave the suffix empty for
   stable releases.
3. Commit and push the tag `vx.y.z-suffix`. The release workflow validates the version and changelog, builds and
   tests the platform packages and the source archive, and publishes a GitHub release. It is marked as a
   prerelease when the version has a suffix.
4. Update the [vcpkg port](ports/README.md).

## Documentation

- [Getting started](docs/getting-started.md)

## License

[MIT](LICENSE)
