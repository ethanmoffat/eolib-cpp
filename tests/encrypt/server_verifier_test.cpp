#include "eolib/encrypt/server_verifier.hpp"

#include "eolib/data/eo_numeric_limits.hpp"

#include <gtest/gtest.h>

using eolib::data::EoNumericLimits;
using eolib::encrypt::ServerVerifier;

namespace
{

struct ServerVerifierCase
{
    int challenge;
    int expected;
};

class ServerVerifierTest : public ::testing::TestWithParam<ServerVerifierCase>
{
};

TEST_P(ServerVerifierTest, Hash)
{
    EXPECT_EQ(ServerVerifier::Hash(GetParam().challenge), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    ServerVerifier, ServerVerifierTest,
    ::testing::Values(ServerVerifierCase{0, 114000}, ServerVerifierCase{1, 115191}, ServerVerifierCase{2, 229432},
                      ServerVerifierCase{5, 613210}, ServerVerifierCase{12345, 266403},
                      ServerVerifierCase{100'000, 145554}, ServerVerifierCase{5'000'000, 339168},
                      ServerVerifierCase{11'092'003, 112773}, ServerVerifierCase{11'092'004, 112655},
                      ServerVerifierCase{11'092'005, 112299}, ServerVerifierCase{11'092'110, 11016},
                      ServerVerifierCase{11'092'111, -2787}, ServerVerifierCase{11'111'111, 103749},
                      ServerVerifierCase{12'345'678, -32046},
                      ServerVerifierCase{static_cast<int>(EoNumericLimits::ThreeMax) - 1, 105960}));

} // namespace
