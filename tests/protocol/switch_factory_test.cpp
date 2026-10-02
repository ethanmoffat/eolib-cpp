#include "eolib/protocol.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace eolib;
using namespace eolib::protocol::net;

namespace
{

template <typename T>
T RoundTrip(const T& object)
{
    data::EoWriter writer;
    object.Serialize(writer);
    data::EoReader reader(writer.ToByteArray());
    T result;
    result.Deserialize(reader);
    return result;
}

} // namespace

TEST(SwitchFactoryTest, ForValue_WithData_SetsCodeAndData)
{
    server::InitInitServerPacket::ReplyCodeDataOutOfDate data;
    data.version.major = 1;

    const auto packet = server::InitInitServerPacket::ForOutOfDate(data);

    EXPECT_EQ(packet.reply_code, server::InitReply::OutOfDate);
    ASSERT_NE(packet.AsOutOfDate(), nullptr);
    EXPECT_EQ(packet.AsOutOfDate()->version.major, 1);
    EXPECT_EQ(RoundTrip(packet), packet);
}

TEST(SwitchFactoryTest, ForValue_CaseWithoutMembers_EmplacesCaseData)
{
    const auto packet = server::LoginReplyServerPacket::ForWrongUser();

    EXPECT_EQ(packet.reply_code, server::LoginReply::WrongUser);
    EXPECT_NE(packet.AsWrongUser(), nullptr);
    EXPECT_EQ(RoundTrip(packet), packet);
}

TEST(SwitchFactoryTest, ForNestedValue_SetsEveryLevel)
{
    server::InitInitServerPacket::ReplyCodeDataBanned::BanTypeDataTemporary data;
    data.minutes_remaining = 5;

    const auto packet = server::InitInitServerPacket::ForBannedTemporary(data);

    EXPECT_EQ(packet.reply_code, server::InitReply::Banned);
    const auto* banned = packet.AsBanned();
    ASSERT_NE(banned, nullptr);
    EXPECT_EQ(banned->ban_type, server::InitBanType::Temporary);
    ASSERT_NE(banned->AsTemporary(), nullptr);
    EXPECT_EQ(banned->AsTemporary()->minutes_remaining, 5);
    EXPECT_EQ(RoundTrip(packet), packet);
}

TEST(SwitchFactoryTest, ForNestedValue_WithoutCase_SetsCodes)
{
    const auto packet = server::InitInitServerPacket::ForBannedPermanent();

    ASSERT_NE(packet.AsBanned(), nullptr);
    EXPECT_EQ(packet.AsBanned()->ban_type, server::InitBanType::Permanent);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(packet.AsBanned()->ban_type_data));
    EXPECT_EQ(RoundTrip(packet), packet);
}

TEST(SwitchFactoryTest, ForIntegerCase_SetsUnrecognizedCode)
{
    server::InitInitServerPacket::ReplyCodeDataBanned::BanTypeData0 data;
    data.minutes_remaining = 7;

    const auto packet = server::InitInitServerPacket::ForBanTypeData0(data);

    ASSERT_NE(packet.AsBanned(), nullptr);
    EXPECT_EQ(packet.AsBanned()->ban_type, static_cast<server::InitBanType>(0));
    ASSERT_NE(packet.AsBanned()->AsBanTypeData0(), nullptr);
    EXPECT_EQ(RoundTrip(packet), packet);
}

TEST(SwitchFactoryTest, ForDefault_ValueWithoutCase_SetsCodeAndData)
{
    server::AccountReplyServerPacket::ReplyCodeDataDefault data;
    data.sequence_start = 12;

    const auto packet =
        server::AccountReplyServerPacket::ForReplyCodeDefault(static_cast<server::AccountReply>(100), data);

    EXPECT_EQ(packet.reply_code, static_cast<server::AccountReply>(100));
    ASSERT_NE(packet.AsReplyCodeDefault(), nullptr);
    EXPECT_EQ(packet.AsReplyCodeDefault()->sequence_start, 12);
    EXPECT_EQ(RoundTrip(packet), packet);
}

TEST(SwitchFactoryTest, ForDefault_ValueWithCase_Throws)
{
    const server::AccountReplyServerPacket::ReplyCodeDataDefault data;

    EXPECT_THROW(server::AccountReplyServerPacket::ForReplyCodeDefault(server::AccountReply::Exists, data),
                 std::invalid_argument);
    EXPECT_THROW(server::AccountReplyServerPacket::ForReplyCodeDefault(static_cast<server::AccountReply>(4), data),
                 std::invalid_argument);
}

TEST(SwitchFactoryTest, ForValue_Struct_SetsCodeAndData)
{
    server::DialogEntry::EntryTypeDataLink data;
    data.link_id = 3;

    const auto entry = server::DialogEntry::ForLink(data);

    EXPECT_EQ(entry.entry_type, server::DialogEntryType::Link);
    ASSERT_NE(entry.AsLink(), nullptr);
    EXPECT_EQ(entry.AsLink()->link_id, 3);
}

TEST(SwitchFactoryTest, As_DifferentCase_ReturnsNull)
{
    auto packet = server::InitInitServerPacket::ForBannedPermanent();

    EXPECT_EQ(packet.AsOk(), nullptr);
    EXPECT_EQ(packet.AsBanned()->AsTemporary(), nullptr);
}

TEST(SwitchFactoryTest, As_NonConst_AllowsModification)
{
    auto packet = server::InitInitServerPacket::ForBannedTemporary({});

    packet.AsBanned()->AsTemporary()->minutes_remaining = 9;

    EXPECT_EQ(packet.AsBanned()->AsTemporary()->minutes_remaining, 9);
}
