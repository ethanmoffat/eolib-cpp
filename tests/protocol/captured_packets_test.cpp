// Round-trip tests for the packets captured from the official client and server, from the eo-captured-packets
// repository. Each packet is deserialized through the packet factory and must serialize back to the exact same bytes.

#include <eolib/protocol.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
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
};

std::vector<CapturedPacket> FindCapturedPackets()
{
    std::vector<CapturedPacket> result;
    const fs::path root = EOLIB_CAPTURED_PACKETS_DIR;
    for (const std::string side : {"client", "server"})
    {
        const fs::path dir = root / side;
        if (!fs::is_directory(dir))
        {
            continue;
        }
        // Only the top-level files are used. The "original" subdirectories contain invalid data sent by the official
        // client and server, preserved for reference.
        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                result.push_back({side + "_" + entry.path().stem().string(), side, entry.path()});
            }
        }
    }
    std::sort(result.begin(), result.end(),
              [](const CapturedPacket& a, const CapturedPacket& b) { return a.name < b.name; });
    return result;
}

std::vector<uint8_t> DecodeBase64(const std::string& input)
{
    static const std::string kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> result;
    int buffer = 0;
    int bits = 0;
    for (const char c : input)
    {
        if (c == '=')
        {
            break;
        }
        const auto index = kAlphabet.find(c);
        if (index == std::string::npos)
        {
            continue;
        }
        buffer = (buffer << 6) | static_cast<int>(index);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            result.push_back(static_cast<uint8_t>((buffer >> bits) & 0xFF));
        }
    }
    return result;
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

class CapturedPacketsTest : public ::testing::TestWithParam<CapturedPacket>
{
};

} // namespace

TEST(CapturedPacketsDiscoveryTest, FindsCapturedPackets)
{
    EXPECT_GT(FindCapturedPackets().size(), 100u) << "Is the eo-captured-packets submodule initialized?";
}

TEST_P(CapturedPacketsTest, RoundTrip)
{
    const auto& captured = GetParam();
    std::ifstream stream(captured.path);
    ASSERT_TRUE(stream) << "Failed to open " << captured.path;
    const auto json = nlohmann::json::parse(stream);

    const auto family = ParseEnum<PacketFamily>(json.at("family").get<std::string>());
    const auto action = ParseEnum<PacketAction>(json.at("action").get<std::string>());
    const auto expected = DecodeBase64(json.at("expected").get<std::string>());

    data::EoReader reader(expected);
    const auto packet = DeserializePacket(captured.side, family, action, reader);
    ASSERT_NE(packet, nullptr) << "No " << captured.side << " packet for " << ToString(family) << "_"
                               << ToString(action);

    EXPECT_EQ(packet->Family(), family);
    EXPECT_EQ(packet->Action(), action);
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(packet->ByteSize(), static_cast<int>(expected.size()));

    data::EoWriter writer;
    packet->Serialize(writer);
    EXPECT_EQ(writer.ToByteArray(), expected) << packet->ToString();

    // Deserializing the serialized data again must produce an identical packet.
    data::EoReader second_reader(writer.ToByteArray());
    const auto second = DeserializePacket(captured.side, family, action, second_reader);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->ToString(), packet->ToString());
}

INSTANTIATE_TEST_SUITE_P(EoCapturedPackets, CapturedPacketsTest, ::testing::ValuesIn(FindCapturedPackets()),
                         [](const ::testing::TestParamInfo<CapturedPacket>& info)
                         {
                             std::string name = info.param.name;
                             std::replace_if(
                                 name.begin(), name.end(), [](unsigned char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
