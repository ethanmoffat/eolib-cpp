// Tests for the serialization rules described in eo-protocol/docs (chunks.md, elements.md and types.md), exercised
// through generated protocol types.

#include "captured_packet_properties.hpp"
#include "test_utils.hpp"

#include "eolib/protocol.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

using namespace eolib;
using namespace eolib::protocol;
using namespace eolib::protocol::net;

namespace
{

template <typename T>
std::vector<std::uint8_t> SerializeToBytes(const T& object)
{
    data::EoWriter writer;
    object.Serialize(writer);
    return writer.ToByteArray();
}

template <typename T>
T DeserializeFromBytes(const std::vector<std::uint8_t>& bytes)
{
    data::EoReader reader(bytes);
    T result;
    result.Deserialize(reader);
    return result;
}

} // namespace

// chunks.md: strings in chunked sections are sanitized, so they can't contain break bytes.
TEST(ProtocolRulesTest, ChunkedStringsAreSanitized)
{
    client::AccountCreateClientPacket packet;
    packet.username = "a\xFF"
                      "b";
    const auto bytes = SerializeToBytes(packet);

    data::EoWriter expected;
    expected.AddShort(0);
    expected.AddByte(0xFF);
    expected.AddString("ayb");
    for (int i = 0; i < 7; ++i)
    {
        expected.AddByte(0xFF);
    }
    EXPECT_EQ(bytes, expected.ToByteArray());
}

TEST(ProtocolRulesTest, UnchunkedStringsAreNotSanitized)
{
    client::AccountRequestClientPacket packet;
    packet.username = "a\xFF"
                      "b";
    EXPECT_EQ(SerializeToBytes(packet), test::Bytes({'a', 0xFF, 'b'}));
}

TEST(ProtocolRulesTest, SanitizationIsRestoredAfterChunkedSection)
{
    data::EoWriter writer;
    client::AccountCreateClientPacket().Serialize(writer);
    EXPECT_FALSE(writer.GetStringSanitization());
}

// chunks.md: under-reading a chunk skips the remaining data up to the next break.
TEST(ProtocolRulesTest, UnderReadChunkSkipsToNextBreak)
{
    data::EoWriter writer;
    writer.AddShort(10);
    writer.AddString("garbage");
    writer.AddByte(0xFF);
    writer.AddShort(20);
    writer.AddByte(0xFF);
    writer.AddString("a");
    writer.AddByte(0xFF);
    writer.AddString("b");
    writer.AddByte(0xFF);
    writer.AddString("c");

    const auto packet = DeserializeFromBytes<client::CitizenReplyClientPacket>(writer.ToByteArray());
    EXPECT_EQ(packet.session_id, 10);
    EXPECT_EQ(packet.behavior_id, 20);
    EXPECT_EQ(packet.answers, (std::vector<std::string>{"a", "b", "c"}));
}

// chunks.md: over-reading a chunk is truncated at the next break, so later chunks are unaffected.
TEST(ProtocolRulesTest, OverReadChunkIsTruncatedAtBreak)
{
    data::EoWriter writer;
    writer.AddChar(10); // short session_id, truncated to one byte
    writer.AddByte(0xFF);
    writer.AddShort(20);
    writer.AddByte(0xFF);
    writer.AddString("a");
    writer.AddByte(0xFF);
    writer.AddString("b");
    writer.AddByte(0xFF);
    writer.AddString("c");

    const auto packet = DeserializeFromBytes<client::CitizenReplyClientPacket>(writer.ToByteArray());
    EXPECT_EQ(packet.session_id, 10);
    EXPECT_EQ(packet.behavior_id, 20);
    EXPECT_EQ(packet.answers, (std::vector<std::string>{"a", "b", "c"}));
}

TEST(ProtocolRulesTest, ChunkedReadingModeIsRestoredAfterChunkedSection)
{
    data::EoReader reader(test::Bytes({1, 0xFF, 1, 0xFF}));
    client::CitizenReplyClientPacket().Deserialize(reader);
    EXPECT_FALSE(reader.GetChunkedReadingMode());
}

// chunks.md: the PLAYERS_AGREE "double read" - the characters_count byte (0xFF) is also read as a break.
TEST(ProtocolRulesTest, BreakByteCanBeReadAsDataAndBreak)
{
    std::ifstream stream(std::string(EOLIB_CAPTURED_PACKETS_DIR) + "/server/original/PlayersAgree.json");
    ASSERT_TRUE(stream) << "Is the eo-captured-packets submodule initialized?";
    const auto json = nlohmann::json::parse(stream);
    const auto bytes = test::DecodeBase64(json.at("expected").get<std::string>());
    ASSERT_EQ(bytes.at(0), 0xFF);

    const auto packet = DeserializeFromBytes<server::PlayersAgreeServerPacket>(bytes);
    ASSERT_FALSE(packet.nearby.characters.empty());
    EXPECT_EQ(packet.nearby.characters[0].name, "vult-r");
    EXPECT_EQ(packet.nearby.characters[0].player_id, 340);
    EXPECT_EQ(packet.ByteSize(), static_cast<int>(bytes.size()));
}

// elements.md: optional fields are not written when absent, and are only read when data remains.
TEST(ProtocolRulesTest, AbsentOptionalFieldIsNotWritten)
{
    server::AvatarRemoveServerPacket packet;
    packet.player_id = 5;
    EXPECT_EQ(SerializeToBytes(packet).size(), 2u);

    packet.warp_effect = server::WarpEffect::Scroll;
    EXPECT_EQ(SerializeToBytes(packet).size(), 3u);
}

