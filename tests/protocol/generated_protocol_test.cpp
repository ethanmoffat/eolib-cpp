#include "test_utils.hpp"

#include "eolib/errors.hpp"
#include "eolib/protocol.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
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

client::WalkPlayerClientPacket MakeWalkPacket()
{
    client::WalkPlayerClientPacket packet;
    packet.walk_action.direction = Direction::Right;
    packet.walk_action.timestamp = 123456;
    packet.walk_action.coords.x = 10;
    packet.walk_action.coords.y = 20;
    return packet;
}

} // namespace

TEST(GeneratedProtocolTest, EnumToString)
{
    EXPECT_EQ(ToString(Direction::Down), "Down");
    EXPECT_EQ(ToString(PacketFamily::Walk), "Walk");
    EXPECT_EQ(ToString(static_cast<Direction>(99)), "Unrecognized(99)");
}

TEST(GeneratedProtocolTest, PacketIdentifiers)
{
    const client::WalkPlayerClientPacket packet;
    EXPECT_EQ(client::WalkPlayerClientPacket::FAMILY, PacketFamily::Walk);
    EXPECT_EQ(client::WalkPlayerClientPacket::ACTION, PacketAction::Player);

    const Packet& base = packet;
    EXPECT_EQ(base.Family(), PacketFamily::Walk);
    EXPECT_EQ(base.Action(), PacketAction::Player);
}

TEST(GeneratedProtocolTest, RoundTrip)
{
    const auto packet = MakeWalkPacket();
    const auto bytes = SerializeToBytes(packet);
    EXPECT_EQ(packet.ByteSize(), 0);

    data::EoReader reader(bytes);
    client::WalkPlayerClientPacket deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(deserialized, packet);
    EXPECT_EQ(deserialized.ByteSize(), static_cast<int>(bytes.size()));
    EXPECT_EQ(SerializeToBytes(deserialized), bytes);
}

TEST(GeneratedProtocolTest, Equality)
{
    const auto a = MakeWalkPacket();
    auto b = MakeWalkPacket();
    EXPECT_EQ(a, b);

    b.walk_action.coords.x = 11;
    EXPECT_NE(a, b);
}

TEST(GeneratedProtocolTest, EqualityIgnoresByteSize)
{
    const auto packet = MakeWalkPacket();
    const auto bytes = SerializeToBytes(packet);
    data::EoReader reader(bytes);
    client::WalkPlayerClientPacket deserialized;
    deserialized.Deserialize(reader);

    EXPECT_NE(deserialized.ByteSize(), packet.ByteSize());
    EXPECT_EQ(deserialized, packet);
}

TEST(GeneratedProtocolTest, ToString)
{
    EXPECT_EQ(MakeWalkPacket().ToString(),
              "WalkPlayerClientPacket{walk_action=WalkAction{direction=Right, timestamp=123456, "
              "coords=Coords{x=10, y=20}}}");
}

TEST(GeneratedProtocolTest, StreamOutput)
{
    std::ostringstream stream;
    stream << MakeWalkPacket() << ' ' << Direction::Left << ' ' << static_cast<Direction>(99) << ' '
           << client::ChairRequestClientPacket::SitActionDataSit();
    EXPECT_EQ(stream.str(), MakeWalkPacket().ToString() + " Left Unrecognized(99) " +
                                client::ChairRequestClientPacket::SitActionDataSit().ToString());
}

TEST(GeneratedProtocolTest, StreamOutputThroughBaseReference)
{
    const auto packet = MakeWalkPacket();
    const Packet& base = packet;
    std::ostringstream stream;
    stream << base;
    EXPECT_EQ(stream.str(), packet.ToString());
}

TEST(GeneratedProtocolTest, SwitchRoundTrip)
{
    client::ChairRequestClientPacket packet;
    packet.sit_action = client::SitAction::Sit;
    client::ChairRequestClientPacket::SitActionDataSit sit_data;
    sit_data.coords.x = 5;
    sit_data.coords.y = 6;
    packet.sit_action_data = sit_data;

    const auto bytes = SerializeToBytes(packet);
    data::EoReader reader(bytes);
    client::ChairRequestClientPacket deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(deserialized, packet);
    ASSERT_TRUE(
        std::holds_alternative<client::ChairRequestClientPacket::SitActionDataSit>(deserialized.sit_action_data));
}

TEST(GeneratedProtocolTest, SwitchDataMismatchThrows)
{
    client::ChairRequestClientPacket packet;
    packet.sit_action = client::SitAction::Sit;

    data::EoWriter writer;
    EXPECT_THROW(packet.Serialize(writer), SerializationError);
}

TEST(GeneratedProtocolTest, FixedLengthStringMismatchThrows)
{
    client::GuildTakeClientPacket packet;
    packet.guild_tag = "TOOLONG";

    data::EoWriter writer;
    EXPECT_THROW(packet.Serialize(writer), SerializationError);

    packet.guild_tag = "ABC";
    EXPECT_NO_THROW(packet.Serialize(writer));
}

TEST(GeneratedProtocolTest, PacketFactory)
{
    const auto packet = client::PacketFactory::Create(PacketFamily::Walk, PacketAction::Player);
    ASSERT_NE(packet, nullptr);
    EXPECT_NE(dynamic_cast<client::WalkPlayerClientPacket*>(packet.get()), nullptr);

    EXPECT_EQ(client::PacketFactory::Create(PacketFamily::Walk, PacketAction::Junk), nullptr);
}

TEST(GeneratedProtocolTest, PacketFactoryContains)
{
    EXPECT_TRUE(client::PacketFactory::Contains(PacketFamily::Walk, PacketAction::Player));
    EXPECT_FALSE(client::PacketFactory::Contains(PacketFamily::Walk, PacketAction::Junk));
    EXPECT_FALSE(client::PacketFactory::Contains(static_cast<PacketFamily>(0), PacketAction::Player));

    EXPECT_TRUE(server::PacketFactory::Contains(PacketFamily::Walk, PacketAction::Reply));
    EXPECT_FALSE(server::PacketFactory::Contains(PacketFamily::Walk, PacketAction::Junk));
}

TEST(GeneratedProtocolTest, PacketFactoryContains_EveryId_MatchesCreate)
{
    for (int family = 0; family <= 0xFF; ++family)
    {
        for (int action = 0; action <= 0xFF; ++action)
        {
            const auto f = static_cast<PacketFamily>(family);
            const auto a = static_cast<PacketAction>(action);
            EXPECT_EQ(client::PacketFactory::Contains(f, a), client::PacketFactory::Create(f, a) != nullptr)
                << "client family " << family << " action " << action;
            EXPECT_EQ(server::PacketFactory::Contains(f, a), server::PacketFactory::Create(f, a) != nullptr)
                << "server family " << family << " action " << action;
        }
    }
}

TEST(GeneratedProtocolTest, PacketFactoryDeserialize)
{
    const auto expected = MakeWalkPacket();
    const auto bytes = SerializeToBytes(expected);

    data::EoReader reader(bytes);
    const auto packet = client::PacketFactory::Deserialize(PacketFamily::Walk, PacketAction::Player, reader);
    ASSERT_NE(packet, nullptr);

    const auto* walk = dynamic_cast<client::WalkPlayerClientPacket*>(packet.get());
    ASSERT_NE(walk, nullptr);
    EXPECT_EQ(*walk, expected);
}
