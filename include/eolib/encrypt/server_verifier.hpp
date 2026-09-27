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
    ///
    /// @warning Challenges larger than <b>11,092,110</b> may produce negative hash values that cannot be properly
    /// represented in the EO protocol (which uses unsigned integers). Keep challenges below this value to avoid
    /// overflow issues.
    ///
    /// @return the hashed challenge value
    ///
    /// @see InitInitClientPacket
    /// @see InitInitServerPacket
    [[nodiscard]] static int Hash(int challenge);
};

} // namespace eolib::encrypt
