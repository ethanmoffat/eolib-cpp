#include "types.hpp"

#include "errors.hpp"

#include <set>
#include <vector>

namespace eolib::generator
{

namespace
{

void FlattenInstructions(const std::vector<Instruction>& instructions, std::vector<const Instruction*>& result)
{
    for (const auto& instruction : instructions)
    {
        result.push_back(&instruction);
        if (instruction.kind == InstructionKind::Chunked)
        {
            FlattenInstructions(instruction.instructions, result);
        }
        else if (instruction.kind == InstructionKind::Switch)
        {
            for (const auto& protocol_case : instruction.cases)
            {
                FlattenInstructions(protocol_case.instructions, result);
            }
        }
    }
}

} // namespace

long long MaxValueOf(const Type& integer_type)
{
    if (integer_type.name == "byte")
    {
        return 255;
    }

    long long result = 1;
    for (int i = 0; i < integer_type.fixed_size.value_or(0); ++i)
    {
        result *= 253;
    }
    return result - 1;
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

TypeRegistry::TypeRegistry(const std::vector<ProtocolFile>& files)
{
    for (const auto& file : files)
    {
        for (const auto& protocol_enum : file.enums)
        {
            if (!definitions_.emplace(protocol_enum.name, Definition{&file, &protocol_enum, nullptr}).second)
            {
                throw GeneratorError(file.path.string() + ": " + protocol_enum.name + " type cannot be redefined.");
            }
        }
        for (const auto& protocol_struct : file.structs)
        {
            if (!definitions_.emplace(protocol_struct.name, Definition{&file, nullptr, &protocol_struct}).second)
            {
                throw GeneratorError(file.path.string() + ": " + protocol_struct.name + " type cannot be redefined.");
            }
        }
    }
}

const Type& TypeRegistry::Get(const std::string& name, const std::optional<std::string>& length)
{
    if (length)
    {
        if (name != "string" && name != "encoded_string")
        {
            throw GeneratorError(name + " type with length " + *length +
                                 " is invalid. (Only string types may specify a length)");
        }

        const std::string key = name + "[" + *length + "]";
        auto it = string_types_.find(key);
        if (it == string_types_.end())
        {
            auto type = std::make_unique<Type>();
            type->kind = TypeKind::String;
            type->name = name;
            type->bounded = true;
            if (IsInteger(*length))
            {
                type->fixed_size = std::stoi(*length);
            }
            it = string_types_.emplace(key, std::move(type)).first;
        }
        return *it->second;
    }

    auto it = types_.find(name);
    if (it == types_.end())
    {
        if (resolving_[name])
        {
            throw GeneratorError(name + " type is recursive.");
        }
        resolving_[name] = true;
        auto type = Create(name);
        resolving_[name] = false;
        it = types_.emplace(name, std::move(type)).first;
    }
    return *it->second;
}

const Type* TypeRegistry::ReadUnderlyingType(const std::string& name)
{
    const auto colon = name.find(':');
    if (colon == std::string::npos)
    {
        return nullptr;
    }

    if (name.find(':', colon + 1) != std::string::npos)
    {
        throw GeneratorError("\"" + name + "\" type syntax is invalid. (Only one colon is allowed)");
    }

    const std::string type_name = name.substr(0, colon);
    const std::string underlying_name = name.substr(colon + 1);
    if (type_name == underlying_name)
    {
        throw GeneratorError(type_name + " type cannot specify itself as an underlying type.");
    }

    const Type& underlying = Get(underlying_name);
    if (underlying.kind != TypeKind::Integer)
    {
        throw GeneratorError(underlying.name +
                             " is not a numeric type, so it cannot be specified as an underlying type.");
    }
    return &underlying;
}

std::unique_ptr<Type> TypeRegistry::Create(const std::string& full_name)
{
    const Type* underlying = ReadUnderlyingType(full_name);
    const std::string name = underlying != nullptr ? full_name.substr(0, full_name.find(':')) : full_name;

    auto type = std::make_unique<Type>();
    type->name = name;

    if (name == "byte" || name == "char" || name == "short" || name == "three" || name == "int")
    {
        type->kind = TypeKind::Integer;
        type->fixed_size = name == "byte" || name == "char" ? 1 : name == "short" ? 2 : name == "three" ? 3 : 4;
    }
    else if (name == "bool")
    {
        type->kind = TypeKind::Bool;
        type->underlying = underlying != nullptr ? underlying : &Get("char");
        type->fixed_size = type->underlying->fixed_size;
        return type;
    }
    else if (name == "string" || name == "encoded_string")
    {
        type->kind = TypeKind::String;
        type->bounded = false;
    }
    else if (name == "blob")
    {
        type->kind = TypeKind::Blob;
        type->bounded = false;
    }
    else
    {
        const auto definition = definitions_.find(name);
        if (definition == definitions_.end())
        {
            throw GeneratorError(name + " type is not defined.");
        }

        if (definition->second.enum_definition != nullptr)
        {
            const auto& protocol_enum = *definition->second.enum_definition;
            type->kind = TypeKind::Enum;
            type->file = definition->second.file;
            type->enum_definition = &protocol_enum;

            if (underlying == nullptr)
            {
                if (protocol_enum.type == protocol_enum.name)
                {
                    throw GeneratorError(name + " type cannot specify itself as an underlying type.");
                }
                const Type& default_underlying = Get(protocol_enum.type);
                if (default_underlying.kind != TypeKind::Integer)
                {
                    throw GeneratorError(default_underlying.name +
                                         " is not a numeric type, so it cannot be specified as an underlying type.");
                }
                underlying = &default_underlying;
            }

            std::set<long long> ordinals;
            std::set<std::string> names;
            for (const auto& value : protocol_enum.values)
            {
                if (!ordinals.insert(value.ordinal).second)
                {
                    throw GeneratorError(name + "." + value.name + " cannot redefine ordinal value " +
                                         std::to_string(value.ordinal) + ".");
                }
                if (!names.insert(value.name).second)
                {
                    throw GeneratorError(name + " enum cannot redefine value name " + value.name + ".");
                }
            }

            type->underlying = underlying;
            type->fixed_size = underlying->fixed_size;
            return type;
        }

        if (underlying != nullptr)
        {
            throw GeneratorError(name + " has no underlying type, so " + underlying->name +
                                 " is not allowed as an underlying type override.");
        }
        return CreateStruct(definition->second);
    }

    if (underlying != nullptr)
    {
        throw GeneratorError(name + " has no underlying type, so " + underlying->name +
                             " is not allowed as an underlying type override.");
    }

    return type;
}

std::unique_ptr<Type> TypeRegistry::CreateStruct(const Definition& definition)
{
    auto type = std::make_unique<Type>();
    type->kind = TypeKind::Struct;
    type->name = definition.struct_definition->name;
    type->file = definition.file;
    type->struct_definition = definition.struct_definition;
    type->fixed_size = CalculateFixedStructSize(*definition.struct_definition);
    type->bounded = IsBounded(*definition.struct_definition);
    return type;
}

std::optional<int> TypeRegistry::CalculateFixedStructSize(const ProtocolStruct& protocol_struct)
{
    int size = 0;

    for (const auto& instruction : protocol_struct.instructions)
    {
        switch (instruction.kind)
        {
            case InstructionKind::Field:
            {
                const auto& type = Get(instruction.type, instruction.length);
                if (!type.fixed_size || instruction.optional)
                {
                    return std::nullopt;
                }
                size += *type.fixed_size;
                break;
            }
            case InstructionKind::Array:
            {
                if (!instruction.length || !IsInteger(*instruction.length))
                {
                    return std::nullopt;
                }
                const auto& type = Get(instruction.type);
                if (!type.fixed_size || instruction.optional || instruction.delimited)
                {
                    return std::nullopt;
                }
                size += std::stoi(*instruction.length) * *type.fixed_size;
                break;
            }
            case InstructionKind::Length:
            {
                const auto& type = Get(instruction.type);
                if (!type.fixed_size || instruction.optional)
                {
                    return std::nullopt;
                }
                size += *type.fixed_size;
                break;
            }
            case InstructionKind::Dummy:
            {
                const auto& type = Get(instruction.type);
                if (!type.fixed_size)
                {
                    return std::nullopt;
                }
                size += *type.fixed_size;
                break;
            }
            case InstructionKind::Chunked:
            case InstructionKind::Switch:
                return std::nullopt;
            case InstructionKind::Break:
                break;
        }
    }

    return size;
}

bool TypeRegistry::IsBounded(const ProtocolStruct& protocol_struct)
{
    std::vector<const Instruction*> instructions;
    FlattenInstructions(protocol_struct.instructions, instructions);

    bool result = true;
    for (const auto* instruction : instructions)
    {
        if (!result)
        {
            result = instruction->kind == InstructionKind::Break;
            continue;
        }

        switch (instruction->kind)
        {
            case InstructionKind::Field:
                result = Get(instruction->type, instruction->length).bounded;
                break;
            case InstructionKind::Array:
                result = Get(instruction->type).bounded && instruction->length.has_value();
                break;
            case InstructionKind::Dummy:
                result = Get(instruction->type).bounded;
                break;
            default:
                break;
        }
    }
    return result;
}

} // namespace eolib::generator
