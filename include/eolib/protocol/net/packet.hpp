#pragma once

#include "eolib/export.hpp"
#include "eolib/protocol/net/enums.hpp"
#include "eolib/protocol/serializable.hpp"

namespace eolib::protocol::net
{

/// An object representing a packet in the EO network protocol.
class EOLIB_API Packet : public Serializable
{
public:
    /// Gets the packet family associated with this packet.
    ///
    /// @return the packet family.
    virtual PacketFamily Family() const noexcept = 0;

    /// Gets the packet action associated with this packet.
    ///
    /// @return the packet action.
    virtual PacketAction Action() const noexcept = 0;
};

} // namespace eolib::protocol::net
