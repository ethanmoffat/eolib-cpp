// Tests for the packets captured from the official client and server, from the eo-captured-packets repository.
//
// Each packet is deserialized through the packet factory and must serialize back to the exact same bytes. Each packet
// is also built from the captured properties (field values): it must equal the deserialized packet, and serialize to
// the captured bytes.

#include "protocol/captured_packet_properties.hpp"

#include "eolib/errors.hpp"
#include "eolib/protocol.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace eolib;
using namespace eolib::protocol::net;

namespace
{

namespace fs = std::filesystem;

struct CapturedPacket
{
    std::string name;
    std::string side;
    fs::path path;
    /// True for the invalid data sent by the official client and server, which doesn't round-trip.
    bool original = false;
};

// Found by GoogleTest via argument-dependent lookup; without it, parameters print as raw bytes.
void PrintTo(const CapturedPacket& packet, std::ostream* os)
{
    *os << packet.path.generic_string();
}

void AddCapturedPackets(const fs::path& dir, const std::string& side, bool original,
                        std::vector<CapturedPacket>& result)
{
    if (!fs::is_directory(dir))
    {
        return;
    }
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            const std::string prefix = side + (original ? "_original_" : "_");
            result.push_back({prefix + entry.path().stem().string(), side, entry.path(), original});
        }
    }
}

/// Finds the captured packets. The "original" subdirectories contain invalid data sent by the official client and
/// server, preserved for reference, which is only included if requested.
std::vector<CapturedPacket> FindCapturedPackets(bool include_original)
{
    std::vector<CapturedPacket> result;
    const fs::path root = EOLIB_CAPTURED_PACKETS_DIR;
    for (const std::string side : {"client", "server"})
    {
        AddCapturedPackets(root / side, side, false, result);
        if (include_original)
        {
            AddCapturedPackets(root / side / "original", side, true, result);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const CapturedPacket& a, const CapturedPacket& b) { return a.name < b.name; });
    return result;
}

std::string TestName(const ::testing::TestParamInfo<CapturedPacket>& info)
{
    std::string name = info.param.name;
    std::replace_if(name.begin(), name.end(), [](unsigned char c) { return !std::isalnum(c); }, '_');
    return name;
}

template <typename Enum>
Enum ParseEnum(const std::string& name)
{
    for (int i = 0; i <= 255; ++i)
    {
        const auto value = static_cast<Enum>(i);
        if (ToString(value) == name)
        {
            return value;
        }
    }
    throw std::invalid_argument("Unknown enum value: " + name);
}

std::unique_ptr<Packet> DeserializePacket(const std::string& side, PacketFamily family, PacketAction action,
                                          data::EoReader& reader)
{
    return side == "client" ? client::PacketFactory::Deserialize(family, action, reader)
                            : server::PacketFactory::Deserialize(family, action, reader);
}

std::unique_ptr<Packet> PacketFromProperties(const std::string& side, PacketFamily family, PacketAction action,
                                             const nlohmann::json& properties)
{
    return side == "client" ? test::client::PacketFromProperties(family, action, properties)
                            : test::server::PacketFromProperties(family, action, properties);
}

bool PacketsEqual(const std::string& side, const Packet& a, const Packet& b)
{
    return side == "client" ? test::client::PacketsEqual(a, b) : test::server::PacketsEqual(a, b);
}

/// Original packets whose data can't be serialized, because it violates the protocol's constraints.
constexpr const char* UNSERIALIZABLE_PACKETS[] = {
    // The characters array has more elements than its length field allows.
    "server_original_PlayersAgree",
};

bool IsUnserializable(const CapturedPacket& captured)
{
    return std::any_of(std::begin(UNSERIALIZABLE_PACKETS), std::end(UNSERIALIZABLE_PACKETS),
                       [&](const char* name) { return captured.name == name; });
}

/// The contents of a captured packet file.
struct CapturedPacketData
{
    PacketFamily family{};
    PacketAction action{};
    std::vector<std::uint8_t> expected;
    nlohmann::json properties;
};

