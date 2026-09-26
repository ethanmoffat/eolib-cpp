#include "eolib/encrypt/data_encrypter.hpp"

#include <gtest/gtest.h>

#include <ostream>

#include <stdexcept>
#include <string>
#include <vector>

using eolib::encrypt::DataEncrypter;
using namespace std::string_literals;

namespace
{

struct EncrypterCase
{
    std::string input;
    std::string expected;
};

struct SwapMultiplesCase
{
    std::string input;
    int multiple;
    std::string expected;
};

void PrintTo(const EncrypterCase& c, std::ostream* os)
{
    *os << ::testing::PrintToString(c.input);
}

void PrintTo(const SwapMultiplesCase& c, std::ostream* os)
{
    *os << ::testing::PrintToString(c.input) << " (multiple " << c.multiple << ")";
}

std::vector<std::uint8_t> ToBytes(const std::string& str)
{
    return std::vector<std::uint8_t>(str.begin(), str.end());
}

std::string FromBytes(const std::vector<std::uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

// Strings are Windows-1252 encoded byte sequences.
// clang-format off
const EncrypterCase kInterleaveCases[] = {
    {"Hello, World!"s, "H!edlllroo,W "s},
    {"We're \xBC of the way there, so \xBE is remaining."s, "W.eg'nrien i\xBC" "a moefr  tshie  \xBEw aoys  t,heer"s},
    {"64\xB2 = 4096"s, "6649\xB2" "0 4= "s},
    {"\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014"s, "\xA9" "4 1F0\xD2" "2\xD6  ZB\xC5\xC3" "BR "s},
    {"\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s, "\xD6\"x\x9Fx\"\xF6  -X \xF6\"\xF6\xEBx\xE4 S\" Lh\xEBt\xEF"s},
    {"Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s, "P\xFF" "a\xFF" "d\xFF" "d\xFF" "e\xFF" "d\xFF \xFFw\xFFiFtFhx 0"s},
    {"This string contains NUL\x00 (value 0) and a \x80 (value 128)"s, "T)h8i2s1  seturlianvg(  c\x80o nat adinnas  )N0U Le\x00u l(av"s},
};

const EncrypterCase kDeinterleaveCases[] = {
    {"Hello, World!"s, "Hlo ol!drW,le"s},
    {"We're \xBC of the way there, so \xBE is remaining."s, "W'e\xBCo h a hr,s  srmiig.nnae i\xBEo eetywetf  re"s},
    {"64\xB2 = 4096"s, "6\xB2=4960  4"s},
    {"\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014"s, "\xA9" "F\xD6" "BRBZ2140 \xC5 \xC3 \xD2 "s},
    {"\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s, "\xD6x \xF6x\"\xEBt \xE4\"-\"\"\x9F  \xEBSh\xEFL \xF6X\xF6x"s},
    {"Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s, "Pde ih0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" "Fx twdda"s},
    {"This string contains NUL\x00 (value 0) and a \x80 (value 128)"s, "Ti tigcnan U\x00(au )ada\x80(au 2)81elv   n 0elv LNsito nrssh"s},
};

const EncrypterCase kFlipMsbCases[] = {
    {"Hello, World!"s, "\xC8\xE5\xEC\xEC\xEF\xAC\xA0\xD7\xEF\xF2\xEC\xE4\xA1"s},
    {"We're \xBC of the way there, so \xBE is remaining."s, "\xD7\xE5\xA7\xF2\xE5\xA0<\xA0\xEF\xE6\xA0\xF4\xE8\xE5\xA0\xF7\xE1\xF9\xA0\xF4\xE8\xE5\xF2\xE5\xAC\xA0\xF3\xEF\xA0>\xA0\xE9\xF3\xA0\xF2\xE5\xED\xE1\xE9\xEE\xE9\xEE\xE7\xAE"s},
    {"64\xB2 = 4096"s, "\xB6\xB4" "2\xA0\xBD\xA0\xB4\xB0\xB9\xB6"s},
    {"\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014"s, ")\xA0\xC6RV\xA0\xC2" "C\xD2\xA0\xC2" "E\xDA\xA0\xB2\xB0\xB1\xB4"s},
    {"\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s, "V\xF8\xF8v\xA0\xD8vv\xF8\xA0\xA2\xCCko\xF4\xE8\xA0\xD3" "dk\xA2\xA0\xAD\xA0\xA2\x1F\xA2"s},
    {"Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s, "\xD0\xE1\xE4\xE4\xE5\xE4\xA0\xF7\xE9\xF4\xE8\xA0\xB0\xF8\xC6\xC6\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F"s},
    {"This string contains NUL\x00 (value 0) and a \x80 (value 128)"s, "\xD4\xE8\xE9\xF3\xA0\xF3\xF4\xF2\xE9\xEE\xE7\xA0\xE3\xEF\xEE\xF4\xE1\xE9\xEE\xF3\xA0\xCE\xD5\xCC\x00\xA0\xA8\xF6\xE1\xEC\xF5\xE5\xA0\xB0\xA9\xA0\xE1\xEE\xE4\xA0\xE1\xA0\x80\xA0\xA8\xF6\xE1\xEC\xF5\xE5\xA0\xB1\xB2\xB8\xA9"s},
};

const SwapMultiplesCase kSwapMultiplesCases[] = {
    {"Hello, World!"s, 3, "Heoll, lroWd!"s},
    {"Hello, World!"s, 0, "Hello, World!"s},
    {"We're \xBC of the way there, so \xBE is remaining."s, 3, "Wer'e \xBC fo the way there, so \xBE is remaining."s},
    {"We're \xBC of the way there, so \xBE is remaining."s, 0, "We're \xBC of the way there, so \xBE is remaining."s},
    {"64\xB2 = 4096"s, 3, "64\xB2 = 4690"s},
    {"64\xB2 = 4096"s, 0, "64\xB2 = 4096"s},
    {"\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014"s, 3, "\xA9 F\xD2\xD6 \xC3" "BR B\xC5Z 2014"s},
    {"\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014"s, 0, "\xA9 F\xD2\xD6 B\xC3R B\xC5Z 2014"s},
    {"\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s, 3, "\xD6\xF6xx Xx\xF6\xF6 \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s},
    {"\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s, 0, "\xD6xx\xF6 X\xF6\xF6x \"L\xEB\xEFth S\xE4\xEB\" - \"\x9F\""s},
    {"Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s, 3, "Padded with x0FF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s},
    {"Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s, 0, "Padded with 0xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"s},
    {"This string contains NUL\x00 (value 0) and a \x80 (value 128)"s, 3, "This stirng ocntains NUL\x00 (vaule 0) and a \x80 (vaule 128)"s},
    {"This string contains NUL\x00 (value 0) and a \x80 (value 128)"s, 0, "This string contains NUL\x00 (value 0) and a \x80 (value 128)"s},
};
// clang-format on

class InterleaveTest : public ::testing::TestWithParam<EncrypterCase>
{
};

TEST_P(InterleaveTest, InterleavesBytes)
{
    EXPECT_EQ(FromBytes(DataEncrypter::Interleave(ToBytes(GetParam().input))), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(DataEncrypterTest, InterleaveTest, ::testing::ValuesIn(kInterleaveCases));

class DeinterleaveTest : public ::testing::TestWithParam<EncrypterCase>
{
};

TEST_P(DeinterleaveTest, DeinterleavesBytes)
{
    EXPECT_EQ(FromBytes(DataEncrypter::Deinterleave(ToBytes(GetParam().input))), GetParam().expected);
}

TEST_P(DeinterleaveTest, ReversesInterleave)
{
    const auto bytes = ToBytes(GetParam().input);
    EXPECT_EQ(DataEncrypter::Deinterleave(DataEncrypter::Interleave(bytes)), bytes);
}

INSTANTIATE_TEST_SUITE_P(DataEncrypterTest, DeinterleaveTest, ::testing::ValuesIn(kDeinterleaveCases));

class FlipMsbTest : public ::testing::TestWithParam<EncrypterCase>
{
};

TEST_P(FlipMsbTest, FlipsMostSignificantBit)
{
    EXPECT_EQ(FromBytes(DataEncrypter::FlipMsb(ToBytes(GetParam().input))), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(DataEncrypterTest, FlipMsbTest, ::testing::ValuesIn(kFlipMsbCases));

class SwapMultiplesTest : public ::testing::TestWithParam<SwapMultiplesCase>
{
};

TEST_P(SwapMultiplesTest, SwapsBytesThatAreMultiplesOfValue)
{
    const auto& param = GetParam();
    EXPECT_EQ(FromBytes(DataEncrypter::SwapMultiples(ToBytes(param.input), param.multiple)), param.expected);
}

INSTANTIATE_TEST_SUITE_P(DataEncrypterTest, SwapMultiplesTest, ::testing::ValuesIn(kSwapMultiplesCases));

TEST(DataEncrypterTest, SwapMultiplesNegativeMultipleThrows)
{
    EXPECT_THROW(DataEncrypter::SwapMultiples(ToBytes("foo"), -1), std::invalid_argument);
}

TEST(DataEncrypterTest, EmptyInput)
{
    const std::vector<std::uint8_t> empty;
    EXPECT_TRUE(DataEncrypter::Interleave(empty).empty());
    EXPECT_TRUE(DataEncrypter::Deinterleave(empty).empty());
    EXPECT_TRUE(DataEncrypter::FlipMsb(empty).empty());
    EXPECT_TRUE(DataEncrypter::SwapMultiples(empty, 3).empty());
}

} // namespace