TEST(ProtocolRulesTest, OptionalFieldIsNotReadWithoutData)
{
    data::EoWriter writer;
    writer.AddShort(5);
    const auto packet = DeserializeFromBytes<server::AvatarRemoveServerPacket>(writer.ToByteArray());
    EXPECT_EQ(packet.player_id, 5);
    EXPECT_FALSE(packet.warp_effect.has_value());
}

// elements.md: dummy fields are only written when the rest of the data is empty.
TEST(ProtocolRulesTest, DummyIsWrittenOnlyWhenOtherwiseEmpty)
{
    server::ChestCloseServerPacket packet;
    EXPECT_EQ(SerializeToBytes(packet), test::Bytes("N"));

    packet.key = 7;
    data::EoWriter expected;
    expected.AddShort(7);
    EXPECT_EQ(SerializeToBytes(packet), expected.ToByteArray());
}

TEST(ProtocolRulesTest, DummyIsConsumedOnDeserialize)
{
    data::EoReader reader(test::Bytes("N"));
    server::ChestCloseServerPacket packet;
    packet.Deserialize(reader);
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(packet.ByteSize(), 1);
}

// types.md: bool values are written as 1 or 0, and any non-zero value is read as true.
TEST(ProtocolRulesTest, BoolSerialization)
{
    client::TradeAgreeClientPacket packet;
    packet.agree = true;
    EXPECT_EQ(SerializeToBytes(packet), test::Bytes({2}));
    packet.agree = false;
    EXPECT_EQ(SerializeToBytes(packet), test::Bytes({1}));

    EXPECT_TRUE(DeserializeFromBytes<client::TradeAgreeClientPacket>(test::Bytes({3})).agree);
    EXPECT_TRUE(DeserializeFromBytes<client::TradeAgreeClientPacket>(test::Bytes({2})).agree);
    EXPECT_FALSE(DeserializeFromBytes<client::TradeAgreeClientPacket>(test::Bytes({1})).agree);
}

// types.md: unrecognized enum values are persisted.
TEST(ProtocolRulesTest, UnrecognizedEnumValueIsPersisted)
{
    const auto unknown = static_cast<server::WarpEffect>(100);
    server::AvatarRemoveServerPacket packet;
    packet.player_id = 1;
    packet.warp_effect = unknown;

    const auto result = DeserializeFromBytes<server::AvatarRemoveServerPacket>(SerializeToBytes(packet));
    EXPECT_EQ(result.warp_effect, unknown);
    EXPECT_EQ(server::ToString(unknown), "Unrecognized(100)");
}

// elements.md: an array without a length or delimiter reads as many elements as fit in the remaining data.
TEST(ProtocolRulesTest, UnsizedArrayReadsElementsThatFitRemainingData)
{
    data::EoWriter writer;
    writer.AddShort(1);
    writer.AddThree(100);
    writer.AddShort(2);
    writer.AddThree(200);
    writer.AddShort(3); // incomplete element, ignored

    data::EoReader reader(writer.ToByteArray());
    server::ChestAgreeServerPacket packet;
    packet.Deserialize(reader);
    ASSERT_EQ(packet.items.size(), 2u);
    EXPECT_EQ(packet.items[1].id, 2);
    EXPECT_EQ(packet.items[1].amount, 200);
    EXPECT_EQ(packet.ByteSize(), 10);
    EXPECT_EQ(reader.Remaining(), 2);
}

// elements.md: delimited arrays write a break after each element, except the last if trailing-delimiter is false.
TEST(ProtocolRulesTest, DelimitedArrayWithoutTrailingDelimiter)
{
    client::CitizenReplyClientPacket packet;
    packet.session_id = 1;
    packet.behavior_id = 2;
    packet.answers = {"a", "b", "c"};

    data::EoWriter expected;
    expected.AddShort(1);
    expected.AddByte(0xFF);
    expected.AddShort(2);
    expected.AddByte(0xFF);
    expected.AddString("a");
    expected.AddByte(0xFF);
    expected.AddString("b");
    expected.AddByte(0xFF);
    expected.AddString("c");
    EXPECT_EQ(SerializeToBytes(packet), expected.ToByteArray());
}

TEST(ProtocolRulesTest, DelimitedArrayWithTrailingDelimiterAndLengthField)
{
    server::PlayersListFriends list;
    list.players = {"a", "b"};

    data::EoWriter expected;
    expected.AddShort(2);
    expected.AddByte(0xFF);
    expected.AddString("a");
    expected.AddByte(0xFF);
    expected.AddString("b");
    expected.AddByte(0xFF);
    const auto bytes = SerializeToBytes(list);
    EXPECT_EQ(bytes, expected.ToByteArray());
    EXPECT_EQ(DeserializeFromBytes<server::PlayersListFriends>(bytes).players, list.players);
}

TEST(ProtocolRulesTest, DelimitedArrayWithoutLengthReadsUntilEndOfData)
{
    data::EoWriter writer;
    writer.AddShort(9);
    writer.AddByte(0xFF);
    writer.AddString("hello");
    writer.AddByte(0xFF);
    writer.AddString("world");
    writer.AddByte(0xFF);

    const auto packet = DeserializeFromBytes<server::QuestReportServerPacket>(writer.ToByteArray());
    EXPECT_EQ(packet.npc_index, 9);
    EXPECT_EQ(packet.messages, (std::vector<std::string>{"hello", "world"}));
}
