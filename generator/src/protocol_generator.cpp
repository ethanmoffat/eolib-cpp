#include "protocol_generator.hpp"

#include "enum_generator.hpp"
#include "errors.hpp"
#include "names.hpp"
#include "packet_factory_generator.hpp"

#include <algorithm>
#include <functional>
#include <utility>

namespace eolib::generator
{

ProtocolGenerator::ProtocolGenerator(const std::vector<ProtocolFile>& files, TypeRegistry& types)
    : files_(files),
      types_(types)
{
}

std::vector<OutputFile> ProtocolGenerator::Generate()
{
    std::vector<OutputFile> result;
    EnumGenerator enum_generator(types_);
    PacketFactoryGenerator packet_factory_generator(types_);
    for (const auto& file : files_)
    {
        const auto layout = FileLayout::Of(file);
        if (layout.has_enums)
        {
            enum_generator.Generate(file, result);
        }
        if (layout.has_structs)
        {
            GenerateStructs(file, result);
        }
        if (layout.has_packets)
        {
            GeneratePackets(file, result);
            packet_factory_generator.Generate(file, result);
        }
        if (!file.namespace_parts.empty())
        {
            GenerateUmbrella(file, result);
        }
    }
    GenerateRootUmbrella(result);
    return result;
}

void ProtocolGenerator::GenerateStructs(const ProtocolFile& file, std::vector<OutputFile>& result)
{
    std::set<std::string> includes = {"eolib/export.hpp", "eolib/protocol/serializable.hpp", "eolib/data/eo_reader.hpp",
                                      "eolib/data/eo_writer.hpp"};
    if (FileLayout::Of(file).has_enums)
    {
        includes.insert(file.IncludePath("enums.hpp"));
    }

    std::vector<ObjectDefinition> objects;
    for (const auto* protocol_struct : SortStructs(file))
    {
        objects.push_back(ObjectDefinition{protocol_struct->name, protocol_struct->comment,
                                           protocol_struct->instructions, std::nullopt});
    }

    GenerateObjectFiles(file, "structs", std::move(includes), objects, result);
}

void ProtocolGenerator::GeneratePackets(const ProtocolFile& file, std::vector<OutputFile>& result)
{
    ValidatePacketIds(file);
    const auto layout = FileLayout::Of(file);
    const Type& family_type = types_.Get("PacketFamily");

    std::set<std::string> includes = {"eolib/export.hpp", "eolib/protocol/net/packet.hpp", "eolib/data/eo_reader.hpp",
                                      "eolib/data/eo_writer.hpp", family_type.file->IncludePath("enums.hpp")};
    if (layout.has_enums)
    {
        includes.insert(file.IncludePath("enums.hpp"));
    }
    if (layout.has_structs)
    {
        includes.insert(file.IncludePath("structs.hpp"));
    }

    std::vector<ObjectDefinition> objects;
    for (const auto& packet : file.packets)
    {
        objects.push_back(ObjectDefinition{PacketClassName(file, packet), packet.comment, packet.instructions,
                                           PacketInfo{packet.family, packet.action}});
    }

    GenerateObjectFiles(file, "packets", std::move(includes), objects, result);
}

void ProtocolGenerator::GenerateObjectFiles(const ProtocolFile& file, const std::string& name,
                                            std::set<std::string> includes,
                                            const std::vector<ObjectDefinition>& objects,
                                            std::vector<OutputFile>& result)
{
    for (const auto& object : objects)
    {
        for (const auto* type : types_.ReferencedTypes(object.instructions))
        {
            if (type->file != &file)
            {
                includes.insert(type->file->IncludePath(type->kind == TypeKind::Enum ? "enums.hpp" : "structs.hpp"));
            }
        }
    }

    GeneratedFile header("include/" + file.IncludePath(name + ".hpp"), GeneratedFile::Kind::Header);
    header.Includes(includes, {"cstdint", "optional", "string", "string_view", "variant", "vector"});
    header.BeginNamespace(file.Namespace());

    GeneratedFile source("src/" + file.IncludePath(name + ".cpp"), GeneratedFile::Kind::Source);
    source.Includes({file.IncludePath(name + ".hpp"), "eolib/errors.hpp", "eolib/protocol/detail/serialization.hpp"},
                    {"cstddef", "string", "variant"});
    source.BeginNamespace(file.Namespace());

    for (const auto& object : objects)
    {
        ObjectGenerator generator(types_, file, object.class_name, object.class_name, Context{});
        generator.GenerateInstructions(object.instructions);
        generator.Finish(object.comment, object.packet, header, source);
        header.Line();
    }

    header.EndNamespace(file.Namespace());
    source.EndNamespace(file.Namespace());
    result.push_back(header.ToOutput());
    result.push_back(source.ToOutput());
}

std::vector<const ProtocolStruct*> ProtocolGenerator::SortStructs(const ProtocolFile& file)
{
    std::vector<const ProtocolStruct*> result;
    std::set<const ProtocolStruct*> visited;
    std::set<const ProtocolStruct*> visiting;

    std::function<void(const ProtocolStruct&)> visit = [&](const ProtocolStruct& protocol_struct)
    {
        if (visited.count(&protocol_struct) != 0)
        {
            return;
        }
        if (visiting.count(&protocol_struct) != 0)
        {
            throw GeneratorError(file.path.string() + ": " + protocol_struct.name + " has a circular dependency.");
        }
        visiting.insert(&protocol_struct);

        for (const auto* type : types_.ReferencedTypes(protocol_struct.instructions))
        {
            if (type->kind == TypeKind::Struct && type->file == &file)
            {
                visit(*type->struct_definition);
            }
        }

        visiting.erase(&protocol_struct);
        visited.insert(&protocol_struct);
        result.push_back(&protocol_struct);
    };

    for (const auto& protocol_struct : file.structs)
    {
        visit(protocol_struct);
    }
    return result;
}

void ProtocolGenerator::ValidatePacketIds(const ProtocolFile& file)
{
    const Type& family_type = types_.Get("PacketFamily");
    const Type& action_type = types_.Get("PacketAction");
    if (family_type.kind != TypeKind::Enum || action_type.kind != TypeKind::Enum)
    {
        throw GeneratorError("PacketFamily and PacketAction must be enums.");
    }

    const auto has_value = [](const Type& type, const std::string& name)
    {
        return std::any_of(type.enum_definition->values.begin(), type.enum_definition->values.end(),
                           [&](const ProtocolEnumValue& value) { return value.name == name; });
    };

    std::set<std::pair<std::string, std::string>> ids;
    for (const auto& packet : file.packets)
    {
        if (!has_value(family_type, packet.family))
        {
            throw GeneratorError(file.path.string() + ": " + packet.family + " is not a valid PacketFamily.");
        }
        if (!has_value(action_type, packet.action))
        {
            throw GeneratorError(file.path.string() + ": " + packet.action + " is not a valid PacketAction.");
        }
        if (!ids.emplace(packet.family, packet.action).second)
        {
            throw GeneratorError(file.path.string() + ": " + packet.family + "_" + packet.action +
                                 " packet cannot be redefined in the same file.");
        }
    }
}

void ProtocolGenerator::GenerateUmbrella(const ProtocolFile& file, std::vector<OutputFile>& result) const
{
    auto headers = FileHeaders(file);
    if (file.namespace_parts.size() == 1 && file.namespace_parts[0] == "net")
    {
        headers.push_back("eolib/protocol/net/packet.hpp");
    }
    result.push_back(UmbrellaHeader("include/" + file.IncludeDir() + ".hpp", std::move(headers)));
}

void ProtocolGenerator::GenerateRootUmbrella(std::vector<OutputFile>& result) const
{
    std::vector<std::string> headers;
    for (const auto& file : files_)
    {
        if (file.namespace_parts.empty())
        {
            const auto file_headers = FileHeaders(file);
            headers.insert(headers.end(), file_headers.begin(), file_headers.end());
        }
        else
        {
            headers.push_back(file.IncludeDir() + ".hpp");
        }
    }
    result.push_back(UmbrellaHeader("include/eolib/protocol.hpp", std::move(headers)));
}

std::vector<std::string> ProtocolGenerator::FileHeaders(const ProtocolFile& file)
{
    const auto layout = FileLayout::Of(file);
    std::vector<std::string> headers;
    if (layout.has_enums)
    {
        headers.push_back(file.IncludePath("enums.hpp"));
    }
    if (layout.has_structs)
    {
        headers.push_back(file.IncludePath("structs.hpp"));
    }
    if (layout.has_packets)
    {
        headers.push_back(file.IncludePath("packets.hpp"));
        headers.push_back(file.IncludePath("packet_factory.hpp"));
    }
    return headers;
}

OutputFile ProtocolGenerator::UmbrellaHeader(const std::string& path, std::vector<std::string> headers)
{
    std::sort(headers.begin(), headers.end());

    GeneratedFile header(path, GeneratedFile::Kind::Header);
    for (const auto& include : headers)
    {
        header.Line("#include \"" + include + "\"");
    }
    return header.ToOutput();
}

} // namespace eolib::generator
