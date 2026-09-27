#include "eolib/data/string_encoder.hpp"

#include <gtest/gtest.h>

#include <ostream>

#include <string>
#include <vector>

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
    auto str = GetParam().decoded;
    StringEncoder::EncodeString(str);
    EXPECT_EQ(str, GetParam().encoded);
}

TEST_P(StringEncoderTest, DecodeString)
{
    auto str = GetParam().encoded;
    StringEncoder::DecodeString(str);
    EXPECT_EQ(str, GetParam().decoded);
}

TEST_P(StringEncoderTest, EncodeByteVector)
{
    const auto& param = GetParam();
    std::vector<std::uint8_t> bytes(param.decoded.begin(), param.decoded.end());
    StringEncoder::EncodeString(bytes);
    EXPECT_EQ(bytes, std::vector<std::uint8_t>(param.encoded.begin(), param.encoded.end()));
}

TEST_P(StringEncoderTest, DecodeByteVector)
{
    const auto& param = GetParam();
    std::vector<std::uint8_t> bytes(param.encoded.begin(), param.encoded.end());
    StringEncoder::DecodeString(bytes);
    EXPECT_EQ(bytes, std::vector<std::uint8_t>(param.decoded.begin(), param.decoded.end()));
}

TEST_P(StringEncoderTest, EncodePointer)
{
    const auto& param = GetParam();
    std::vector<std::uint8_t> bytes(param.decoded.begin(), param.decoded.end());
    StringEncoder::EncodeString(bytes.data(), bytes.size());
    EXPECT_EQ(bytes, std::vector<std::uint8_t>(param.encoded.begin(), param.encoded.end()));
}

TEST_P(StringEncoderTest, DecodePointer)
{
    const auto& param = GetParam();
    std::vector<std::uint8_t> bytes(param.encoded.begin(), param.encoded.end());
    StringEncoder::DecodeString(bytes.data(), bytes.size());
    EXPECT_EQ(bytes, std::vector<std::uint8_t>(param.decoded.begin(), param.decoded.end()));
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