CapturedPacketData LoadCapturedPacket(const CapturedPacket& captured)
{
    std::ifstream stream(captured.path);
    if (!stream)
    {
        throw std::runtime_error("Failed to open " + captured.path.string());
    }
    const auto json = nlohmann::json::parse(stream);

    CapturedPacketData result;
    result.family = ParseEnum<PacketFamily>(json.at("family").get<std::string>());
    result.action = ParseEnum<PacketAction>(json.at("action").get<std::string>());
    result.expected = test::DecodeBase64(json.at("expected").get<std::string>());
    const auto properties = json.find("properties");
    result.properties = properties == json.end() || properties->is_null() ? nlohmann::json::array() : *properties;
    return result;
}

class CapturedPacketsTest : public ::testing::TestWithParam<CapturedPacket>
{
};

class CapturedPacketPropertiesTest : public ::testing::TestWithParam<CapturedPacket>
{
};

} // namespace

TEST(CapturedPacketsDiscoveryTest, FindsCapturedPackets)
{
    EXPECT_GT(FindCapturedPackets(false).size(), 100u) << "Is the eo-captured-packets submodule initialized?";
    EXPECT_GT(FindCapturedPackets(true).size(), FindCapturedPackets(false).size());
}

TEST_P(CapturedPacketsTest, RoundTrip)
{
    const auto& captured = GetParam();
    const auto data = LoadCapturedPacket(captured);

    data::EoReader reader(data.expected);
    const auto packet = DeserializePacket(captured.side, data.family, data.action, reader);
    ASSERT_NE(packet, nullptr) << "No " << captured.side << " packet for " << ToString(data.family) << "_"
                               << ToString(data.action);

    EXPECT_EQ(packet->Family(), data.family);
    EXPECT_EQ(packet->Action(), data.action);
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(packet->ByteSize(), static_cast<int>(data.expected.size()));

    data::EoWriter writer;
    packet->Serialize(writer);
    EXPECT_EQ(writer.ToByteArray(), data.expected) << packet->ToString();

    // Deserializing the serialized data again must produce an identical packet.
    data::EoReader second_reader(writer.ToByteArray());
    const auto second = DeserializePacket(captured.side, data.family, data.action, second_reader);
    ASSERT_NE(second, nullptr);
    EXPECT_TRUE(PacketsEqual(captured.side, *second, *packet)) << second->ToString() << "\n" << packet->ToString();
}

TEST_P(CapturedPacketPropertiesTest, DeserializeMatchesProperties)
{
    const auto& captured = GetParam();
    const auto data = LoadCapturedPacket(captured);

    const auto expected = PacketFromProperties(captured.side, data.family, data.action, data.properties);
    ASSERT_NE(expected, nullptr);

    data::EoReader reader(data.expected);
    const auto actual = DeserializePacket(captured.side, data.family, data.action, reader);
    ASSERT_NE(actual, nullptr);

    EXPECT_TRUE(PacketsEqual(captured.side, *actual, *expected)) << "Actual:   " << actual->ToString() << "\n"
                                                                 << "Expected: " << expected->ToString();
}

TEST_P(CapturedPacketPropertiesTest, SerializeFromProperties)
{
    const auto& captured = GetParam();
    const auto data = LoadCapturedPacket(captured);

    const auto packet = PacketFromProperties(captured.side, data.family, data.action, data.properties);
    ASSERT_NE(packet, nullptr);

    data::EoWriter writer;
    if (IsUnserializable(captured))
    {
        EXPECT_THROW(packet->Serialize(writer), eolib::SerializationError);
        return;
    }
    packet->Serialize(writer);

    if (!captured.original)
    {
        EXPECT_EQ(writer.ToByteArray(), data.expected) << packet->ToString();
        return;
    }

    // The original data doesn't conform to the protocol, so it can't be reproduced. Instead, the serialized data must
    // deserialize to the same packet.
    data::EoReader reader(writer.ToByteArray());
    const auto actual = DeserializePacket(captured.side, data.family, data.action, reader);
    ASSERT_NE(actual, nullptr);
    EXPECT_TRUE(PacketsEqual(captured.side, *actual, *packet)) << "Actual:   " << actual->ToString() << "\n"
                                                               << "Expected: " << packet->ToString();
}

INSTANTIATE_TEST_SUITE_P(EoCapturedPackets, CapturedPacketsTest, ::testing::ValuesIn(FindCapturedPackets(false)),
                         TestName);

INSTANTIATE_TEST_SUITE_P(EoCapturedPackets, CapturedPacketPropertiesTest,
                         ::testing::ValuesIn(FindCapturedPackets(true)), TestName);
