#include "packet_factory_generator.hpp"

#include "names.hpp"
#include "packet_switch.hpp"

namespace eolib::generator
{

PacketFactoryGenerator::PacketFactoryGenerator(TypeRegistry& types)
    : types_(types)
{
}

void PacketFactoryGenerator::Generate(const ProtocolFile& file, std::vector<OutputFile>& result)
{
    const Type& family_type = types_.Get("PacketFamily");
    const std::string side = file.PacketSide();

    GeneratedFile header("include/" + file.IncludePath("packet_factory.hpp"), GeneratedFile::Kind::Header);
    header.Includes({"eolib/export.hpp", "eolib/data/eo_reader.hpp", "eolib/protocol/net/packet.hpp",
                     family_type.file->IncludePath("enums.hpp")},
                    {"memory"});
    header.BeginNamespace(file.Namespace());
    WriteDeclaration(side, header);
    header.EndNamespace(file.Namespace());

    GeneratedFile source("src/" + file.IncludePath("packet_factory.cpp"), GeneratedFile::Kind::Source);
    source.Includes({file.IncludePath("packet_factory.hpp"), file.IncludePath("packets.hpp")}, {"memory"});
    source.BeginNamespace(file.Namespace());

    const PacketSwitch packet_switch(file, "");

    source.Open("std::unique_ptr<Packet> PacketFactory::Create(PacketFamily family, PacketAction action)");
    packet_switch.Write(
        source,
        [&](CodeWriter& writer, const std::vector<const ProtocolPacket*>& packets)
        {
            for (const auto* packet : packets)
            {
                writer.Line(packet_switch.ActionLabel(*packet));
                writer.Indent();
                writer.Line("return std::make_unique<" + PacketClassName(file, *packet) + ">();");
                writer.Dedent();
            }
        },
        "return nullptr;");
    source.Close();
    source.Line();

    source.Open("bool PacketFactory::Contains(PacketFamily family, PacketAction action) noexcept");
    packet_switch.Write(
        source,
        [&](CodeWriter& writer, const std::vector<const ProtocolPacket*>& packets)
        {
            for (const auto* packet : packets)
            {
                writer.Line(packet_switch.ActionLabel(*packet));
            }
            writer.Indent();
            writer.Line("return true;");
            writer.Dedent();
        },
        "return false;");
    source.Close();
    source.Line();

    source.Open("std::unique_ptr<Packet> PacketFactory::Deserialize(PacketFamily family, PacketAction action, "
                "data::EoReader& reader)");
    source.Line("auto packet = Create(family, action);");
    source.Open("if (packet != nullptr)");
    source.Line("packet->Deserialize(reader);");
    source.Close();
    source.Line("return packet;");
    source.Close();
    source.EndNamespace(file.Namespace());

    result.push_back(header.ToOutput());
    result.push_back(source.ToOutput());
}

void PacketFactoryGenerator::WriteDeclaration(const std::string& side, CodeWriter& header)
{
    header.DocComment("Creates " + side + " packets from packet family and action identifiers.");
    header.Line("class EOLIB_API PacketFactory");
    header.Line("{");
    header.Line("public:");
    header.Indent();
    header.Line("PacketFactory() = delete;");
    header.Line();
    header.DocComment("Creates a default-initialized " + side +
                      " packet with the specified family and action.\n\n"
                      "@param family the packet family.\n"
                      "@param action the packet action.\n"
                      "@return the packet, or nullptr if no " +
                      side + " packet has the specified family and action.");
    header.Line("static std::unique_ptr<Packet> Create(PacketFamily family, PacketAction action);");
    header.Line();
    header.DocComment("Checks whether a " + side +
                      " packet exists with the specified family and action, without creating it.\n\n"
                      "@param family the packet family.\n"
                      "@param action the packet action.\n"
                      "@return true if a " +
                      side + " packet has the specified family and action, otherwise false.");
    header.Line("static bool Contains(PacketFamily family, PacketAction action) noexcept;");
    header.Line();
    header.DocComment("Creates a " + side +
                      " packet with the specified family and action, and deserializes it from the reader.\n\n"
                      "@param family the packet family.\n"
                      "@param action the packet action.\n"
                      "@param reader the reader that the packet data will be read from.\n"
                      "@return the packet, or nullptr if no " +
                      side + " packet has the specified family and action.");
    header.Line("static std::unique_ptr<Packet> Deserialize(PacketFamily family, PacketAction action, "
                "data::EoReader& reader);");
    header.Dedent();
    header.Line("};");
}

} // namespace eolib::generator
