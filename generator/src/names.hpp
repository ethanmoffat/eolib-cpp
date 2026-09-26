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

/// Escapes a string for use in a C++ string literal (including the quotes).
std::string StringLiteral(const std::string& value);

} // namespace eolib::generator
