# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure: CMake build, `eo-protocol` and `eo-captured-packets` submodules, code style configuration.
- `eolib::data` runtime: `EoReader` (chunked reading, slicing), `EoWriter` (string sanitization, padding), `NumberEncoder`, `StringEncoder` and `EoNumericLimits`.
- `EolibError`, `SerializationError` and `DeserializationError` exception types.
- `eolib::encrypt` runtime: `DataEncrypter` and `ServerVerifier`.
- `eolib::packet` runtime: `PacketSequencer` and the `ZeroSequenceStart`, `InitSequenceStart`, `PingSequenceStart` and `AccountReplySequenceStart` sequence starts.
- `eolib::protocol::Serializable` interface.
- `eolib-protocol-gen` code generator, which generates the protocol code from the `eo-protocol` XML files at build time.
- Generated protocol code: enums, structs, client/server packets and packet factories in `eolib::protocol::net`, `eolib::protocol::pub` and `eolib::protocol::map`, plus the `eolib::protocol::net::Packet` interface.
- `eolib/eolib.hpp` and `eolib/protocol.hpp` umbrella headers.
- CMake package (`find_package(eolib CONFIG)`, `eolib::eolib` target), FetchContent/`add_subdirectory` support and CPack archives named `eolib-<version>-<platform>`.
- vcpkg overlay port (`ports/eolib`).
- Round-trip tests for the packets in `eo-captured-packets`, and pub/map file tests.
