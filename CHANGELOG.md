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
