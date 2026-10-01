#include "model.hpp"

#include "errors.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <cctype>
#include <sstream>

namespace eolib::generator
{

namespace
{

std::string Trim(const std::string& str)
{
    const auto begin = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
    const auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); });
    if (begin >= end.base())
    {
        return {};
    }
    return std::string(begin, end.base());
}

/// Trims each line of a comment and joins the non-empty lines with a space. Comment lines are wrapped prose, so a
/// sentence spanning multiple lines is kept together.
std::string NormalizeComment(const std::string& comment)
{
    std::istringstream stream(comment);
    std::string result;
    std::string line;
    while (std::getline(stream, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }
        if (!result.empty())
        {
            result += ' ';
        }
        result += line;
    }
    return result;
}

std::string ElementText(const pugi::xml_node& node)
{
    std::string text;
    for (const auto& child : node.children())
    {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata)
        {
            text += child.value();
        }
    }
    return text;
}

void SetElementText(pugi::xml_node node, const std::string& text)
{
    while (node.first_child())
    {
        node.remove_child(node.first_child());
    }
    node.append_child(pugi::node_pcdata).set_value(text.c_str());
}

void AppendComments(pugi::xml_node element, const std::vector<std::string>& comments)
{
    std::vector<std::string> normalized;
    for (const auto& comment : comments)
    {
        auto text = NormalizeComment(comment);
        if (!text.empty())
        {
            normalized.push_back(std::move(text));
        }
    }

    if (normalized.empty())
    {
        return;
    }

    auto comment_element = element.child("comment");
    if (!comment_element)
    {
        comment_element = element.prepend_child("comment");
    }
    else if (auto existing = ElementText(comment_element); !existing.empty())
    {
        normalized.insert(normalized.begin(), std::move(existing));
    }

    std::string text;
    for (const auto& comment : normalized)
    {
        text += (text.empty() ? "" : "\n") + comment;
    }
    SetElementText(comment_element, text);
}

/// Removes XML comments from the children of the specified element (recursively) and attaches them as <comment>
/// elements. A comment attaches to the next sibling element. A comment with no following sibling element attaches to
/// its parent. Existing <comment> text is kept first, followed by trailing comments, then preceding comments. Existing
/// <comment> text is normalized the same way as XML comments.
void RewriteCommentsAsElementsInPlace(pugi::xml_node element)
{
    std::vector<std::string> pending_comments;
    for (auto child = element.first_child(); child;)
    {
        const auto next = child.next_sibling();
        if (child.type() == pugi::node_comment)
        {
            pending_comments.emplace_back(child.value());
            element.remove_child(child);
        }
        else if (child.type() == pugi::node_element)
        {
            if (std::string(child.name()) == "comment")
            {
                SetElementText(child, NormalizeComment(ElementText(child)));
            }
            else
            {
                RewriteCommentsAsElementsInPlace(child);
                AppendComments(child, pending_comments);
                pending_comments.clear();
            }
        }
        child = next;
    }

    AppendComments(element, pending_comments);
}

std::optional<std::string> ReadComment(const pugi::xml_node& node)
{
    const auto comment = node.child("comment");
    if (!comment)
    {
        return std::nullopt;
    }

    std::istringstream stream(comment.text().get());
    std::string result;
    std::string line;
    while (std::getline(stream, line))
    {
        line = Trim(line);
        if (line.empty() && result.empty())
        {
            continue;
        }
        if (!result.empty())
        {
            result += '\n';
        }
        result += line;
    }

    result = Trim(result);
    if (result.empty())
    {
        return std::nullopt;
    }
    return result;
}

std::optional<std::string> OptionalAttribute(const pugi::xml_node& node, const char* name)
{
    const auto attribute = node.attribute(name);
    if (!attribute)
    {
        return std::nullopt;
    }
    return std::string(attribute.value());
}

std::string RequiredAttribute(const pugi::xml_node& node, const char* name, const std::string& context)
{
    const auto attribute = node.attribute(name);
    if (!attribute)
    {
        throw GeneratorError(context + ": <" + node.name() + "> is missing required attribute \"" + name + "\".");
    }
    return attribute.value();
}

bool BoolAttribute(const pugi::xml_node& node, const char* name, bool default_value, const std::string& context)
{
    const auto attribute = node.attribute(name);
    if (!attribute)
    {
        return default_value;
    }

    const std::string value = attribute.value();
    if (value == "true")
    {
        return true;
    }
    if (value == "false")
    {
        return false;
    }
    throw GeneratorError(context + ": attribute \"" + name + "\" must be \"true\" or \"false\" (got \"" + value +
                         "\").");
}

/// Reads the text content of an element, excluding any child <comment> element.
std::optional<std::string> ReadTextContent(const pugi::xml_node& node)
{
    std::string text;
    bool has_text = false;
    for (const auto& child : node.children())
    {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata)
        {
            text += child.value();
            has_text = true;
        }
    }

    if (!has_text)
    {
        return std::nullopt;
    }

    text = Trim(text);
    if (text.empty())
    {
        return std::nullopt;
    }
    return text;
}

int ParseInt(const std::string& value, const std::string& context)
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

std::vector<Instruction> ReadInstructions(const pugi::xml_node& parent, const std::string& context);

