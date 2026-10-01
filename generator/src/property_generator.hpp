#pragma once

#include "code_writer.hpp"
#include "generated_file.hpp"
#include "model.hpp"
#include "types.hpp"

#include <set>
#include <string>
#include <vector>

namespace eolib::generator
{

/// Generates test support code that builds packets from the property trees in eo-captured-packets JSON files, so the
/// tests can compare deserialized field values and serialize packets that were not produced by deserialization.
///
/// The generated file, "captured_packet_properties.cpp", implements the functions declared in the eolib tests'
/// "protocol/captured_packet_properties.hpp" header, and depends on nlohmann/json. Each generated object has a
/// specialization of `template <typename T> T FromProperties(const Json& properties)`, where properties is the array
/// of property objects.
class PropertyGenerator
{
public:
    PropertyGenerator(const std::vector<ProtocolFile>& files, TypeRegistry& types);

    /// Generates captured_packet_properties.cpp.
    /// @throws GeneratorError if any protocol file is invalid.
    std::vector<OutputFile> Generate();

private:
    /// The code for the properties of an object, built up as its instructions are generated.
    struct ObjectBody
    {
        /// An if/else if branch for each property name.
        CodeWriter branches;
        /// Names of the properties that are not stored in the object (length fields).
        std::vector<std::string> ignored_names;
    };

    const std::vector<ProtocolFile>& files_;
    TypeRegistry& types_;
    CodeWriter declarations_;
    CodeWriter definitions_;
    std::set<const ProtocolStruct*> generated_structs_;

    /// Gets an expression for the value of a property, including struct properties.
    std::string PropertyExpression(const Type& type, const std::string& property) const;

    /// Generates the FromProperties specialization for a struct type, if it hasn't been generated yet.
    void RequireStruct(const Type& type);

    /// Generates the FromProperties specialization for an object. dump_prefix is the prefix of switch case type
    /// names in eo-captured-packets files, which is the family and action for packets.
    void GenerateObject(const ProtocolFile& file, const std::string& qualified_name, const std::string& dump_prefix,
                        const std::vector<Instruction>& instructions);
    void GenerateInstructions(const ProtocolFile& file, const std::string& qualified_name,
                              const std::string& dump_prefix, const std::vector<Instruction>& instructions,
                              ObjectBody& body);
    void GenerateField(const Instruction& instruction, ObjectBody& body);
    void GenerateArray(const Instruction& instruction, ObjectBody& body);
    void GenerateSwitch(const ProtocolFile& file, const std::string& qualified_name, const std::string& dump_prefix,
                        const Instruction& instruction, ObjectBody& body);

    /// Generates PacketFromProperties and PacketsEqual for the packets of a client or server protocol file.
    void GeneratePacketFunctions(const ProtocolFile& file, CodeWriter& output);

    /// Opens the branch for a property name.
    static void BeginBranch(ObjectBody& body, const std::string& name);

    /// Writes the fallback statement that follows an if/else if chain: in an else branch if the chain has any
    /// branches, otherwise on its own.
    static void WriteElse(CodeWriter& writer, bool has_branches, const std::string& fallback);
};

} // namespace eolib::generator
