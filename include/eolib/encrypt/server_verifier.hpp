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
    /// @param challenge the challenge value; should be no larger than 11,092,110, since larger values may produce
    ///                  negative hashes that can't be represented in the EO protocol.
    /// @return the hashed challenge value.
    static int Hash(int challenge);
};

} // namespace eolib::encrypt
