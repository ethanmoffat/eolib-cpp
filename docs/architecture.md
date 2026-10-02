# Architecture

This document describes how eolib-cpp is put together: what each part of the library does, how the files are organized, and how the pieces call into each other. It's intended for contributors and for anyone who wants to understand the library beyond the public API. For usage, see [Getting started](getting-started.md).

## Background

The Endless Online protocol is described by the XML files in [eo-protocol](https://github.com/cirras/eo-protocol). Every eolib implementation (Java, .NET, Go, Rust and this one) generates its protocol types from those files instead of writing them by hand. When the protocol changes, the submodule is updated and the code is regenerated.

The protocol types can't do anything on their own, though. They need a way to read and write EO-encoded numbers and strings, and applications need the encryption and sequencing logic that goes along with the network protocol. That part is small, rarely changes, and is written by hand.

eolib-cpp is split into three parts along those lines:

1. The **runtime**: hand-written code in `include/` and `src/`. This is the reader, writer, encoders, encryption and packet sequencing, plus the base interfaces that generated types implement.
2. The **generator**: `eolib-protocol-gen`, a command-line tool in `generator/` that runs at build time and converts the XML into C++.
3. The **generated protocol code**: the output of the generator. It's written to the build tree and compiled into the same library as the runtime. It is never committed.

```
eo-protocol/xml/**/protocol.xml
        |
        |  eolib-protocol-gen (runs during the build)
        v
build/generated/include/eolib/protocol/**.hpp
build/generated/src/eolib/protocol/**.cpp
        |
        |  compiled together with src/**
        v
eolib library (CMake target eolib::eolib)
```

The generated code depends on the runtime, and the runtime knows nothing about the generated code (with one exception, described under "Protocol interfaces" below). The generator doesn't depend on either; it only reads XML and writes text.

## Repository layout

| Path | Contents |
|---|---|
| `include/eolib/` | Public headers for the runtime |
| `src/` | Runtime sources and the library's `CMakeLists.txt` |
| `generator/` | The code generator. See [Code generator](generator.md). |
| `cmake/` | CMake modules for code generation, install/packaging, versioning, compiler options, style checks and docs |
| `tests/` | Unit tests, generator tests and protocol tests |
| `eo-protocol/` | Submodule with the protocol XML files |
| `tests/eo-captured-packets/` | Submodule with packets captured from the official client and server |
| `docs/` | This documentation and the Doxygen configuration |
| `scripts/` | Dependency install, release preparation and release validation scripts |

## Runtime

The runtime is organized by namespace, and each namespace has a matching folder under `include/eolib/` and `src/`. For example, `eolib::data::EoReader` is declared in `include/eolib/data/eo_reader.hpp` and implemented in `src/data/eo_reader.cpp`.

### Data (`eolib::data`)

`EoWriter` and `EoReader` are the core of the library. Every generated `Serialize` method is a series of calls on an `EoWriter`, and every `Deserialize` method is a series of calls on an `EoReader`.

`EoWriter` appends to a `std::vector<std::uint8_t>`. Its `Add*` methods (`AddChar`, `AddShort`, `AddString`, etc.) encode values with `NumberEncoder` and `StringEncoder` before appending them. When serialization is done, the bytes are retrieved with `ToByteArray()`, or `ToByteString()` for code that stores packets in a `std::string`.

`EoReader` is a window over a buffer: a data pointer, a limit and a position. The data is either owned (the constructors copy it, and keep it alive with a `std::shared_ptr`) or borrowed (`EoReader::View`, where the caller keeps the buffer alive). `Slice()` creates another reader over part of the same buffer without copying it, and slices share ownership with the reader they were created from.

`EoReader` also has a **chunked reading mode**, which the protocol uses for sections of data separated by `0xFF` break bytes. In chunked mode, reads are limited to the current chunk, and `NextChunk()` moves past the next break byte.

Note that `EoReader` never throws when it runs out of data. Reads past the end return 0 or an empty value. This matches the official client, and the generated code for optional fields depends on it.

### Encryption (`eolib::encrypt`)

`DataEncrypter` implements the packet encryption steps (`Interleave`, `Deinterleave`, `FlipMsb` and `SwapMultiples`), and `ServerVerifier` computes the response to the client's challenge during the init handshake. Both are classes with only static functions. Each function is implemented once for a pointer and length, with inline overloads for `std::vector<std::uint8_t>&` and `std::string&` that call the pointer version.

### Packet sequencing (`eolib::packet`)

`SequenceStart` is an abstract base for the four ways a sequence start can be created: `ZeroSequenceStart`, `InitSequenceStart`, `PingSequenceStart` and `AccountReplySequenceStart`. Each subclass has static functions that create it from received values or generate new random values.

`PacketSequencer` produces the sequence number for each packet. It copies the value out of a `SequenceStart` when it's constructed (or when `SetSequenceStart` is called) and doesn't keep a reference to it.

### Protocol interfaces (`eolib::protocol`)

`Serializable` is the interface implemented by every generated struct and packet. It declares `Serialize`, `Deserialize`, `ByteSize` and `ToString`. `net::Packet` derives from `Serializable` and adds `Family()` and `Action()`.

`net::Packet` is the one place where the runtime depends on generated code: `include/eolib/protocol/net/packet.hpp` includes the generated `eolib/protocol/net/enums.hpp` for the `PacketFamily` and `PacketAction` types.

`include/eolib/protocol/detail/serialization.hpp` holds helpers that are only used by generated code. It isn't part of the public API, and it's excluded from the API documentation. It contains:

- `ChunkedReadingModeGuard` and `StringSanitizationGuard`, which save the reader's chunked reading mode or the writer's string sanitization mode and restore it when they go out of scope. Generated code uses them around `<chunked>` sections, so an exception in the middle of a chunked section doesn't leave the reader or writer in the wrong mode.
- `FormatValue` overloads used by the generated `ToString()` methods, for strings, bools, vectors, optionals and variants.

### Errors

All exceptions thrown by eolib derive from `eolib::EolibError` (`include/eolib/errors.hpp`). Generated `Serialize` methods throw `SerializationError` when an object can't be written, for example when a fixed-size array has the wrong number of elements or switch data doesn't match its switch field.

## Generated code

### Files and namespaces

The generator produces a set of files for each `protocol.xml` file. The folder of the XML file determines the C++ namespace and the folder of the generated files:

| XML file | Namespace |
|---|---|
| `xml/protocol.xml` | `eolib::protocol` |
| `xml/net/protocol.xml` | `eolib::protocol::net` |
| `xml/net/client/protocol.xml` | `eolib::protocol::net::client` |
| `xml/net/server/protocol.xml` | `eolib::protocol::net::server` |
| `xml/pub/protocol.xml` | `eolib::protocol::pub` |
| `xml/pub/server/protocol.xml` | `eolib::protocol::pub::server` |
| `xml/map/protocol.xml` | `eolib::protocol::map` |

Within each folder, the files are split by what they contain. Files are only generated if the XML file defines something of that kind:

| File | Contents |
|---|---|
| `enums.hpp/.cpp` | `enum class` types, with a `ToString` function and `operator<<` for each |
| `structs.hpp/.cpp` | Struct classes, derived from `Serializable` |
| `packets.hpp/.cpp` | Packet classes, derived from `net::Packet` |
| `packet_factory.hpp/.cpp` | The `PacketFactory` class for the namespace |

There are also umbrella headers that include everything for a namespace (for example `eolib/protocol/net/client.hpp`), and `eolib/protocol.hpp`, which includes all of the generated code.

### Generated classes

Each struct and packet becomes a `final` class with public fields named exactly as in the XML (snake_case). The class declares `Serialize`, `Deserialize`, `ByteSize`, `ToString`, `operator==` and `operator!=`, and the definitions are in the matching `.cpp` file. Packet classes also have `FAMILY` and `ACTION` constants, which are returned by the `Family()` and `Action()` overrides.

A simplified example of a generated packet:

```cpp
class EOLIB_API WalkPlayerClientPacket final : public net::Packet
{
public:
    static constexpr PacketFamily FAMILY = PacketFamily::Walk;
    static constexpr PacketAction ACTION = PacketAction::Player;

    WalkAction walk_action{};

    PacketFamily Family() const noexcept override { return FAMILY; }
    PacketAction Action() const noexcept override { return ACTION; }

    int ByteSize() const noexcept override;
    void Serialize(data::EoWriter& writer) const override;
    void Deserialize(data::EoReader& reader) override;
    std::string ToString() const override;

    bool operator==(const WalkPlayerClientPacket& other) const;
    bool operator!=(const WalkPlayerClientPacket& other) const;

private:
    int byte_size_ = 0;
};
```

`ByteSize()` returns the number of bytes the object was deserialized from, or 0 for an object that wasn't deserialized. It's stored in `byte_size_` during `Deserialize`.

Some XML elements don't map directly to a field:

- `<length>` fields have no member. The length is calculated from the referenced field when serializing, and used to read the referenced field when deserializing.
- Unnamed hardcoded fields (a field with a value but no name) have no member. The value is always written, and skipped when reading.
- Named hardcoded fields have a member and a `DEFAULT_<NAME>` constant. A zero (or empty) value is written as the default, unless the object was deserialized, so a new object sends the expected value and a received object round-trips exactly.
- `<dummy>` elements have no member. They're only written when the object would otherwise be empty.
- `<switch>` elements generate a nested class for each case that has data, and a `std::variant` member named `<field>_data` that holds one of them. `std::monostate` is used for cases without data. Each case class gets `As<Case>()` accessors that return a pointer to the data, or `nullptr`. Top-level structs and packets get static `For<Case>()` factories that set the switch field and the data together, including the fields and data of nested switches.
- Comments in the XML become Doxygen comments on the generated type or member. Comments on elements without a member (such as dummies and break bytes) are added as notes on the containing type.

### Packet factory

Each namespace with packets (`net::client` and `net::server`) has a `PacketFactory` class with three static functions:

1. `Create(family, action)` returns a new, empty packet of the right type, or `nullptr` if the family and action don't identify a known packet. It's implemented as a `switch` on the family with a nested `switch` on the action.
2. `Contains(family, action)` checks whether a packet is known without creating one.
3. `Deserialize(family, action, reader)` calls `Create`, then calls `Deserialize` on the new packet.

## How the pieces fit together

When an application receives a packet, the call flow looks like this:

1. The application removes the length prefix and decrypts the data with `DataEncrypter`.
2. It reads the action and family bytes, then creates an `EoReader` over the rest of the data.
3. It calls `client::PacketFactory::Deserialize(family, action, reader)`, which creates the packet class and calls its `Deserialize` method.
4. The generated `Deserialize` method reads each field from the reader (`reader.GetChar()`, `reader.GetString()`, etc.). Fields that are structs call that struct's `Deserialize` method with the same reader.

Sending is the same process in reverse. The application fills in a packet's fields and calls `Serialize` with an `EoWriter`. The generated method writes each field with the writer's `Add*` methods. The application then gets the bytes from the writer, adds the action, family and sequence number (from `PacketSequencer`), encrypts the data and adds the length prefix.

Pub and map files work the same way, without the packet steps: create an `EoReader` over the file's contents and call `Deserialize` on a `pub::Eif`, `map::Emf`, etc.

## Build

The CMake build is split into modules in `cmake/`:

| Module | Purpose |
|---|---|
| `EolibGenerate.cmake` | Builds the generator and runs it on the XML files |
| `EolibInstall.cmake` | Install rules, the `eolibConfig.cmake` package config and CPack archives |
| `EolibVersion.cmake` | Combines the project version with `EOLIB_VERSION_SUFFIX` (e.g. `-beta.1`) and generates `eolib/version.hpp` |
| `EolibCompilerOptions.cmake` | Warning levels, and warnings as errors when requested |
| `EolibFetchDependency.cmake` | Pinned `FetchContent` dependencies (pugixml, GoogleTest, nlohmann/json) and offline builds |
| `EolibStyle.cmake` | The `format`, `format-check` and clang-tidy targets |
| `EolibDocs.cmake` | The Doxygen `docs` target |

Code generation needs some special handling, because CMake needs the list of generated files when it configures the project, before the generator has run. `EolibGenerate.cmake` works this out by scanning each XML file for `<enum`, `<struct` and `<packet` elements, using the same rules as the generator. The generator then runs as a single custom command that depends on the generator executable and all of the XML files.

The generator only rewrites files whose content changed, and touches a stamp file when it finishes. The generated files are listed as byproducts of the stamp file, so changing the generator or the XML doesn't recompile generated sources that ended up the same.

When cross-compiling, the generator has to run on the build machine instead of the target. In that case it's built separately for the host with `ExternalProject`, or `EOLIB_GENERATOR_EXECUTABLE` can point to a prebuilt copy.

`src/CMakeLists.txt` then compiles the runtime sources and the generated sources into one library, `eolib`, with the alias `eolib::eolib`.

## Tests

The tests are built into two executables. `eolib_tests` tests the library, and `eolib_generator_tests` links the generator's sources directly to test the generator without running it as a separate program.

The folders under `tests/` follow the same layout as the code being tested:

- `data/`, `encrypt/` and `packet/` test the runtime. Most of these tests are ported from eolib-dotnet.
- `generator/` runs the generator on small XML snippets and checks the generated code or the error it reports.
- `protocol/` tests the generated code. The largest of these is `captured_packets_test.cpp`, which deserializes every packet in `eo-captured-packets`, checks that serializing it again gives the same bytes, and compares each field to the values recorded in the capture. The code that builds packets from those recorded values is itself generated, with `eolib-protocol-gen --mode test-properties`.
- `consumer/` is a small separate project that uses an installed eolib package through `find_package`. CI builds it to check that the package works for consumers.
