#include "packet_switch.hpp"

#include <algorithm>

namespace eolib::generator
{

PacketSwitch::PacketSwitch(const ProtocolFile& file, std::string enum_prefix)
    : enum_prefix_(std::move(enum_prefix))
{
    for (const auto& packet : file.packets)
    {
        auto family = std::find_if(families_.begin(), families_.end(),
                                   [&](const auto& entry) { return entry.first == packet.family; });
        if (family == families_.end())
        {
            family = families_.emplace(families_.end(), packet.family, std::vector<const ProtocolPacket*>{});
        }
        family->second.push_back(&packet);
    }
}

void PacketSwitch::Write(CodeWriter& writer, const CaseWriter& write_cases, const std::string& fallback) const
{
    writer.Open("switch (family)");
    for (const auto& [family, packets] : families_)
    {
        writer.Line("case " + enum_prefix_ + "PacketFamily::" + family + ":");
        writer.Indent();
        writer.Open("switch (action)");
        write_cases(writer, packets);
        writer.Line("default:");
        writer.Indent();
        writer.Line(fallback);
        writer.Dedent();
        writer.Close();
        writer.Dedent();
    }
    writer.Line("default:");
    writer.Indent();
    writer.Line(fallback);
    writer.Dedent();
    writer.Close();
}

std::string PacketSwitch::ActionLabel(const ProtocolPacket& packet) const
{
    return "case " + enum_prefix_ + "PacketAction::" + packet.action + ":";
}

} // namespace eolib::generator
