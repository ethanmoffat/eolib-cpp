#include "serialization_code.hpp"

#include "names.hpp"

#include <utility>

namespace eolib::generator
{

SerializationCode::SerializationCode(const ProtocolFile& file, std::string qualified_name)
    : file_(file),
      qualified_name_(std::move(qualified_name))
{
}

std::string SerializationCode::WriteStatement(const Type& type, const std::string& value,
                                              const std::optional<std::string>& length, bool padded) const
{
    const Type& serialization_type = type.SerializationType();
    std::string expression = value;
    if (type.kind == TypeKind::Enum)
    {
        expression = "static_cast<int>(" + value + ")";
    }
    else if (type.kind == TypeKind::Bool)
    {
        expression = value + " ? 1 : 0";
    }

    switch (serialization_type.kind)
    {
        case TypeKind::Integer:
            return "writer.Add" + MethodName(serialization_type, false) + "(" + expression + ");";
        case TypeKind::String:
            if (length)
            {
                expression += ", " + LengthArguments(*length, padded);
            }
            return "writer.Add" + MethodName(serialization_type, length.has_value()) + "(" + expression + ");";
        case TypeKind::Blob:
            return "writer.AddBytes(" + expression + ");";
        case TypeKind::Struct:
            return expression + ".Serialize(writer);";
        default:
            throw Error("Unhandled type for serialization: " + type.name);
    }
}

std::string SerializationCode::ReadExpression(const Type& type, const std::optional<std::string>& length, bool padded,
                                              int offset) const
{
    const Type& serialization_type = type.SerializationType();
    std::string expression;
    switch (serialization_type.kind)
    {
        case TypeKind::Integer:
            expression = "reader.Get" + MethodName(serialization_type, false) + "()" + OffsetExpression(offset);
            break;
        case TypeKind::String:
            expression = "reader.Get" + MethodName(serialization_type, length.has_value()) + "(" +
                         (length ? LengthArguments(*length, padded) : "") + ")";
            break;
        case TypeKind::Blob:
            expression = "reader.GetBytes(reader.Remaining())";
            break;
        default:
            throw Error("Unhandled type for deserialization: " + type.name);
    }

    if (type.kind == TypeKind::Enum)
    {
        return "static_cast<" + CppTypeName(type, &file_) + ">(" + expression + ")";
    }
    if (type.kind == TypeKind::Bool)
    {
        return expression + " != 0";
    }
    return expression;
}

std::string SerializationCode::HardcodedValueExpression(const Type& type, const std::string& value) const
{
    switch (type.kind)
    {
        case TypeKind::Integer:
            if (!IsInteger(value))
            {
                throw Error("\"" + value + "\" is not a valid integer value.");
            }
            return value;
        case TypeKind::Bool:
            if (value == "false")
            {
                return "false";
            }
            if (value == "true")
            {
                return "true";
            }
            throw Error("\"" + value + "\" is not a valid bool value.");
        case TypeKind::String:
            return StringLiteral(value);
        default:
            throw Error("Hardcoded field values are not allowed for " + type.name + " fields (must be a basic type).");
    }
}

std::string SerializationCode::OffsetExpression(int offset)
{
    if (offset == 0)
    {
        return {};
    }
    return offset > 0 ? " + " + std::to_string(offset) : " - " + std::to_string(-offset);
}

bool SerializationCode::HasNonZeroDefault(const Type& type, const std::string& value)
{
    switch (type.kind)
    {
        case TypeKind::Integer:
            return std::stoll(value) != 0;
        case TypeKind::Bool:
            return value == "true";
        default:
            return !value.empty();
    }
}

std::string SerializationCode::HardcodedFieldValueExpression(const Instruction& instruction, const Type& type,
                                                             const std::string& value)
{
    if (!HasNonZeroDefault(type, *instruction.value))
    {
        return value;
    }

    const std::string default_name = DefaultConstantName(*instruction.name);
    switch (type.kind)
    {
        case TypeKind::Integer:
            return "(" + value + " == 0 && byte_size_ == 0 ? " + default_name + " : " + value + ")";
        case TypeKind::Bool:
            return "(!" + value + " && byte_size_ == 0 ? " + default_name + " : " + value + ")";
        default:
            return "(" + value + ".empty() && byte_size_ == 0 ? " + default_name + " : std::string_view(" + value +
                   "))";
    }
}

std::string SerializationCode::LengthArguments(const std::string& length, bool padded)
{
    return length + (padded ? ", true" : "");
}

GeneratorError SerializationCode::Error(const std::string& message) const
{
    return GeneratorError(file_.path.string() + ": " + qualified_name_ + ": " + message);
}

std::string SerializationCode::MethodName(const Type& serialization_type, bool fixed_length)
{
    const std::string& name = serialization_type.name;
    if (serialization_type.kind == TypeKind::String)
    {
        return std::string(fixed_length ? "Fixed" : "") + (name == "encoded_string" ? "EncodedString" : "String");
    }
    if (name == "byte" || name == "char" || name == "short" || name == "three" || name == "int")
    {
        return SnakeCaseToPascalCase(name);
    }
    throw GeneratorError("Unhandled integer type " + name);
}

} // namespace eolib::generator
