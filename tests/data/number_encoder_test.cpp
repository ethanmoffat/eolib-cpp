#include "eolib/data/number_encoder.hpp"

#include <gtest/gtest.h>

#include <ostream>

#include <array>
#include <cstdint>
#include <vector>

using eolib::data::NumberEncoder;

namespace
{

struct NumberEncoderCase
{
    unsigned int number;
    std::array<std::uint8_t, 4> bytes;
};

void PrintTo(const NumberEncoderCase& c, std::ostream* os)
{
    *os << c.number;
}

class NumberEncoderTest : public ::testing::TestWithParam<NumberEncoderCase>
{
};

TEST_P(NumberEncoderTest, EncodeNumber)
{
    const auto& param = GetParam();
    EXPECT_EQ(NumberEncoder::EncodeNumber(static_cast<int>(param.number)), param.bytes);
}

TEST_P(NumberEncoderTest, DecodeNumber)
{
    const auto& param = GetParam();
    const std::vector<std::uint8_t> bytes(param.bytes.begin(), param.bytes.end());
    EXPECT_EQ(static_cast<unsigned int>(NumberEncoder::DecodeNumber(bytes)), param.number);
}

INSTANTIATE_TEST_SUITE_P(
    NumberEncoder, NumberEncoderTest,
    ::testing::Values(
        NumberEncoderCase{0, {0x01, 0xFE, 0xFE, 0xFE}}, NumberEncoderCase{1, {0x02, 0xFE, 0xFE, 0xFE}},
        NumberEncoderCase{28, {0x1D, 0xFE, 0xFE, 0xFE}}, NumberEncoderCase{100, {0x65, 0xFE, 0xFE, 0xFE}},
        NumberEncoderCase{128, {0x81, 0xFE, 0xFE, 0xFE}}, NumberEncoderCase{252, {0xFD, 0xFE, 0xFE, 0xFE}},
        NumberEncoderCase{253, {0x01, 0x02, 0xFE, 0xFE}}, NumberEncoderCase{254, {0x02, 0x02, 0xFE, 0xFE}},
        NumberEncoderCase{255, {0x03, 0x02, 0xFE, 0xFE}}, NumberEncoderCase{32003, {0x7E, 0x7F, 0xFE, 0xFE}},
        NumberEncoderCase{32004, {0x7F, 0x7F, 0xFE, 0xFE}}, NumberEncoderCase{32005, {0x80, 0x7F, 0xFE, 0xFE}},
        NumberEncoderCase{64008, {0xFD, 0xFD, 0xFE, 0xFE}}, NumberEncoderCase{64009, {0x01, 0x01, 0x02, 0xFE}},
        NumberEncoderCase{64010, {0x02, 0x01, 0x02, 0xFE}}, NumberEncoderCase{10'000'000, {0xB0, 0x3A, 0x9D, 0xFE}},
        NumberEncoderCase{16'194'276, {0xFD, 0xFD, 0xFD, 0xFE}},
        NumberEncoderCase{16'194'277, {0x01, 0x01, 0x01, 0x02}},
        NumberEncoderCase{16'194'278, {0x02, 0x01, 0x01, 0x02}},
        NumberEncoderCase{2'048'576'039, {0x7E, 0x7F, 0x7F, 0x7F}},
        NumberEncoderCase{2'048'576'040, {0x7F, 0x7F, 0x7F, 0x7F}},
        NumberEncoderCase{2'048'576'041, {0x80, 0x7F, 0x7F, 0x7F}},
        NumberEncoderCase{4'097'152'079U, {0xFC, 0xFD, 0xFD, 0xFD}},
        NumberEncoderCase{4'097'152'080U, {0xFD, 0xFD, 0xFD, 0xFD}}));

// EO numbers are represented as int (like eolib-dotnet and eolib-java). These tests lock in how values that don't fit
// the non-negative int range are represented.

TEST(NumberEncoderSignednessTest, IntAboveIntMaxWrapsToNegative)
{
    const int decoded = NumberEncoder::DecodeNumber(std::vector<std::uint8_t>{0xFD, 0xFD, 0xFD, 0xFD});
    EXPECT_EQ(decoded, -197'815'216); // 4,097,152,080 - 2^32
    EXPECT_EQ(NumberEncoder::EncodeNumber(decoded), (std::array<std::uint8_t, 4>{0xFD, 0xFD, 0xFD, 0xFD}));
}

TEST(NumberEncoderSignednessTest, ZeroBytesDecodeToNegativeValues)
{
    // 0x00 is not a valid encoded byte; like the official client and other eolibs, it decodes as -1.
    EXPECT_EQ(NumberEncoder::DecodeNumber(std::vector<std::uint8_t>{0x00}), -1);
    EXPECT_EQ(NumberEncoder::DecodeNumber(std::vector<std::uint8_t>{0x01, 0x00}), -253);
}

} // namespace
