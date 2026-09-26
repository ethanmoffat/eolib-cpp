#include "eolib/encrypt/server_verifier.hpp"

namespace eolib::encrypt
{

int ServerVerifier::Hash(int challenge)
{
    ++challenge;
    return 110905 + (challenge % 9 + 1) * ((11092004 - challenge) % ((challenge % 11 + 1) * 119)) * 119 +
           challenge % 2004;
}

} // namespace eolib::encrypt