Instruction ReadInstruction(const pugi::xml_node& node, const std::string& context)
{
    const std::string element = node.name();
    Instruction instruction;
    instruction.comment = ReadComment(node);

    if (element == "field")
    {
        instruction.kind = InstructionKind::Field;
        instruction.name = OptionalAttribute(node, "name");
        instruction.type = RequiredAttribute(node, "type", context);
        instruction.length = OptionalAttribute(node, "length");
        instruction.padded = BoolAttribute(node, "padded", false, context);
        instruction.optional = BoolAttribute(node, "optional", false, context);
        instruction.value = ReadTextContent(node);
    }
    else if (element == "array")
    {
        instruction.kind = InstructionKind::Array;
        instruction.name = RequiredAttribute(node, "name", context);
        instruction.type = RequiredAttribute(node, "type", context);
        instruction.length = OptionalAttribute(node, "length");
        instruction.optional = BoolAttribute(node, "optional", false, context);
        instruction.delimited = BoolAttribute(node, "delimited", false, context);
        if (node.attribute("trailing-delimiter"))
        {
            instruction.trailing_delimiter = BoolAttribute(node, "trailing-delimiter", true, context);
        }
    }
    else if (element == "length")
    {
        instruction.kind = InstructionKind::Length;
        instruction.name = RequiredAttribute(node, "name", context);
        instruction.type = RequiredAttribute(node, "type", context);
        instruction.optional = BoolAttribute(node, "optional", false, context);
        if (const auto offset = OptionalAttribute(node, "offset"))
        {
            instruction.offset = ParseInt(*offset, context);
        }
    }
    else if (element == "dummy")
    {
        instruction.kind = InstructionKind::Dummy;
        instruction.type = RequiredAttribute(node, "type", context);
        instruction.value = ReadTextContent(node);
        if (!instruction.value)
        {
            throw GeneratorError(context + ": <dummy> elements must specify a value.");
        }
    }
    else if (element == "switch")
    {
        instruction.kind = InstructionKind::Switch;
        instruction.switch_field = RequiredAttribute(node, "field", context);
        for (const auto& case_node : node.children("case"))
        {
            ProtocolCase protocol_case;
            protocol_case.value = OptionalAttribute(case_node, "value");
            protocol_case.is_default = BoolAttribute(case_node, "default", false, context);
            protocol_case.comment = ReadComment(case_node);
            protocol_case.instructions = ReadInstructions(case_node, context);

            if (protocol_case.is_default && protocol_case.value)
            {
                throw GeneratorError(context + ": default <case> elements must not specify a value.");
            }
            if (!protocol_case.is_default && !protocol_case.value)
            {
                throw GeneratorError(context + ": non-default <case> elements must specify a value.");
            }

            instruction.cases.push_back(std::move(protocol_case));
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

std::vector<Instruction> ReadInstructions(const pugi::xml_node& parent, const std::string& context)
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

} // namespace

std::string ProtocolFile::Namespace() const
{
    std::string result = "eolib::protocol";
    for (const auto& part : namespace_parts)
    {
        result += "::" + part;
    }
    return result;
}

std::string ProtocolFile::IncludeDir() const
{
    std::string result = "eolib/protocol";
    for (const auto& part : namespace_parts)
    {
        result += "/" + part;
    }
    return result;
}

void FlattenChunked(const std::vector<Instruction>& instructions, std::vector<const Instruction*>& result)
{
    for (const auto& instruction : instructions)
    {
        result.push_back(&instruction);
        if (instruction.kind == InstructionKind::Chunked)
        {
            FlattenChunked(instruction.instructions, result);
        }
    }
}

ProtocolFile LoadProtocolFile(const std::filesystem::path& path, const std::string& relative_dir)
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
            RewriteCommentsAsElementsInPlace(top_level);
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

    for (const auto& node : root.children())
    {
        if (node.type() != pugi::node_element)
        {
            continue;
        }

        const std::string element = node.name();
        if (element == "enum")
        {
            ProtocolEnum protocol_enum;
            protocol_enum.name = RequiredAttribute(node, "name", path.string());
            const std::string context = path.string() + ": " + protocol_enum.name;
            protocol_enum.type = RequiredAttribute(node, "type", context);
            protocol_enum.comment = ReadComment(node);
            for (const auto& value_node : node.children("value"))
            {
                ProtocolEnumValue value;
                value.name = RequiredAttribute(value_node, "name", context);
                value.comment = ReadComment(value_node);
                const auto text = ReadTextContent(value_node);
                if (!text)
                {
                    throw GeneratorError(context + "." + value.name + ": enum values must specify an ordinal value.");
                }
                value.ordinal = ParseInt(*text, context + "." + value.name);
                protocol_enum.values.push_back(std::move(value));
            }
            file.enums.push_back(std::move(protocol_enum));
        }
        else if (element == "struct")
        {
            ProtocolStruct protocol_struct;
            protocol_struct.name = RequiredAttribute(node, "name", path.string());
            protocol_struct.comment = ReadComment(node);
            protocol_struct.instructions = ReadInstructions(node, path.string() + ": " + protocol_struct.name);
            file.structs.push_back(std::move(protocol_struct));
        }
        else if (element == "packet")
        {
            ProtocolPacket packet;
            packet.family = RequiredAttribute(node, "family", path.string());
            packet.action = RequiredAttribute(node, "action", path.string());
            packet.comment = ReadComment(node);
            packet.instructions =
                ReadInstructions(node, path.string() + ": packet " + packet.family + "_" + packet.action);
            file.packets.push_back(std::move(packet));
        }
        else if (element != "comment")
        {
            throw GeneratorError(path.string() + ": unexpected <" + element + "> element.");
        }
    }

    return file;
}

std::vector<ProtocolFile> LoadProtocolFiles(const std::filesystem::path& xml_root)
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
        result.push_back(LoadProtocolFile(path, relative));
    }
    return result;
}

} // namespace eolib::generator
