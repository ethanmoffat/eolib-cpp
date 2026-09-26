#include "eolib/data/string_encoder.hpp"

#include <gtest/gtest.h>

#include <ostream>

#include <string>

using eolib::data::StringEncoder;

namespace
{

struct StringEncoderCase
{
    std::string decoded;
    std::string encoded;
};

void PrintTo(const StringEncoderCase& c, std::ostream* os)
{
    *os << ::testing::PrintToString(c.decoded);
}

class StringEncoderTest : public ::testing::TestWithParam<StringEncoderCase>
{
};

TEST_P(StringEncoderTest, EncodeString)
{
    EXPECT_EQ(StringEncoder::EncodeString(GetParam().decoded), GetParam().encoded);
}

TEST_P(StringEncoderTest, DecodeString)
{
    EXPECT_EQ(StringEncoder::DecodeString(GetParam().encoded), GetParam().decoded);
}

TEST_P(StringEncoderTest, EncodeByteVector)
{
    const auto& param = GetParam();
    const std::vector<std::uint8_t> input(param.decoded.begin(), param.decoded.end());
    const std::vector<std::uint8_t> expected(param.encoded.begin(), param.encoded.end());
    EXPECT_EQ(StringEncoder::EncodeString(input), expected);
}

// Strings are Windows-1252 encoded byte sequences.
INSTANTIATE_TEST_SUITE_P(
    StringEncoder, StringEncoderTest,
    ::testing::Values(StringEncoderCase{"Hello, World!", "!;a-^H s^3a:)"},
                      StringEncoderCase{"We're \xBC of the way there, so \xBE is remaining.",
                                        "C8_6_6l2h- ,d \xBE ^, sh-h7Y T>V h7Y g0 \xBC :[xhH"},
                      StringEncoderCase{"64\xB2 = 4096", ";fAk b \xB2=i"},
                      StringEncoderCase{"\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014", "=nAm E\xC5] M\xC3] \xD6\xD2Y \xA9"},
                      StringEncoderCase{"\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\"",
                                        "O\x9FO D O\xEB\xE4L 7Y\xEF\xEBSO U\xF6\xF6G \xF6U'\xD6"},
                      StringEncoderCase{"Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",
                                        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF+YUo 7Y6V i:i;lO"}));

} // namespace
