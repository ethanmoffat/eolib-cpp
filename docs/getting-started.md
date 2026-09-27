# Getting started

This guide covers the main parts of the eolib API. Include everything with `<eolib/eolib.hpp>`, or include only the
headers you need:

| Header | Contents |
|---|---|
| `eolib/data/eo_reader.hpp`, `eolib/data/eo_writer.hpp` | `EoReader`, `EoWriter` |
| `eolib/data/number_encoder.hpp`, `eolib/data/string_encoder.hpp` | EO number/string encoding |
| `eolib/encrypt/data_encrypter.hpp`, `eolib/encrypt/server_verifier.hpp` | Packet encryption, server verification |
| `eolib/packet/packet_sequencer.hpp`, `eolib/packet/sequence_start.hpp` | Packet sequencing |
| `eolib/protocol.hpp` | All generated protocol code |
| `eolib/protocol/net/client.hpp`, `eolib/protocol/net/server.hpp` | Client or server packets only |
| `eolib/protocol/pub.hpp`, `eolib/protocol/map.hpp` | Pub and map files |

## Naming conventions

- Namespaces follow the protocol XML layout: `eolib::protocol`, `eolib::protocol::net`, `eolib::protocol::net::client`,
  `eolib::protocol::net::server`, `eolib::protocol::pub`, `eolib::protocol::pub::server` and `eolib::protocol::map`.
- Types, methods and enum values are PascalCase. Packets are named `<Family><Action><Client|Server>Packet`, for
  example `client::WalkPlayerClientPacket`.
- Fields are public members named exactly as in the protocol XML (snake_case).

## Protocol types

Generated types derive from `eolib::protocol::Serializable`, which provides `Serialize(EoWriter&)`,
`Deserialize(EoReader&)`, `ByteSize()` and `ToString()`, and can be written to a `std::ostream` with `operator<<`.
Enums have `ToString(value)` and `operator<<` too. Structs also have `operator==` and `operator!=`.

| Protocol type | C++ type |
|---|---|
| `byte`, `char`, `short`, `three`, `int` | `int` |
| `bool` | `bool` |
| `string`, `encoded_string` | `std::string` (raw bytes, no transcoding) |
| `blob` | `std::vector<std::uint8_t>` |
| enum | `enum class : int`. Unrecognized values are preserved. |
| array | `std::vector<T>` |
| optional field | `std::optional<T>` |
| switch | `std::variant<std::monostate, ...>` of generated case structs |

### Numeric types

All EO numbers are `int`, as in eolib-dotnet and eolib-java. Sizes, positions and lengths are `int` too, since protocol
lengths can have negative offsets applied. Only raw byte data (`blob`, `GetBytes`, pointer + length overloads) uses
`std::uint8_t` and `std::size_t`.

- EO `int` values can be up to 4,097,152,080, which does not fit in a 32-bit `int`. Values above `INT_MAX` are read as
  negative numbers. `AddInt` accepts them back, so they round-trip, but they can't be written as positive values.
- Malformed data can decode to negative or out-of-range values; for example, a `0x00` byte decodes as -1.
- Validate numbers read from untrusted data (such as client packets on a server) before using them, for example by
  rejecting negative amounts.

## Packets

Every packet derives from `eolib::protocol::net::Packet`, which adds `Family()` and `Action()`. Packet classes also
have `FAMILY` and `ACTION` constants.

```cpp
using namespace eolib;
using namespace eolib::protocol::net;

client::WalkPlayerClientPacket walk;
walk.walk_action.direction = protocol::Direction::Up;
walk.walk_action.coords.x = 5;
walk.walk_action.coords.y = 7;

data::EoWriter writer;
walk.Serialize(writer);
std::vector<std::uint8_t> payload = writer.ToByteArray();
```

`PacketFactory` creates packets from their family and action. It returns `nullptr` for unknown packets:

```cpp
data::EoReader reader(payload);
std::unique_ptr<Packet> packet = client::PacketFactory::Deserialize(PacketFamily::Walk, PacketAction::Player, reader);
if (auto* received = dynamic_cast<client::WalkPlayerClientPacket*>(packet.get()))
{
    // use received->walk_action
}
```

`EoReader`'s constructors copy (or take ownership of) the data. To read a buffer you own without copying it, use
`EoReader::View`. The buffer must outlive the reader and its slices; generated types copy their fields out, so it only
needs to live until `Deserialize` returns:

```cpp
void OnPacket(const std::string& payload)
{
    data::EoReader reader = data::EoReader::View(payload);
    auto packet = client::PacketFactory::Deserialize(family, action, reader);
}
```

Switch fields are `std::variant` members named `<field>_data`. Set the alternative that matches the switch field,
or serialization throws `eolib::SerializationError`:

```cpp
server::InitInitServerPacket init;
init.reply_code = server::InitReply::Ok;

server::InitInitServerPacket::ReplyCodeDataOk ok;
ok.player_id = 1;
init.reply_code_data = ok;

if (const auto* data = std::get_if<server::InitInitServerPacket::ReplyCodeDataOk>(&init.reply_code_data))
{
    // use data->player_id
}
```

## Pub and map files

```cpp
std::ifstream file("dat001.eif", std::ios::binary);
std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

protocol::pub::Eif eif;
data::EoReader eif_reader(bytes);
eif.Deserialize(eif_reader);

for (const auto& item : eif.items)
{
    std::cout << item.name << "\n";
}
```

`protocol::pub::Enf`, `Esf` and `Ecf`, the `protocol::pub::server` files and `protocol::map::Emf` work the same way.

## Encryption and sequencing

`DataEncrypter` transforms the packet bytes after the 2-byte length prefix in place. Each function accepts a `std::vector<std::uint8_t>&`, a `std::string&`, or a pointer and length:

```cpp
using encrypt::DataEncrypter;

// Encrypt
DataEncrypter::SwapMultiples(bytes, 6);
DataEncrypter::Interleave(bytes);
DataEncrypter::FlipMsb(bytes);

// Decrypt
DataEncrypter::FlipMsb(bytes);
DataEncrypter::Deinterleave(bytes);
DataEncrypter::SwapMultiples(bytes, 6);
```

A server generates a sequence start for the init handshake and tracks the sequence for each connection:

```cpp
auto start = packet::InitSequenceStart::Generate();
init_ok.seq1 = start.Seq1();
init_ok.seq2 = start.Seq2();

packet::PacketSequencer sequencer(start);
int expected_sequence = sequencer.NextSequence();
```

`ServerVerifier::Hash(challenge)` computes the response to the client's challenge in `InitInitClientPacket`.

## Errors

- `EoReader` does not throw when data runs out. Reads return `0` or empty values, like the official client.
- `EoWriter` throws `std::invalid_argument` for values that can't be encoded.
- Generated `Serialize` methods throw `eolib::SerializationError` for invalid objects, such as fixed-size arrays with
  the wrong number of elements or switch data that doesn't match the switch field.
- All eolib exceptions derive from `eolib::EolibError`.

## Windows

The public headers compile after `<windows.h>` without `NOMINMAX` or `WIN32_LEAN_AND_MEAN`; a Windows test checks
this. Members named like Win32 macros (for example `Weight::max`) are not followed by `(`, so function-like macros do
not expand. Object-like macros from other headers can still collide with enum values. If that happens, include
eolib before those headers or `#undef` the macro.
