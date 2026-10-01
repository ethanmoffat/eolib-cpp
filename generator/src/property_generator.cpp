#include "property_generator.hpp"

#include "errors.hpp"
#include "names.hpp"
#include "packet_switch.hpp"

#include <functional>
#include <utility>

namespace eolib::generator
{

PropertyGenerator::PropertyGenerator(const std::vector<ProtocolFile>& files, TypeRegistry& types)
    : files_(files),
      types_(types)
{
}

std::vector<OutputFile> PropertyGenerator::Generate()
{
    // Only the packets, and the structs that they reference, are generated.
    for (const auto& file : files_)
    {
        for (const auto& packet : file.packets)
        {
            GenerateObject(file, file.Namespace() + "::" + PacketClassName(file, packet), packet.family + packet.action,
                           packet.instructions);
        }
    }

    GeneratedFile output("captured_packet_properties.cpp", GeneratedFile::Kind::Source);
    output.Includes({"protocol/captured_packet_properties.hpp"}, {});
    output.Includes({"eolib/protocol.hpp"}, {"memory", "string", "variant"});
    output.BeginNamespace("eolib::test");
    output.Line("namespace");
    output.Line("{");
    output.Line();
    output.Line("using namespace eolib::test::detail;");
    output.Line();
    output.Line("/// Creates an object of type T with its fields set from the properties array. Each type has an");
    output.Line("/// explicit specialization below.");
    output.Line("template <typename T>");
    output.Line("T FromProperties(const Json& properties);");
    output.Line();
    output.Append(declarations_);
    output.Line();
    output.Append(definitions_);
    output.TrimTrailingBlankLine();
    output.Line();
    output.Line("} // namespace");
    output.Line();

    for (const auto& file : files_)
    {
        if (!file.packets.empty())
        {
            GeneratePacketFunctions(file, output);
        }
    }

    output.EndNamespace("eolib::test");
    return {output.ToOutput()};
}

std::string PropertyGenerator::PropertyExpression(const Type& type, const std::string& property) const
{
    switch (type.kind)
    {
        case TypeKind::Integer:
            return "IntValue(" + property + ")";
        case TypeKind::Bool:
            return "BoolValue(" + property + ")";
        case TypeKind::String:
            return "StringValue(" + property + ")";
        case TypeKind::Blob:
            return "BlobValue(" + property + ")";
        case TypeKind::Enum:
            return "static_cast<" + CppTypeName(type) + ">(IntValue(" + property + "))";
        case TypeKind::Struct:
            return "FromProperties<" + CppTypeName(type) + ">(Children(" + property + "))";
    }
    throw GeneratorError("Unhandled type for property value: " + type.name);
}

void PropertyGenerator::RequireStruct(const Type& type)
{
    if (type.kind != TypeKind::Struct || !generated_structs_.insert(type.struct_definition).second)
    {
        return;
    }
    GenerateObject(*type.file, CppTypeName(type), "", type.struct_definition->instructions);
}

void PropertyGenerator::GenerateObject(const ProtocolFile& file, const std::string& qualified_name,
                                       const std::string& dump_prefix, const std::vector<Instruction>& instructions)
{
    ObjectBody body;
    GenerateInstructions(file, qualified_name, dump_prefix, instructions, body);

    const std::string signature = qualified_name + " FromProperties<" + qualified_name + ">(const Json& properties)";
    declarations_.Line("template <>");
    declarations_.Line(signature + ";");

    definitions_.Line("template <>");
    definitions_.Open(signature);
    definitions_.Line(qualified_name + " result;");
    if (body.branches.Empty() && body.ignored_names.empty())
    {
        // A loop would warn about unreachable code (MSVC C4702), since UnknownProperty doesn't return.
        definitions_.Open("if (!properties.empty())");
        definitions_.Line("UnknownProperty(properties.front(), \"" + qualified_name + "\");");
        definitions_.Close();
        definitions_.Line("return result;");
        definitions_.Close();
        definitions_.Line();
        return;
    }
    definitions_.Open("for (const auto& property : properties)");
    definitions_.Line("const std::string& name = Name(property);");
    definitions_.Append(body.branches);
    if (!body.ignored_names.empty())
    {
        std::string condition;
        for (const auto& name : body.ignored_names)
        {
            condition += (condition.empty() ? "" : " || ") + std::string("name == \"") + name + "\"";
        }
        definitions_.Line((body.branches.Empty() ? "if (" : "else if (") + condition + ")");
        definitions_.Line("{");
        definitions_.Indent();
        definitions_.Line("// Length fields are not stored in the object.");
        definitions_.Close();
    }
    definitions_.Open("else");
    definitions_.Line("UnknownProperty(property, \"" + qualified_name + "\");");
    definitions_.Close();
    definitions_.Close();
    definitions_.Line("return result;");
    definitions_.Close();
    definitions_.Line();
}

void PropertyGenerator::GenerateInstructions(const ProtocolFile& file, const std::string& qualified_name,
                                             const std::string& dump_prefix,
                                             const std::vector<Instruction>& instructions, ObjectBody& body)
{
    for (const auto& instruction : instructions)
    {
        switch (instruction.kind)
        {
            case InstructionKind::Field:
                GenerateField(instruction, body);
                break;
            case InstructionKind::Array:
                GenerateArray(instruction, body);
                break;
            case InstructionKind::Length:
                body.ignored_names.push_back(*instruction.name);
                break;
            case InstructionKind::Switch:
                GenerateSwitch(file, qualified_name, dump_prefix, instruction, body);
                break;
            case InstructionKind::Chunked:
                GenerateInstructions(file, qualified_name, dump_prefix, instruction.instructions, body);
                break;
            case InstructionKind::Dummy:
            case InstructionKind::Break:
                break;
        }
    }
}

void PropertyGenerator::GenerateField(const Instruction& instruction, ObjectBody& body)
{
    if (!instruction.name)
    {
        return;
    }

    const Type& type = types_.Get(instruction.type, instruction.length);
    RequireStruct(type);
    const std::string assignment =
        "result." + MemberIdentifier(*instruction.name) + " = " + PropertyExpression(type, "property") + ";";
    BeginBranch(body, *instruction.name);
    if (instruction.optional)
    {
        body.branches.Open("if (HasValue(property))");
        body.branches.Line(assignment);
        body.branches.Close();
    }
    else
    {
        body.branches.Line(assignment);
    }
    body.branches.Close();
}

void PropertyGenerator::GenerateArray(const Instruction& instruction, ObjectBody& body)
{
    const Type& type = types_.Get(instruction.type);
    RequireStruct(type);
    const std::string member = "result." + MemberIdentifier(*instruction.name);
    BeginBranch(body, *instruction.name);

    std::string container = member;
    if (instruction.optional)
    {
        body.branches.Open("if (!HasValue(property))");
        body.branches.Line("continue;");
        body.branches.Close();
        body.branches.Line("auto& values = " + member + ".emplace();");
        container = "values";
    }

    body.branches.Open("for (const auto& element : Children(property))");
    body.branches.Line(container + ".push_back(" + PropertyExpression(type, "element") + ");");
    body.branches.Close();
    body.branches.Close();
}

void PropertyGenerator::GenerateSwitch(const ProtocolFile& file, const std::string& qualified_name,
                                       const std::string& dump_prefix, const Instruction& instruction, ObjectBody& body)
{
    const std::string data_name = SwitchDataName(instruction.switch_field);
    const std::string member = "result." + MemberIdentifier(data_name);

    // Generate the case objects first, so their FromProperties specializations are declared before use.
    std::vector<std::pair<std::string, std::string>> data_cases;
    std::vector<std::string> empty_cases;
    for (const auto& protocol_case : instruction.cases)
    {
        const std::string case_type_name = SwitchCaseTypeName(instruction.switch_field, protocol_case);
        if (protocol_case.instructions.empty())
        {
            empty_cases.push_back(dump_prefix + case_type_name);
            continue;
        }
        const std::string case_qualified_name = qualified_name + "::" + case_type_name;
        GenerateObject(file, case_qualified_name, dump_prefix, protocol_case.instructions);
        data_cases.emplace_back(dump_prefix + case_type_name, case_qualified_name);
    }

    BeginBranch(body, data_name);
    body.branches.Line("const std::string type = TypeName(property);");
    std::string keyword = "if";
    for (const auto& [dump_name, case_qualified_name] : data_cases)
    {
        body.branches.Open(keyword + " (type == \"" + dump_name + "\")");
        body.branches.Line(member + " = FromProperties<" + case_qualified_name + ">(Children(property));");
        body.branches.Close();
        keyword = "else if";
    }
    for (const auto& dump_name : empty_cases)
    {
        body.branches.Open(keyword + " (type == \"" + dump_name + "\")");
        body.branches.Line(member + " = std::monostate();");
        body.branches.Close();
        keyword = "else if";
    }
    WriteElse(body.branches, !instruction.cases.empty(), "UnknownType(property, \"" + qualified_name + "\");");
    body.branches.Close();
}

void PropertyGenerator::GeneratePacketFunctions(const ProtocolFile& file, CodeWriter& output)
{
    const std::string side = file.PacketSide();
    const std::string net_namespace = types_.Get("PacketFamily").file->Namespace();
    const PacketSwitch packet_switch(file, net_namespace + "::");

    // Writes a block for each packet, with the statements from write_packet.
    const auto packet_cases = [&](const std::function<void(CodeWriter&, const ProtocolPacket&)>& write_packet)
    {
        return [&packet_switch, write_packet](CodeWriter& writer, const std::vector<const ProtocolPacket*>& packets)
        {
            for (const auto* packet : packets)
            {
                writer.Line(packet_switch.ActionLabel(*packet));
                writer.Line("{");
                writer.Indent();
                write_packet(writer, *packet);
                writer.Close();
            }
        };
    };

    output.BeginNamespace(side);

    output.Open("std::unique_ptr<" + net_namespace + "::Packet> PacketFromProperties(" + net_namespace +
                "::PacketFamily family, " + net_namespace + "::PacketAction action, const Json& properties)");
    packet_switch.Write(output,
                        packet_cases(
                            [&](CodeWriter& writer, const ProtocolPacket& packet)
                            {
                                const std::string packet_type = file.Namespace() + "::" + PacketClassName(file, packet);
                                writer.Line("return std::make_unique<" + packet_type + ">(FromProperties<" +
                                            packet_type + ">(properties));");
                            }),
                        "return nullptr;");
    output.Close();
    output.Line();

    output.Open("bool PacketsEqual(const " + net_namespace + "::Packet& a, const " + net_namespace + "::Packet& b)");
    output.Open("if (a.Family() != b.Family() || a.Action() != b.Action())");
    output.Line("return false;");
    output.Close();
    output.Line("const auto family = a.Family();");
    output.Line("const auto action = a.Action();");
    packet_switch.Write(
        output,
        packet_cases(
            [&](CodeWriter& writer, const ProtocolPacket& packet)
            { writer.Line("return EqualAs<" + file.Namespace() + "::" + PacketClassName(file, packet) + ">(a, b);"); }),
        "return false;");
    output.Close();
    output.Line();

    output.Line("} // namespace " + side);
    output.Line();
}

void PropertyGenerator::BeginBranch(ObjectBody& body, const std::string& name)
{
    const std::string keyword = body.branches.Empty() ? "if" : "else if";
    body.branches.Open(keyword + " (name == \"" + name + "\")");
}

void PropertyGenerator::WriteElse(CodeWriter& writer, bool has_branches, const std::string& fallback)
{
    if (!has_branches)
    {
        writer.Line(fallback);
        return;
    }
    writer.Open("else");
    writer.Line(fallback);
    writer.Close();
}

} // namespace eolib::generator
