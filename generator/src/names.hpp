#pragma once

#include <string>

namespace eolib::generator
{

/// Converts snake_case to PascalCase, e.g. "reply_code" -> "ReplyCode".
std::string SnakeCaseToPascalCase(const std::string& name);

/// Returns true if the name is a C++ keyword or alternative token.
bool IsCppKeyword(const std::string& name);

/// Converts a protocol field name to a C++ identifier (appending "_" to keywords).
std::string FieldIdentifier(const std::string& name);

/// Returns true if the name is used for a parameter or local in generated member functions.
bool IsReservedIdentifier(const std::string& name);

/// Converts a protocol field name to the identifier of the generated member, which is renamed (by appending "_") if
/// it is a C++ keyword or a reserved identifier.
std::string MemberIdentifier(const std::string& name);

/// Escapes a string for use in a C++ string literal (including the quotes).
std::string StringLiteral(const std::string& value);

/// Escapes the characters that start a Doxygen command or link (\\, @ and #) in text from the protocol XML, so it
/// is shown as written in the generated documentation.
std::string DocText(const std::string& text);

} // namespace eolib::generator
