#include "names.hpp"

#include <array>
#include <cctype>
#include <cstdio>
#include <set>
#include <string_view>

namespace eolib::generator
{

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

} // namespace eolib::generator
