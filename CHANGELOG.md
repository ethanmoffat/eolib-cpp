# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- An API reference site at https://ethanmoffat.github.io/eolib-cpp/, built with Doxygen, with a summary page, the
  getting started guide, namespace descriptions and a version picker. It has the newest release of each minor version.
  Each release also includes the docs as `eolib-<version>-docs.tar.gz`.
- `EOLIB_BUILD_DOCS` CMake option (off by default), which adds a `docs` target that builds the API reference.
- `scripts/serve-docs.sh`, which builds the docs site for the working tree and serves it on localhost.
- `scripts/install-deps.sh --docs` and `scripts/install-deps.ps1 -Docs` install Doxygen into `.tools/bin`.
- `PacketFactory::Contains(family, action)` for client and server packets, which checks whether a packet exists for a family and action without creating it. It is generated from the protocol files, like `Create`.

### Fixed
- Protocol comments containing `#`, `@` or `\` are escaped in the generated documentation comments, so Doxygen shows
  them as written instead of treating them as commands or links (e.g. `#nowall`).

## [0.1.0-beta.2] - 2026-09-27

### Added
- `EoWriter::ToByteArray() &&` overload, which moves the data out of the writer instead of copying it.
- `EoReader::View`, which creates a reader over data owned by the caller without copying it.
- `operator<<` for `Serializable` (and therefore every generated struct and packet) and for generated enums, which writes the `ToString()` representation to a `std::ostream`.

### Changed
- `DataEncrypter` and `StringEncoder` functions now transform data in place instead of returning a copy. Each function has `(std::uint8_t*, std::size_t)`, `std::vector<std::uint8_t>&` and `std::string&` overloads. `StringEncoder::EncodeInPlace`/`DecodeInPlace` are replaced by the `EncodeString`/`DecodeString` pointer overloads.
- Generated structs and packets are now `final`.
- Functions that cannot throw are now `noexcept`, including the `Serializable::ByteSize`, `Packet::Family`, `Packet::Action` and `SequenceStart::Value` virtual functions. Implementations of these interfaces must also be `noexcept`.
- `eolib-protocol-gen` now rejects protocol files where a non-delimited array without a length is followed by another element in the same chunk, as required by `eo-protocol/docs/elements.md`.
- Documented the numeric type policy: all EO numbers are `int`, values above `INT_MAX` wrap to negative numbers, and malformed data can decode to negative values.
- Complete documentation for the public API: every hand-written and generated function now documents its parameters, return value and exceptions, and generated switch data members include the XML comment of the `<switch>` element.

### Fixed
- `eolib-protocol-gen` no longer dereferences an empty value (crashing on musl) when a `<length>` field is referenced by an unnamed hardcoded field; it reports the "Hardcoded fields must not reference a length field" error instead.
- `docs/getting-started.md` incorrectly listed generated `byte` fields as `std::uint8_t`; they are `int`.
- Using a moved-from `EoReader` no longer dereferences a null pointer; it now behaves like a reader over empty data.
- `DataEncrypter` documentation: the order of the encryption and decryption steps was incorrect.
- Removed documentation links to `encryption.md` and `sequence.md`, which do not exist in `eo-protocol`.
- `ServerVerifier::Hash` documentation: challenges should be no larger than 11,092,110, not `EoNumericLimits::ThreeMax`, since larger values may produce negative hashes.

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

[Unreleased]: https://github.com/ethanmoffat/eolib-cpp/compare/v0.1.0-beta.2...HEAD
[0.1.0-beta.2]: https://github.com/ethanmoffat/eolib-cpp/releases/tag/v0.1.0-beta.2
[0.1.0-beta.1]: https://github.com/ethanmoffat/eolib-cpp/releases/tag/v0.1.0-beta.1
