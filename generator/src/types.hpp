#pragma once

#include "model.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>

namespace eolib::generator
{

enum class TypeKind
{
    Integer,
    Bool,
    String,
    Blob,
    Enum,
    Struct,
};

/// A resolved protocol type.
struct Type
{
    TypeKind kind = TypeKind::Integer;
    /// Basic type name ("byte", "char", "string", ...) or custom type name.
    std::string name;
    std::optional<int> fixed_size;
    bool bounded = true;
    /// For bool and enum types, the integer type used for serialization.
    const Type* underlying = nullptr;
    /// For custom types, the file that defines the type.
    const ProtocolFile* file = nullptr;
    const ProtocolEnum* enum_definition = nullptr;
    const ProtocolStruct* struct_definition = nullptr;

    bool IsBasic() const
    {
        return kind == TypeKind::Integer || kind == TypeKind::Bool || kind == TypeKind::String;
    }

    bool IsCustom() const
    {
        return kind == TypeKind::Enum || kind == TypeKind::Struct;
    }

    /// The type that determines how values are read and written.
    const Type& SerializationType() const
    {
        return underlying != nullptr ? *underlying : *this;
    }
};

/// Maximum value representable by an integer type.
long long MaxValueOf(const Type& integer_type);

/// Returns true if the value is a (possibly negative) decimal integer literal.
bool IsInteger(const std::string& value);

/// A registry of all types defined across the protocol files. Mirrors eolib-java's TypeFactory.
class TypeRegistry
{
public:
    explicit TypeRegistry(const std::vector<ProtocolFile>& files);

    /// Resolves a type reference, e.g. "short", "bool:short", "Direction", or "string" with a length.
    /// @throws GeneratorError if the type is invalid.
    const Type& Get(const std::string& name, const std::optional<std::string>& length = std::nullopt);

private:
    struct Definition
    {
        const ProtocolFile* file = nullptr;
        const ProtocolEnum* enum_definition = nullptr;
        const ProtocolStruct* struct_definition = nullptr;
    };

    std::map<std::string, Definition> definitions_;
    std::map<std::string, std::unique_ptr<Type>> types_;
    std::map<std::string, std::unique_ptr<Type>> string_types_;
    std::map<std::string, bool> resolving_;

    std::unique_ptr<Type> Create(const std::string& name);
    const Type* ReadUnderlyingType(const std::string& name);
    std::unique_ptr<Type> CreateStruct(const Definition& definition);
    std::optional<int> CalculateFixedStructSize(const ProtocolStruct& protocol_struct);
    bool IsBounded(const ProtocolStruct& protocol_struct);
};

} // namespace eolib::generator
