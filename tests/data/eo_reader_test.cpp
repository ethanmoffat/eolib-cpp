#include "eolib/data/eo_reader.hpp"

#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using eolib::data::EoReader;
using eolib::test::Bytes;

namespace
{

// Pads the input with leading/trailing garbage and slices it, to verify that offsets are respected.
EoReader CreateReader(std::initializer_list<int> values)
{
    std::vector<std::uint8_t> data(values.size() + 20);
    std::size_t i = 10;
    for (const int value : values)
    {
        data[i++] = static_cast<std::uint8_t>(value);
    }
    return EoReader(std::move(data)).Slice(10, static_cast<int>(values.size()));
}

EoReader CreateReader(std::string_view str)
{
    return EoReader(str);
}

TEST(EoReaderTest, SliceCreatesReaderWithIndependentLength)
{
    auto reader = CreateReader({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    reader.GetByte();
    reader.SetChunkedReadingMode(true);

    auto reader2 = reader.Slice();

    EXPECT_EQ(reader2.Position(), 0);
    EXPECT_EQ(reader2.Remaining(), 5);
    EXPECT_FALSE(reader2.GetChunkedReadingMode());

    EXPECT_EQ(reader.Position(), 1);
    EXPECT_EQ(reader.Remaining(), 5);
    EXPECT_TRUE(reader.GetChunkedReadingMode());
}

TEST(EoReaderTest, SliceWithIndexCreatesReaderWithIndependentLength)
{
    auto reader = CreateReader({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    reader.GetByte();
    reader.SetChunkedReadingMode(true);

    auto reader2 = reader.Slice();
    auto reader3 = reader2.Slice(1);

    EXPECT_EQ(reader3.Position(), 0);
    EXPECT_EQ(reader3.Remaining(), 4);
    EXPECT_FALSE(reader3.GetChunkedReadingMode());

    EXPECT_EQ(reader.Position(), 1);
    EXPECT_EQ(reader.Remaining(), 5);
    EXPECT_TRUE(reader.GetChunkedReadingMode());
}

TEST(EoReaderTest, SliceWithIndexAndLengthCreatesReaderWithIndependentLength)
{
    auto reader = CreateReader({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    reader.GetByte();
    reader.SetChunkedReadingMode(true);

    auto reader2 = reader.Slice();
    auto reader3 = reader2.Slice(1);
    auto reader4 = reader3.Slice(1, 2);

    EXPECT_EQ(reader4.Position(), 0);
    EXPECT_EQ(reader4.Remaining(), 2);
    EXPECT_FALSE(reader4.GetChunkedReadingMode());

    EXPECT_EQ(reader.Position(), 1);
    EXPECT_EQ(reader.Remaining(), 5);
    EXPECT_TRUE(reader.GetChunkedReadingMode());
}

TEST(EoReaderTest, SliceOfSliceReadsExpectedData)
{
    auto reader = CreateReader({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    reader.GetByte();

    auto reader2 = reader.Slice();
    auto reader3 = reader2.Slice(1);
    auto reader4 = reader3.Slice(1, 2);

    EXPECT_EQ(reader2.GetByte(), 0x02);
    EXPECT_EQ(reader3.GetByte(), 0x03);
    EXPECT_EQ(reader4.GetBytes(10), Bytes({0x04, 0x05}));
}

TEST(EoReaderTest, SliceOverReadDoesNotSeekPastEnd)
{
    auto reader = CreateReader({0x01, 0x02, 0x03});
    EXPECT_EQ(reader.Slice(2, 5).Remaining(), 1);
    EXPECT_EQ(reader.Slice(3).Remaining(), 0);
    EXPECT_EQ(reader.Slice(4).Remaining(), 0);
    EXPECT_EQ(reader.Slice(4, 12345).Remaining(), 0);
}

TEST(EoReaderTest, SliceNegativeIndexThrows)
{
    auto reader = CreateReader({0x01, 0x02, 0x03});
    EXPECT_THROW(reader.Slice(-1), std::invalid_argument);
}

TEST(EoReaderTest, SliceNegativeLengthThrows)
{
    auto reader = CreateReader({0x01, 0x02, 0x03});
    EXPECT_THROW(reader.Slice(0, -1), std::invalid_argument);
}

class EoReaderGetByteTest : public ::testing::TestWithParam<int>
{
};

TEST_P(EoReaderGetByteTest, GetsNextByte)
{
    auto reader = CreateReader({GetParam()});
    EXPECT_EQ(reader.GetByte(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(EoReaderTest, EoReaderGetByteTest,
                         ::testing::Values(0x00, 0x01, 0x02, 0x80, 0xFD, 0xFE, 0xFF));

TEST(EoReaderTest, GetByteOverReadDoesNotSeekPastEnd)
{
    auto reader = CreateReader({});
    EXPECT_EQ(reader.GetByte(), 0);
}

TEST(EoReaderTest, GetBytesReadsMultipleBytes)
{
    auto reader = CreateReader({0x01, 0x02, 0x03, 0x04, 0x05});
    EXPECT_EQ(reader.GetBytes(3), Bytes({0x01, 0x02, 0x03}));
    EXPECT_EQ(reader.GetBytes(10), Bytes({0x04, 0x05}));
    EXPECT_TRUE(reader.GetBytes(1).empty());
}

TEST(EoReaderTest, GetCharGetsAndDecodesNextChar)
{
    auto reader = CreateReader({0x01, 0x02, 0x80, 0x81, 0xFD, 0xFE, 0xFF});
    EXPECT_EQ(reader.GetChar(), 0);
    EXPECT_EQ(reader.GetChar(), 1);
    EXPECT_EQ(reader.GetChar(), 127);
    EXPECT_EQ(reader.GetChar(), 128);
    EXPECT_EQ(reader.GetChar(), 252);
    EXPECT_EQ(reader.GetChar(), 0);
    EXPECT_EQ(reader.GetChar(), 254);
}

TEST(EoReaderTest, GetShortGetsAndDecodesNextShort)
{
    auto reader =
        CreateReader({0x01, 0xFE, 0x02, 0xFE, 0x80, 0xFE, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0x80, 0x7F, 0x7F, 0xFD, 0xFD});
    EXPECT_EQ(reader.GetShort(), 0);
    EXPECT_EQ(reader.GetShort(), 1);
    EXPECT_EQ(reader.GetShort(), 127);
    EXPECT_EQ(reader.GetShort(), 252);
    EXPECT_EQ(reader.GetShort(), 0);
    EXPECT_EQ(reader.GetShort(), 0);
    EXPECT_EQ(reader.GetShort(), 32004);
    EXPECT_EQ(reader.GetShort(), 64008);
}

TEST(EoReaderTest, GetThreeGetsAndDecodesNextThree)
{
    auto reader = CreateReader({0x01, 0xFE, 0xFE, 0x02, 0xFE, 0xFE, 0x80, 0xFE, 0xFE, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE,
                                0xFE, 0xFE, 0x80, 0x81, 0x7F, 0x7F, 0xFE, 0xFD, 0xFD, 0xFE, 0xFD, 0xFD, 0xFD});
    EXPECT_EQ(reader.GetThree(), 0);
    EXPECT_EQ(reader.GetThree(), 1);
    EXPECT_EQ(reader.GetThree(), 127);
    EXPECT_EQ(reader.GetThree(), 252);
    EXPECT_EQ(reader.GetThree(), 0);
    EXPECT_EQ(reader.GetThree(), 0);
    EXPECT_EQ(reader.GetThree(), 32004);
    EXPECT_EQ(reader.GetThree(), 64008);
    EXPECT_EQ(reader.GetThree(), 16194276);
}

TEST(EoReaderTest, GetIntGetsAndDecodesNextInt)
{
    auto reader =
        CreateReader({0x01, 0xFE, 0xFE, 0xFE, 0x02, 0xFE, 0xFE, 0xFE, 0x80, 0xFE, 0xFE, 0xFE, 0xFD, 0xFE, 0xFE,
                      0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x80, 0x81, 0x82, 0x7F, 0x7F, 0xFE, 0xFE, 0xFD, 0xFD,
                      0xFE, 0xFE, 0xFD, 0xFD, 0xFD, 0xFE, 0x7F, 0x7F, 0x7F, 0x7F, 0xFD, 0xFD, 0xFD, 0xFD});
    EXPECT_EQ(reader.GetInt(), 0);
    EXPECT_EQ(reader.GetInt(), 1);
    EXPECT_EQ(reader.GetInt(), 127);
    EXPECT_EQ(reader.GetInt(), 252);
    EXPECT_EQ(reader.GetInt(), 0);
    EXPECT_EQ(reader.GetInt(), 0);
    EXPECT_EQ(reader.GetInt(), 32004);
    EXPECT_EQ(reader.GetInt(), 64008);
    EXPECT_EQ(reader.GetInt(), 16194276);
    EXPECT_EQ(reader.GetInt(), 2'048'576'040);
    EXPECT_EQ(static_cast<unsigned int>(reader.GetInt()), 4'097'152'080U);
}

TEST(EoReaderTest, GetString)
{
    auto reader = CreateReader("Hello, World!");
    EXPECT_EQ(reader.GetString(), "Hello, World!");
}

TEST(EoReaderTest, GetFixedString)
{
    auto reader = CreateReader("foobar");
    EXPECT_EQ(reader.GetFixedString(3), "foo");
    EXPECT_EQ(reader.GetFixedString(3), "bar");
}

TEST(EoReaderTest, GetPaddedFixedString)
{
    auto reader = CreateReader("foo\xFF"
                               "bar\xFF\xFF\xFF");
    EXPECT_EQ(reader.GetFixedString(4, true), "foo");
    EXPECT_EQ(reader.GetFixedString(6, true), "bar");
}

TEST(EoReaderTest, ChunkedGetStringGetsStringForCurrentChunk)
{
    auto reader = CreateReader("Hello,\xFFWorld!");
    reader.SetChunkedReadingMode(true);

    EXPECT_EQ(reader.GetString(), "Hello,");

    reader.NextChunk();
    EXPECT_EQ(reader.GetString(), "World!");
}

TEST(EoReaderTest, GetNegativeLengthStringThrows)
{
    auto reader = CreateReader("foo");
    EXPECT_THROW(reader.GetFixedString(-1), std::invalid_argument);
}

TEST(EoReaderTest, GetEncodedString)
{
    auto reader = CreateReader("!;a-^H s^3a:)");
    EXPECT_EQ(reader.GetEncodedString(), "Hello, World!");
}

TEST(EoReaderTest, GetFixedEncodedString)
{
    auto reader = CreateReader("^0g[>k");
    EXPECT_EQ(reader.GetFixedEncodedString(3), "foo");
    EXPECT_EQ(reader.GetFixedEncodedString(3), "bar");
}

TEST(EoReaderTest, GetPaddedFixedEncodedString)
{
    auto reader = CreateReader("\xFF"
                               "0^9\xFF\xFF\xFF-l=S>k");
    EXPECT_EQ(reader.GetFixedEncodedString(4, true), "foo");
    EXPECT_EQ(reader.GetFixedEncodedString(6, true), "bar");
    EXPECT_EQ(reader.GetFixedEncodedString(3, true), "baz");
}

TEST(EoReaderTest, ChunkedGetEncodedStringGetsStringForCurrentChunk)
{
    auto reader = CreateReader("E0a3hW\xFF!;a-^H");
    reader.SetChunkedReadingMode(true);

    EXPECT_EQ(reader.GetEncodedString(), "Hello,");

    reader.NextChunk();
    EXPECT_EQ(reader.GetEncodedString(), "World!");
}

TEST(EoReaderTest, GetNegativeLengthEncodedStringThrows)
{
    auto reader = CreateReader("^0g");
    EXPECT_THROW(reader.GetFixedEncodedString(-1), std::invalid_argument);
}

TEST(EoReaderTest, RemainingHasCorrectValue)
{
    auto reader = CreateReader({0x01, 0x03, 0x04, 0xFE, 0x05, 0xFE, 0xFE, 0x06, 0xFE, 0xFE, 0xFE});
    EXPECT_EQ(reader.Remaining(), 11);

    reader.GetByte();
    EXPECT_EQ(reader.Remaining(), 10);

    reader.GetChar();
    EXPECT_EQ(reader.Remaining(), 9);

    reader.GetShort();
    EXPECT_EQ(reader.Remaining(), 7);

    reader.GetThree();
    EXPECT_EQ(reader.Remaining(), 4);

    reader.GetInt();
    EXPECT_EQ(reader.Remaining(), 0);

    reader.GetChar();
    EXPECT_EQ(reader.Remaining(), 0);
}

TEST(EoReaderTest, ChunkedRemainingHasValueForCurrentChunkSize)
{
    auto reader = CreateReader({0x01, 0x03, 0x04, 0xFF, 0x05, 0xFE, 0xFE, 0x06, 0xFE, 0xFE, 0xFE});
    EXPECT_EQ(reader.Remaining(), 11);

    reader.SetChunkedReadingMode(true);
    EXPECT_EQ(reader.Remaining(), 3);

    reader.GetChar();
    reader.GetShort();
    EXPECT_EQ(reader.Remaining(), 0);

    reader.GetChar();
    EXPECT_EQ(reader.Remaining(), 0);

    reader.NextChunk();
    EXPECT_EQ(reader.Remaining(), 7);
}

TEST(EoReaderTest, NextChunkMovesToNextChunk)
{
    auto reader = CreateReader({0x01, 0x02, 0xFF, 0x03, 0x04, 0x05, 0xFF, 0x06});

    reader.SetChunkedReadingMode(true);
    EXPECT_EQ(reader.Position(), 0);

    reader.NextChunk();
    EXPECT_EQ(reader.Position(), 3);

    reader.NextChunk();
    EXPECT_EQ(reader.Position(), 7);

    reader.NextChunk();
    EXPECT_EQ(reader.Position(), 8);

    reader.NextChunk();
    EXPECT_EQ(reader.Position(), 8);
}

TEST(EoReaderTest, NextChunkNotInChunkedReadingModeThrows)
{
    auto reader = CreateReader({0x01, 0x02, 0xFF, 0x03, 0x04, 0x05, 0xFF, 0x06});
    EXPECT_THROW(reader.NextChunk(), std::logic_error);
}

TEST(EoReaderTest, ChunkedReadingToggledBetweenReadsSetsExpectedPosition)
{
    auto reader = CreateReader({0x01, 0x02, 0xFF, 0x03, 0x04, 0x05, 0xFF, 0x06});
    EXPECT_EQ(reader.Position(), 0);

    const int expected[] = {3, 7, 8, 8};
    for (const int position : expected)
    {
        reader.SetChunkedReadingMode(true);
        reader.NextChunk();
        reader.SetChunkedReadingMode(false);
        EXPECT_EQ(reader.Position(), position);
    }
}

TEST(EoReaderTest, ChunkedReadingReadLessThanFullChunkIgnoresGarbageData)
{
    // See: https://github.com/Cirras/eo-protocol/blob/master/docs/chunks.md#1-under-read
    auto reader = CreateReader({0x7C, 0x67, 0x61, 0x72, 0x62, 0x61, 0x67, 0x65, 0xFF, 0xCA, 0x31});
    reader.SetChunkedReadingMode(true);

    EXPECT_EQ(reader.GetChar(), 123);
    reader.NextChunk();
    EXPECT_EQ(reader.GetShort(), 12345);
}

TEST(EoReaderTest, ChunkedReadingReadMoreThanChunkTruncatesResult)
{
    // See: https://github.com/Cirras/eo-protocol/blob/master/docs/chunks.md#2-over-read
    auto reader = CreateReader({0xFF, 0x7C});
    reader.SetChunkedReadingMode(true);

    EXPECT_EQ(reader.GetInt(), 0);
    reader.NextChunk();
    EXPECT_EQ(reader.GetShort(), 123);
}

TEST(EoReaderTest, ChunkedReadingDoubleReadReadsExpectedData)
{
    // See: https://github.com/Cirras/eo-protocol/blob/master/docs/chunks.md#3-double-read
    auto reader = CreateReader({0xFF, 0x7C, 0xCA, 0x31});

    // Reading all 4 bytes of the input data
    EXPECT_EQ(reader.GetInt(), 790222478);

    // Activating chunked mode and seeking to the first break byte with NextChunk(), which actually takes our reader
    // position backwards.
    reader.SetChunkedReadingMode(true);
    reader.NextChunk();
    EXPECT_EQ(reader.GetChar(), 123);
    EXPECT_EQ(reader.GetShort(), 12345);
}

static_assert(std::is_nothrow_move_constructible_v<EoReader>);
static_assert(std::is_nothrow_move_assignable_v<EoReader>);

TEST(EoReaderTest, CopySharesDataWithIndependentPosition)
{
    auto reader = CreateReader({0x01, 0x02, 0x03});
    reader.GetByte();

    auto copy = reader;
    EXPECT_EQ(copy.GetByte(), 0x02);
    EXPECT_EQ(copy.Position(), 2);
    EXPECT_EQ(reader.Position(), 1);
    EXPECT_EQ(reader.GetByte(), 0x02);
}

TEST(EoReaderTest, MoveConstructorTransfersStateAndLeavesSourceEmpty)
{
    auto reader = CreateReader({0x01, 0x02, 0x03});
    reader.GetByte();

    EoReader moved(std::move(reader));
    EXPECT_EQ(moved.Position(), 1);
    EXPECT_EQ(moved.GetByte(), 0x02);

    // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_EQ(reader.Position(), 0);
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(reader.GetByte(), 0x00);
    EXPECT_TRUE(reader.GetBytes(3).empty());
    EXPECT_EQ(reader.GetString(), "");
    EXPECT_EQ(reader.Slice().Remaining(), 0);
    reader.SetChunkedReadingMode(true);
    reader.NextChunk();
    EXPECT_EQ(reader.Remaining(), 0);
    // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST(EoReaderTest, MoveAssignmentTransfersStateAndLeavesSourceEmpty)
{
    auto reader = CreateReader({0x01, 0x02, 0x03});
    reader.GetByte();

    auto target = CreateReader({0x04});
    target = std::move(reader);
    EXPECT_EQ(target.Position(), 1);
    EXPECT_EQ(target.GetByte(), 0x02);

    // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(reader.GetByte(), 0x00);
    // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

} // namespace

TEST(EoReaderTest, ViewReadsDataWithoutCopying)
{
    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03};
    auto reader = EoReader::View(data);
    data[1] = 0x05;

    EXPECT_EQ(reader.GetByte(), 0x01);
    EXPECT_EQ(reader.GetByte(), 0x05);
    EXPECT_EQ(reader.Remaining(), 1);
}

TEST(EoReaderTest, ViewOverStringView)
{
    const std::string data = "ab\xFF"
                             "c";
    auto reader = EoReader::View(std::string_view(data));
    reader.SetChunkedReadingMode(true);
    EXPECT_EQ(reader.GetString(), "ab");
    reader.NextChunk();
    EXPECT_EQ(reader.GetString(), "c");
}

TEST(EoReaderTest, ViewOverPointerAndLength)
{
    const std::uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    auto reader = EoReader::View(data, 2);
    EXPECT_EQ(reader.GetBytes(4), (std::vector<std::uint8_t>{0x01, 0x02}));
}

TEST(EoReaderTest, ViewOverEmptyData)
{
    auto reader = EoReader::View(nullptr, 0);
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(reader.GetByte(), 0);
    EXPECT_EQ(reader.GetString(), "");
    reader.SetChunkedReadingMode(true);
    reader.NextChunk();
    EXPECT_EQ(reader.Slice(0).Remaining(), 0);
}

TEST(EoReaderTest, SliceOfViewRefersToSameData)
{
    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    const auto reader = EoReader::View(data);
    auto slice = reader.Slice(1, 2);
    data[2] = 0x09;

    EXPECT_EQ(slice.GetBytes(3), (std::vector<std::uint8_t>{0x02, 0x09}));
}

TEST(EoReaderTest, SliceOutlivesOwningReader)
{
    std::optional<EoReader> slice;
    {
        const EoReader reader(std::vector<std::uint8_t>{0x01, 0x02, 0x03});
        slice = reader.Slice(1);
    }
    EXPECT_EQ(slice->GetBytes(2), (std::vector<std::uint8_t>{0x02, 0x03}));
}
