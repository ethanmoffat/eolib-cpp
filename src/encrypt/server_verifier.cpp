#include "eolib/encrypt/server_verifier.hpp"

namespace eolib::encrypt
{

int ServerVerifier::Hash(int challenge)
{
    const int value = challenge + 1;
    const int multiplier = (value % 9) + 1;
    const int remainder = (11092004 - value) % (((value % 11) + 1) * 119);
    return 110905 + (multiplier * remainder * 119) + (value % 2004);
}

} // namespace eolib::encrypt
