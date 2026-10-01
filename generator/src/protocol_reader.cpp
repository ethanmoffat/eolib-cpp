#include "protocol_reader.hpp"

#include "comment_rewriter.hpp"
#include "errors.hpp"
#include "xml_node.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <sstream>
#include <utility>

namespace eolib::generator
{

std::vector<ProtocolFile> ProtocolReader::ReadAll(const std::filesystem::path& xml_root)
{
    if (!std::filesystem::is_directory(xml_root))
    {
        throw GeneratorError(xml_root.string() + " is not a directory.");
    }

    std::vector<std::pair<std::string, std::filesystem::path>> paths;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(xml_root))
    {
        if (entry.is_regular_file() && entry.path().filename() == "protocol.xml")
        {
            auto relative = entry.path().parent_path().lexically_relative(xml_root).generic_string();
            if (relative == ".")
            {
                relative.clear();
            }
            paths.emplace_back(relative, entry.path());
        }
    }

    std::sort(paths.begin(), paths.end());

    std::vector<ProtocolFile> result;
    for (const auto& [relative, path] : paths)
    {
        result.push_back(ReadFile(path, relative));
    }
    return result;
}

ProtocolFile ProtocolReader::ReadFile(const std::filesystem::path& path, const std::string& relative_dir)
{
    pugi::xml_document document;
    const auto parse_result = document.load_file(path.c_str(), pugi::parse_default | pugi::parse_comments);
    if (!parse_result)
    {
        throw GeneratorError(path.string() + ": failed to parse XML (" + parse_result.description() + " at offset " +
                             std::to_string(parse_result.offset) + ").");
    }
    for (auto top_level : document.children())
    {
        if (top_level.type() == pugi::node_element)
        {
            CommentRewriter::RewriteInPlace(top_level);
        }
    }

    const auto root = document.child("protocol");
    if (!root)
    {
        throw GeneratorError(path.string() + ": missing <protocol> root element.");
    }

    ProtocolFile file;
    file.path = path;
    file.relative_dir = relative_dir;

    std::string part;
    std::istringstream parts(relative_dir);
    while (std::getline(parts, part, '/'))
    {
        if (!part.empty())
        {
            file.namespace_parts.push_back(part);
        }
    }

    const ProtocolReader reader(path.string());
    for (const auto& node : root.children())
    {
        if (node.type() != pugi::node_element)
        {
            continue;
        }

        const std::string element = node.name();
        if (element == "enum")
        {
            file.enums.push_back(reader.ReadEnum(node));
        }
        else if (element == "struct")
        {
            file.structs.push_back(reader.ReadStruct(node));
        }
        else if (element == "packet")
        {
            file.packets.push_back(reader.ReadPacket(node));
        }
        else if (element != "comment")
        {
            throw GeneratorError(path.string() + ": unexpected <" + element + "> element.");
        }
    }

    return file;
}

ProtocolReader::ProtocolReader(std::string path)
    : path_(std::move(path))
{
}

ProtocolEnum ProtocolReader::ReadEnum(const pugi::xml_node& node) const
{
    ProtocolEnum protocol_enum;
    protocol_enum.name = XmlNode::RequiredAttribute(node, "name", path_);
    const std::string context = path_ + ": " + protocol_enum.name;
    protocol_enum.type = XmlNode::RequiredAttribute(node, "type", context);
    protocol_enum.comment = ReadComment(node);
    for (const auto& value_node : node.children("value"))
    {
        ProtocolEnumValue value;
        value.name = XmlNode::RequiredAttribute(value_node, "name", context);
        value.comment = ReadComment(value_node);
        const auto text = XmlNode::TrimmedText(value_node);
        if (!text)
        {
            throw GeneratorError(context + "." + value.name + ": enum values must specify an ordinal value.");
        }
        value.ordinal = ParseInt(*text, context + "." + value.name);
        protocol_enum.values.push_back(std::move(value));
    }
    return protocol_enum;
}

ProtocolStruct ProtocolReader::ReadStruct(const pugi::xml_node& node) const
{
    ProtocolStruct protocol_struct;
    protocol_struct.name = XmlNode::RequiredAttribute(node, "name", path_);
    protocol_struct.comment = ReadComment(node);
    protocol_struct.instructions = ReadInstructions(node, path_ + ": " + protocol_struct.name);
    return protocol_struct;
}

ProtocolPacket ProtocolReader::ReadPacket(const pugi::xml_node& node) const
{
    ProtocolPacket packet;
    packet.family = XmlNode::RequiredAttribute(node, "family", path_);
    packet.action = XmlNode::RequiredAttribute(node, "action", path_);
    packet.comment = ReadComment(node);
    packet.instructions = ReadInstructions(node, path_ + ": packet " + packet.family + "_" + packet.action);
    return packet;
}

