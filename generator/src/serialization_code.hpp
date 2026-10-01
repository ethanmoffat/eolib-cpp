#pragma once

#include "errors.hpp"
#include "model.hpp"
#include "types.hpp"

#include <optional>
#include <string>

namespace eolib::generator
{

/// Builds the expressions and statements that serialize and deserialize values in a generated class.
class SerializationCode
{
public:
    /// Creates the builder for a generated class. file is the protocol file that the class is generated for, and
    /// qualified_name is the class name used in error messages.
    SerializationCode(const ProtocolFile& file, std::string qualified_name);

    /// Gets a statement that writes the value expression to `writer`. length is the expression for the length of a
    /// sized string, or nullopt for a string without a length.
    std::string WriteStatement(const Type& type, const std::string& value, const std::optional<std::string>& length,
                               bool padded) const;

    /// Gets an expression that reads a value of the specified integer, bool, string, blob or enum type from `reader`.
    /// The offset is added to integer values.
    std::string ReadExpression(const Type& type, const std::optional<std::string>& length, bool padded,
                               int offset = 0) const;

    /// Gets the literal expression for a hardcoded value.
    /// @throws GeneratorError if the value is invalid for the type.
    std::string HardcodedValueExpression(const Type& type, const std::string& value) const;

    /// Gets the expression that adds an offset to a value, e.g. " + 1", or an empty string for an offset of 0.
    static std::string OffsetExpression(int offset);

    /// Checks whether a hardcoded value differs from the value of a value-initialized field.
    static bool HasNonZeroDefault(const Type& type, const std::string& value);

    /// Gets the expression for the value of a named hardcoded field to serialize: the field's value, or the default
    /// value if the field has a zero value and the object was not deserialized.
    static std::string HardcodedFieldValueExpression(const Instruction& instruction, const Type& type,
                                                     const std::string& value);

private:
    const ProtocolFile& file_;
    std::string qualified_name_;

    GeneratorError Error(const std::string& message) const;

    /// Gets the name of the EoWriter and EoReader methods for an integer or string type, without the "Add" or "Get"
    /// prefix, e.g. "Short" or "FixedEncodedString".
    static std::string MethodName(const Type& serialization_type, bool fixed_length);

    /// Gets the arguments that follow the value in a call to a fixed string method, e.g. "4, true".
    static std::string LengthArguments(const std::string& length, bool padded);
};

} // namespace eolib::generator
