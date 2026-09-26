#include "eolib/data/eo_writer.hpp"

#include "eolib/data/eo_numeric_limits.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using eolib::data::EoNumericLimits;
using eolib::data::EoWriter;
using eolib::test::Bytes;

namespace
{

EoWriter SanitizedWriter()
{
    EoWriter writer;
    writer.SetStringSanitization(true);
    return writer;
}

TEST(EoWriterTest, AddByte)
{
    EoWriter writer;
    writer.AddByte(0x00);
    EXPECT_EQ(writer.ToByteArray(), Bytes({0x00}));
}

TEST(EoWriterTest, AddBytes)
{
    EoWriter writer;
    writer.AddBytes(Bytes({0x00, 0xFF}));
    EXPECT_EQ(writer.ToByteArray(), Bytes({0x00, 0xFF}));
}

TEST(EoWriterTest, AddChar)
{
    EoWriter writer;
    writer.AddChar(123);
    EXPECT_EQ(writer.ToByteArray(), Bytes({0x7C}));
}

TEST(EoWriterTest, AddShort)
{
    EoWriter writer;
    writer.AddShort(12345);
    EXPECT_EQ(writer.ToByteArray(), Bytes({0xCA, 0x31}));
}

TEST(EoWriterTest, AddThree)
{
    EoWriter writer;
    writer.AddThree(10'000'000);
    EXPECT_EQ(writer.ToByteArray(), Bytes({0xB0, 0x3A, 0x9D}));
}

TEST(EoWriterTest, AddInt)
{
    EoWriter writer;
    writer.AddInt(2'048'576'040);
    EXPECT_EQ(writer.ToByteArray(), Bytes({0x7F, 0x7F, 0x7F, 0x7F}));
}

TEST(EoWriterTest, AddString)
{
    EoWriter writer;
    writer.AddString("foo");
    EXPECT_EQ(writer.ToByteArray(), Bytes("foo"));
}

TEST(EoWriterTest, AddFixedString)
{
    EoWriter writer;
    writer.AddFixedString("bar", 3);
    EXPECT_EQ(writer.ToByteArray(), Bytes("bar"));
}

TEST(EoWriterTest, AddPaddedFixedString)
{
    EoWriter writer;
    writer.AddFixedString("bar", 6, true);
    EXPECT_EQ(writer.ToByteArray(), Bytes("bar\xFF\xFF\xFF"));
}

TEST(EoWriterTest, AddPaddedWithPerfectFitFixedString)
{
    EoWriter writer;
    writer.AddFixedString("bar", 3, true);
    EXPECT_EQ(writer.ToByteArray(), Bytes("bar"));
}

TEST(EoWriterTest, AddEncodedString)
{
    EoWriter writer;
    writer.AddEncodedString("foo");
    EXPECT_EQ(writer.ToByteArray(), Bytes("^0g"));
}

TEST(EoWriterTest, AddFixedEncodedString)
{
    EoWriter writer;
    writer.AddFixedEncodedString("bar", 3);
    EXPECT_EQ(writer.ToByteArray(), Bytes("[>k"));
}

TEST(EoWriterTest, AddPaddedFixedEncodedString)
{
    EoWriter writer;
    writer.AddFixedEncodedString("bar", 6, true);
    EXPECT_EQ(writer.ToByteArray(), Bytes("\xFF\xFF\xFF-l="));
}

TEST(EoWriterTest, AddPaddedWithPerfectFitFixedEncodedString)
{
    EoWriter writer;
    writer.AddFixedEncodedString("bar", 3, true);
    EXPECT_EQ(writer.ToByteArray(), Bytes("[>k"));
}

TEST(EoWriterTest, AddSanitizedString)
{
    auto writer = SanitizedWriter();
    writer.AddString("a\xFFz");
    EXPECT_EQ(writer.ToByteArray(), Bytes("ayz"));
}

TEST(EoWriterTest, AddSanitizedFixedString)
{
    auto writer = SanitizedWriter();
    writer.AddFixedString("a\xFFz", 3);
    EXPECT_EQ(writer.ToByteArray(), Bytes("ayz"));
}

TEST(EoWriterTest, AddSanitizedPaddedFixedString)
{
    auto writer = SanitizedWriter();
    writer.AddFixedString("a\xFFz", 6, true);
    EXPECT_EQ(writer.ToByteArray(), Bytes("ayz\xFF\xFF\xFF"));
}

TEST(EoWriterTest, AddSanitizedEncodedString)
{
    auto writer = SanitizedWriter();
    writer.AddEncodedString("a\xFFz");
    EXPECT_EQ(writer.ToByteArray(), Bytes("S&l"));
}

TEST(EoWriterTest, AddSanitizedFixedEncodedString)
{
    auto writer = SanitizedWriter();
    writer.AddFixedEncodedString("a\xFFz", 3);
    EXPECT_EQ(writer.ToByteArray(), Bytes("S&l"));
}

TEST(EoWriterTest, AddSanitizedPaddedFixedEncodedString)
{
    auto writer = SanitizedWriter();
    writer.AddFixedEncodedString("a\xFFz", 6, true);
    EXPECT_EQ(writer.ToByteArray(), Bytes("\xFF\xFF\xFF%T>"));
}

TEST(EoWriterTest, AddNumbersOnBoundary)
{
    EoWriter writer;
    EXPECT_NO_THROW(writer.AddByte(0xFF));
    EXPECT_NO_THROW(writer.AddChar(static_cast<int>(EoNumericLimits::CharMax - 1)));
    EXPECT_NO_THROW(writer.AddShort(static_cast<int>(EoNumericLimits::ShortMax - 1)));
    EXPECT_NO_THROW(writer.AddThree(static_cast<int>(EoNumericLimits::ThreeMax - 1)));
    EXPECT_NO_THROW(writer.AddInt(static_cast<int>(EoNumericLimits::IntMax - 1)));
}

TEST(EoWriterTest, AddNumbersExceedingLimit)
{
    EoWriter writer;
    EXPECT_THROW(writer.AddByte(256), std::invalid_argument);
    EXPECT_THROW(writer.AddChar(static_cast<int>(EoNumericLimits::CharMax)), std::invalid_argument);
    EXPECT_THROW(writer.AddShort(static_cast<int>(EoNumericLimits::ShortMax)), std::invalid_argument);
    EXPECT_THROW(writer.AddThree(static_cast<int>(EoNumericLimits::ThreeMax)), std::invalid_argument);
    EXPECT_THROW(writer.AddInt(static_cast<int>(EoNumericLimits::IntMax)), std::invalid_argument);
}

TEST(EoWriterTest, AddNegativeNumbers)
{
    EoWriter writer;
    EXPECT_THROW(writer.AddByte(-1), std::invalid_argument);
    EXPECT_THROW(writer.AddChar(-1), std::invalid_argument);
    EXPECT_THROW(writer.AddShort(-1), std::invalid_argument);
    EXPECT_THROW(writer.AddThree(-1), std::invalid_argument);
}

TEST(EoWriterTest, AddFixedStringWithIncorrectLength)
{
    EoWriter writer;
    EXPECT_THROW(writer.AddFixedString("foo", 2), std::invalid_argument);
    EXPECT_THROW(writer.AddFixedString("foo", 2, true), std::invalid_argument);
    EXPECT_THROW(writer.AddFixedString("foo", 4), std::invalid_argument);
    EXPECT_THROW(writer.AddFixedEncodedString("foo", 2), std::invalid_argument);
    EXPECT_THROW(writer.AddFixedEncodedString("foo", 2, true), std::invalid_argument);
    EXPECT_THROW(writer.AddFixedEncodedString("foo", 4), std::invalid_argument);
}

TEST(EoWriterTest, Length)
{
    EoWriter writer;
    EXPECT_EQ(writer.Length(), 0);

    writer.AddString("Lorem ipsum dolor sit amet");
    EXPECT_EQ(writer.Length(), 26);

    for (int i = 27; i <= 100; ++i)
    {
        writer.AddByte(0xFF);
    }
    EXPECT_EQ(writer.Length(), 100);
}

TEST(EoWriterTest, ToByteString)
{
    EoWriter writer;
    writer.AddString("foo");
    writer.AddByte(0xFF);
    EXPECT_EQ(writer.ToByteString(), std::string("foo\xFF"));
}

} // namespace
