#pragma once

#include "eolib/export.hpp"

namespace eolib::encrypt
{

/// Server verification, as performed during the connection handshake.
class EOLIB_API ServerVerifier
{
public:
    ServerVerifier() = delete;

    /// Hashes a challenge value sent by the client, producing the response expected from a genuine server.
    ///
    /// The client sends the challenge in the <c>InitInitClientPacket</c>, and the server responds with the hash in
    /// the <c>InitInitServerPacket</c>.
    ///
    /// @param challenge the challenge value; should be less than <c>EoNumericLimits::ThreeMax</c>.
    static int Hash(int challenge);
};

} // namespace eolib::encrypt
