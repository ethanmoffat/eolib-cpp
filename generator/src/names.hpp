#pragma once

#include <string>
#include <vector>

namespace eolib::generator
{

struct ProtocolCase;
struct ProtocolFile;
struct ProtocolPacket;
struct Type;

/// Removes leading and trailing whitespace.
std::string Trim(const std::string& str);

/// Joins strings with a separator between each one.
std::string Join(const std::vector<std::string>& parts, const std::string& separator);

/// Returns true if the value is a (possibly negative) decimal integer literal.
bool IsInteger(const std::string& value);

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

/// Gets the C++ type name of a value of the specified type. Custom types are named relative to the namespace of the
/// from file, or fully qualified if from is nullptr.
std::string CppTypeName(const Type& type, const ProtocolFile* from = nullptr);

/// Gets the class name of a packet, e.g. "WelcomeRequestClientPacket".
/// @throws GeneratorError if the file is not a client or server protocol file.
std::string PacketClassName(const ProtocolFile& file, const ProtocolPacket& packet);

/// Gets the name of the constant that holds the default value of a named hardcoded field, e.g. "DEFAULT_VERSION".
std::string DefaultConstantName(const std::string& field_name);

/// Gets the protocol name of the data member for a switch, e.g. "reply_code_data". This is also the property name used
/// in eo-captured-packets files.
std::string SwitchDataName(const std::string& switch_field);

/// Gets the name of the variant type alias for a switch, e.g. "ReplyCodeData".
std::string SwitchDataTypeName(const std::string& switch_field);

/// Gets the name of the type generated for a switch case, e.g. "ReplyCodeDataOk" or "ReplyCodeDataDefault".
std::string SwitchCaseTypeName(const std::string& switch_field, const ProtocolCase& protocol_case);

} // namespace eolib::generator