std::vector<Instruction> ProtocolReader::ReadInstructions(const pugi::xml_node& parent,
                                                          const std::string& context) const
{
    std::vector<Instruction> result;
    for (const auto& child : parent.children())
    {
        if (child.type() != pugi::node_element)
        {
            continue;
        }
        const std::string element = child.name();
        if (element == "comment" || element == "case")
        {
            continue;
        }
        result.push_back(ReadInstruction(child, context));
    }
    return result;
}

Instruction ProtocolReader::ReadInstruction(const pugi::xml_node& node, const std::string& context) const
{
    const std::string element = node.name();
    Instruction instruction;
    instruction.comment = ReadComment(node);

    if (element == "field")
    {
        instruction.kind = InstructionKind::Field;
        instruction.name = XmlNode::OptionalAttribute(node, "name");
        instruction.type = XmlNode::RequiredAttribute(node, "type", context);
        instruction.length = XmlNode::OptionalAttribute(node, "length");
        instruction.padded = XmlNode::BoolAttribute(node, "padded", false, context);
        instruction.optional = XmlNode::BoolAttribute(node, "optional", false, context);
        instruction.value = XmlNode::TrimmedText(node);
    }
    else if (element == "array")
    {
        instruction.kind = InstructionKind::Array;
        instruction.name = XmlNode::RequiredAttribute(node, "name", context);
        instruction.type = XmlNode::RequiredAttribute(node, "type", context);
        instruction.length = XmlNode::OptionalAttribute(node, "length");
        instruction.optional = XmlNode::BoolAttribute(node, "optional", false, context);
        instruction.delimited = XmlNode::BoolAttribute(node, "delimited", false, context);
        if (node.attribute("trailing-delimiter"))
        {
            instruction.trailing_delimiter = XmlNode::BoolAttribute(node, "trailing-delimiter", true, context);
        }
    }
    else if (element == "length")
    {
        instruction.kind = InstructionKind::Length;
        instruction.name = XmlNode::RequiredAttribute(node, "name", context);
        instruction.type = XmlNode::RequiredAttribute(node, "type", context);
        instruction.optional = XmlNode::BoolAttribute(node, "optional", false, context);
        if (const auto offset = XmlNode::OptionalAttribute(node, "offset"))
        {
            instruction.offset = ParseInt(*offset, context);
        }
    }
    else if (element == "dummy")
    {
        instruction.kind = InstructionKind::Dummy;
        instruction.type = XmlNode::RequiredAttribute(node, "type", context);
        instruction.value = XmlNode::TrimmedText(node);
        if (!instruction.value)
        {
            throw GeneratorError(context + ": <dummy> elements must specify a value.");
        }
    }
    else if (element == "switch")
    {
        instruction.kind = InstructionKind::Switch;
        instruction.switch_field = XmlNode::RequiredAttribute(node, "field", context);
        for (const auto& case_node : node.children("case"))
        {
            instruction.cases.push_back(ReadCase(case_node, context));
        }
    }
    else if (element == "chunked")
    {
        instruction.kind = InstructionKind::Chunked;
        instruction.instructions = ReadInstructions(node, context);
    }
    else if (element == "break")
    {
        instruction.kind = InstructionKind::Break;
    }
    else
    {
        throw GeneratorError(context + ": unexpected <" + element + "> element.");
    }

    return instruction;
}

ProtocolCase ProtocolReader::ReadCase(const pugi::xml_node& node, const std::string& context) const
{
    ProtocolCase protocol_case;
    protocol_case.value = XmlNode::OptionalAttribute(node, "value");
    protocol_case.is_default = XmlNode::BoolAttribute(node, "default", false, context);
    protocol_case.comment = ReadComment(node);
    protocol_case.instructions = ReadInstructions(node, context);

    if (protocol_case.is_default && protocol_case.value)
    {
        throw GeneratorError(context + ": default <case> elements must not specify a value.");
    }
    if (!protocol_case.is_default && !protocol_case.value)
    {
        throw GeneratorError(context + ": non-default <case> elements must specify a value.");
    }
    return protocol_case;
}

std::optional<std::string> ProtocolReader::ReadComment(const pugi::xml_node& node)
{
    const auto comment = node.child("comment");
    if (!comment)
    {
        return std::nullopt;
    }
    return XmlNode::TrimmedText(comment);
}

int ProtocolReader::ParseInt(const std::string& value, const std::string& context)
{
    const GeneratorError error(context + ": \"" + value + "\" is not a valid integer.");
    std::size_t consumed = 0;
    int result = 0;
    try
    {
        result = std::stoi(value, &consumed);
    }
    catch (const std::logic_error&)
    {
        throw error;
    }
    if (consumed != value.size())
    {
        throw error;
    }
    return result;
}

} // namespace eolib::generator
