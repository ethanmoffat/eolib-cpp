# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0-beta.1] - 2026-09-26

### Added
- Initial project structure: CMake build, `build-linux.sh`/`build-windows.ps1` convenience scripts, `scripts/install-deps.sh`/`scripts/install-deps.ps1` dependency installation scripts, `scripts/prepare-release.sh` and `scripts/validate-release.sh` release scripts, `eo-protocol` and `eo-captured-packets` submodules, code style configuration, and the `EOLIB_OFFLINE` option for building without an internet connection.
- `eolib::data` runtime: `EoReader` (chunked reading, slicing), `EoWriter` (string sanitization, padding), `NumberEncoder`, `StringEncoder` and `EoNumericLimits`.
- `EolibError`, `SerializationError` and `DeserializationError` exception types.
- `eolib::encrypt` runtime: `DataEncrypter` and `ServerVerifier`.
- `eolib::packet` runtime: `PacketSequencer` and the `ZeroSequenceStart`, `InitSequenceStart`, `PingSequenceStart` and `AccountReplySequenceStart` sequence starts, with `MaxValue` constants for generated values.
- `eolib::protocol::Serializable` interface.
- `eolib-protocol-gen` code generator, which generates the protocol code from the `eo-protocol` XML files at build time.
- Generated protocol code: enums, structs, client/server packets and packet factories in `eolib::protocol::net`, `eolib::protocol::pub` and `eolib::protocol::map`, plus the `eolib::protocol::net::Packet` interface.
- `eolib/eolib.hpp` and `eolib/protocol.hpp` umbrella headers.
- CMake package (`find_package(eolib CONFIG)`, `eolib::eolib` target), FetchContent/`add_subdirectory` support and CPack archives named `eolib-<version>-<platform>`.
- Tests for the packets in `eo-captured-packets`, checking byte-exact round trips and the field values of each packet against the captured properties, and pub/map file tests.
- Documentation: getting started guide.

[Unreleased]: https://github.com/ethanmoffat/eolib-cpp/compare/v0.1.0-beta.1...HEAD
[0.1.0-beta.1]: https://github.com/ethanmoffat/eolib-cpp/releases/tag/v0.1.0-beta.1
