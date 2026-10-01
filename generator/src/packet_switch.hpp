#pragma once

#include "code_writer.hpp"
#include "model.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace eolib::generator
{

/// Writes a switch statement over the packet family and action of the packets in a protocol file: a `switch (family)`
/// with a nested `switch (action)` for each family. Families are in order of first appearance in the file.
class PacketSwitch
{
public:
    /// Writes the case labels and statements for the packets of a family, inside the `switch (action)`.
    using CaseWriter = std::function<void(CodeWriter& writer, const std::vector<const ProtocolPacket*>& packets)>;

    /// Creates the switch for the packets in a file. enum_prefix qualifies the PacketFamily and PacketAction enums in
    /// case labels, e.g. "eolib::protocol::net::", or is empty if they don't need to be qualified.
    PacketSwitch(const ProtocolFile& file, std::string enum_prefix);

    /// Writes the switch. fallback is the statement for the default cases, e.g. "return nullptr;".
    void Write(CodeWriter& writer, const CaseWriter& write_cases, const std::string& fallback) const;

    /// Gets the case label for the action of a packet, e.g. "case PacketAction::Request:".
    std::string ActionLabel(const ProtocolPacket& packet) const;

private:
    std::string enum_prefix_;
    std::vector<std::pair<std::string, std::vector<const ProtocolPacket*>>> families_;
};

} // namespace eolib::generator
