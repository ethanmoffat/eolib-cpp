#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace eolib::generator
{

struct Instruction;

struct ProtocolCase
{
    std::optional<std::string> value;
    bool is_default = false;
    std::optional<std::string> comment;
    std::vector<Instruction> instructions;
};

enum class InstructionKind
{
    Field,
    Array,
    Length,
    Dummy,
    Switch,
    Chunked,
    Break,
};

/// A single instruction within a struct, packet, case or chunked section. Only the members relevant to the
/// instruction's kind are populated.
struct Instruction
{
    InstructionKind kind = InstructionKind::Field;

    // field, array, length
    std::optional<std::string> name;
    // field, array, length, dummy
    std::string type;
    // field, array
    std::optional<std::string> length;
    // field (hardcoded value), dummy
    std::optional<std::string> value;
    bool padded = false;
    bool optional = false;
    // array
    bool delimited = false;
    std::optional<bool> trailing_delimiter;
    // length
    int offset = 0;
    // switch
    std::string switch_field;
    std::vector<ProtocolCase> cases;
    // chunked
    std::vector<Instruction> instructions;

    std::optional<std::string> comment;
};

struct ProtocolEnumValue
{
    std::string name;
    long long ordinal = 0;
    std::optional<std::string> comment;
};

struct ProtocolEnum
{
    std::string name;
    std::string type;
    std::optional<std::string> comment;
    std::vector<ProtocolEnumValue> values;
};

struct ProtocolStruct
{
    std::string name;
    std::optional<std::string> comment;
    std::vector<Instruction> instructions;
};

struct ProtocolPacket
{
    std::string family;
    std::string action;
    std::optional<std::string> comment;
    std::vector<Instruction> instructions;
};

/// The contents of a single protocol.xml file.
struct ProtocolFile
{
    /// Path to the XML file.
    std::filesystem::path path;
    /// Directory of the XML file relative to the XML root, using '/' separators. Empty for the root file.
    std::string relative_dir;
    /// Namespace components below eolib::protocol, e.g. {"net", "client"}.
    std::vector<std::string> namespace_parts;

    std::vector<ProtocolEnum> enums;
    std::vector<ProtocolStruct> structs;
    std::vector<ProtocolPacket> packets;

    /// Fully qualified C++ namespace, e.g. "eolib::protocol::net::client".
    std::string Namespace() const;
    /// Include path prefix for generated files, e.g. "eolib/protocol/net/client".
    std::string IncludeDir() const;
    /// Path of a generated file, relative to the include or source directory, e.g. "eolib/protocol/net/enums.hpp".
    std::string IncludePath(const std::string& file_name) const;
    /// Gets the side that the packets in this file are sent from: "client" or "server".
    /// @throws GeneratorError if this is not a client or server protocol file.
    std::string PacketSide() const;
};

/// Gets pointers to the specified instructions, with the contents of each chunked section following the chunked
/// instruction.
std::vector<const Instruction*> FlattenChunked(const std::vector<Instruction>& instructions);

/// Gets pointers to the specified instructions, with the contents of each chunked section following the chunked
/// instruction, and the contents of each switch case following the switch instruction.
std::vector<const Instruction*> FlattenAll(const std::vector<Instruction>& instructions);

} // namespace eolib::generator
