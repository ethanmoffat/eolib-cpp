#include "names.hpp"

#include "model.hpp"
#include "types.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <iterator>
#include <set>
#include <string_view>

namespace eolib::generator
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

std::string Join(const std::vector<std::string>& parts, const std::string& separator)
{
    std::string result;
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            result += separator;
        }
        result += parts[i];
    }
    return result;
}

bool IsInteger(const std::string& value)
{
    if (value.empty())
    {
        return false;
    }

    std::size_t start = value[0] == '-' ? 1 : 0;
    if (start == value.size())
    {
        return false;
    }

    for (std::size_t i = start; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
        {
            return false;
        }
    }
    return true;
}

std::string SnakeCaseToPascalCase(const std::string& name)
{
    std::string result;
    bool upper = true;
    for (const char c : name)
    {
        if (c == '_')
        {
            upper = true;
            continue;
        }
        result += upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        upper = false;
    }
    return result;
}

bool IsCppKeyword(const std::string& name)
{
    static constexpr std::array<std::string_view, 97> KEYWORDS = {
        "alignas",     "alignof",   "and",        "and_eq",    "asm",      "auto",         "bitand",
        "bitor",       "bool",      "break",      "case",      "catch",    "char",         "char8_t",
        "char16_t",    "char32_t",  "class",      "compl",     "concept",  "const",        "consteval",
        "constexpr",   "constinit", "const_cast", "continue",  "co_await", "co_return",    "co_yield",
        "decltype",    "default",   "delete",     "do",        "double",   "dynamic_cast", "else",
        "enum",        "explicit",  "export",     "extern",    "false",    "float",        "for",
        "friend",      "goto",      "if",         "inline",    "int",      "long",         "mutable",
        "namespace",   "new",       "noexcept",   "not",       "not_eq",   "nullptr",      "operator",
        "or",          "or_eq",     "private",    "protected", "public",   "register",     "reinterpret_cast",
        "requires",    "return",    "short",      "signed",    "sizeof",   "static",       "static_assert",
        "static_cast", "struct",    "switch",     "template",  "this",     "thread_local", "throw",
        "true",        "try",       "typedef",    "typeid",    "typename", "union",        "unsigned",
        "using",       "virtual",   "void",       "volatile",  "wchar_t",  "while",        "xor",
        "xor_eq",      "final",     "override",   "import",    "module",   "NULL",
    };

    for (const auto keyword : KEYWORDS)
    {
        if (name == keyword)
        {
            return true;
        }
    }
    return false;
}

std::string FieldIdentifier(const std::string& name)
{
    return IsCppKeyword(name) ? name + "_" : name;
}

bool IsReservedIdentifier(const std::string& name)
{
    static const std::set<std::string> RESERVED_IDENTIFIERS = {
        "writer",
        "reader",
        "other",
        "i",
        "result",
        "case_data",
        "old_writer_length",
        "reader_start_position",
        "reached_null_optional",
        "sanitization_guard",
        "chunked_guard",
        "byte_size_",
        "FAMILY",
        "ACTION",
    };
    return RESERVED_IDENTIFIERS.count(name) != 0;
}

std::string MemberIdentifier(const std::string& name)
{
    std::string result = FieldIdentifier(name);
    while (IsReservedIdentifier(result))
    {
        result += "_";
    }
    return result;
}

std::string StringLiteral(const std::string& value)
{
    std::string result = "\"";
    bool previous_was_hex_escape = false;
    for (const char c : value)
    {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '"' || c == '\\')
        {
            result += '\\';
            result += c;
            previous_was_hex_escape = false;
        }
        else if (byte < 0x20 || byte >= 0x7F)
        {
            char buffer[8];
            std::snprintf(buffer, sizeof(buffer), "\\x%02X", byte);
            result += buffer;
            previous_was_hex_escape = true;
        }
        else
        {
            if (previous_was_hex_escape && std::isxdigit(byte))
            {
                result += "\" \"";
            }
            result += c;
            previous_was_hex_escape = false;
        }
    }
    result += '"';
    return result;
}

std::string DocText(const std::string& text)
{
    std::string result;
    result.reserve(text.size());
    for (const char c : text)
    {
        if (c == '\\' || c == '@' || c == '#')
        {
            result += '\\';
        }
        result += c;
    }
    return result;
}

std::string CppTypeName(const Type& type, const ProtocolFile* from)
{
    switch (type.kind)
    {
        case TypeKind::Integer:
            return "int";
        case TypeKind::Bool:
            return "bool";
        case TypeKind::String:
            return "std::string";
        case TypeKind::Blob:
            return "std::vector<std::uint8_t>";
        case TypeKind::Enum:
        case TypeKind::Struct:
            break;
    }

    if (from == nullptr)
    {
        return type.file->Namespace() + "::" + type.name;
    }
    if (type.file == from)
    {
        return type.name;
    }
    if (type.file->namespace_parts.empty())
    {
        return "protocol::" + type.name;
    }
    return Join(type.file->namespace_parts, "::") + "::" + type.name;
}

std::string PacketClassName(const ProtocolFile& file, const ProtocolPacket& packet)
{
    return packet.family + packet.action + (file.PacketSide() == "client" ? "ClientPacket" : "ServerPacket");
}

std::string DefaultConstantName(const std::string& field_name)
{
    std::string result = "DEFAULT_";
    std::transform(field_name.begin(), field_name.end(), std::back_inserter(result),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

std::string SwitchDataName(const std::string& switch_field)
{
    return switch_field + "_data";
}

std::string SwitchDataTypeName(const std::string& switch_field)
{
    return SnakeCaseToPascalCase(switch_field) + "Data";
}

std::string SwitchCaseTypeName(const std::string& switch_field, const ProtocolCase& protocol_case)
{
    return SwitchDataTypeName(switch_field) +
           (protocol_case.is_default ? std::string("Default") : *protocol_case.value);
}

} // namespace eolib::generator
