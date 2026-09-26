#include "test_utils.hpp"

#include <eolib/errors.hpp>
#include <eolib/protocol/map/structs.hpp>
#include <eolib/protocol/pub/structs.hpp>

#include <gtest/gtest.h>

using namespace eolib;
using namespace eolib::protocol;

namespace
{

template <typename T>
std::vector<uint8_t> SerializeToBytes(const T& object)
{
    data::EoWriter writer;
    object.Serialize(writer);
    return writer.ToByteArray();
}

template <typename T>
T DeserializeFromBytes(const std::vector<uint8_t>& bytes)
{
    data::EoReader reader(bytes);
    T result;
    result.Deserialize(reader);
    EXPECT_EQ(reader.Remaining(), 0);
    EXPECT_EQ(result.ByteSize(), static_cast<int>(bytes.size()));
    return result;
}

pub::EnfRecord MakeEnfRecord(const std::string& name, int graphic_id)
{
    pub::EnfRecord record;
    record.name = name;
    record.graphic_id = graphic_id;
    record.race = 1;
    record.boss = true;
    record.child = false;
    record.type = pub::NpcType::Aggressive;
    record.behavior_id = 7;
    record.hp = 100000;
    record.tp = 50;
    record.min_damage = 1;
    record.max_damage = 5;
    record.accuracy = 10;
    record.evade = 11;
    record.armor = 12;
    record.return_damage = 0;
    record.element = pub::Element::Dark;
    record.element_damage = 3;
    record.element_weakness = pub::Element::Light;
    record.element_weakness_damage = 4;
    record.level = 20;
    record.experience = 1234;
    return record;
}

} // namespace

TEST(PubTest, EnfSerializesExpectedBytes)
{
    pub::Enf enf;
    enf.rid = {1, 2};
    enf.total_npcs_count = 1;
    enf.version = 0;
    enf.npcs.push_back(MakeEnfRecord("Crow", 3));

    data::EoWriter expected;
    expected.AddFixedString("ENF", 3);
    expected.AddShort(1);
    expected.AddShort(2);
    expected.AddShort(1);
    expected.AddChar(0);
    expected.AddChar(4);
    expected.AddFixedString("Crow", 4);
    expected.AddShort(3);
    expected.AddChar(1);
    expected.AddShort(1);
    expected.AddShort(0);
    expected.AddShort(2);
    expected.AddShort(7);
    expected.AddThree(100000);
    expected.AddShort(50);
    expected.AddShort(1);
    expected.AddShort(5);
    expected.AddShort(10);
    expected.AddShort(11);
    expected.AddShort(12);
    expected.AddChar(0);
    expected.AddShort(2);
    expected.AddShort(3);
    expected.AddShort(1);
    expected.AddShort(4);
    expected.AddChar(20);
    expected.AddThree(1234);

    EXPECT_EQ(SerializeToBytes(enf), expected.ToByteArray());
}

TEST(PubTest, EnfRoundTrip)
{
    pub::Enf enf;
    enf.rid = {100, 200};
    enf.total_npcs_count = 3;
    enf.version = 1;
    enf.npcs.push_back(MakeEnfRecord("Crow", 3));
    enf.npcs.push_back(MakeEnfRecord("Goat", 4));
    enf.npcs.push_back(MakeEnfRecord("", 0));

    const auto bytes = SerializeToBytes(enf);
    const auto deserialized = DeserializeFromBytes<pub::Enf>(bytes);
    EXPECT_EQ(deserialized, enf);
    EXPECT_EQ(SerializeToBytes(deserialized), bytes);
}

TEST(PubTest, InvalidFileTypeIsIgnoredOnDeserialize)
{
    // Hardcoded values are read but not validated, matching the other eolib implementations.
    const auto bytes = test::Bytes({'X', 'Y', 'Z', 1, 254, 1, 254, 1, 254, 1});
    const auto enf = DeserializeFromBytes<pub::Enf>(bytes);
    EXPECT_TRUE(enf.npcs.empty());
    EXPECT_EQ(SerializeToBytes(enf), test::Bytes({'E', 'N', 'F', 1, 254, 1, 254, 1, 254, 1}));
}

TEST(PubTest, RidMustHaveExactlyTwoElements)
{
    pub::Enf enf;
    enf.rid = {1};
    data::EoWriter writer;
    EXPECT_THROW(enf.Serialize(writer), SerializationError);
}

TEST(MapTest, SignStringDataLengthIsOffset)
{
    map::MapSign sign;
    sign.coords.x = 1;
    sign.coords.y = 2;
    sign.string_data = "TitleText";
    sign.title_length = 5;

    const auto bytes = SerializeToBytes(sign);
    data::EoReader reader(bytes);
    EXPECT_EQ(reader.GetChar(), 1);
    EXPECT_EQ(reader.GetChar(), 2);
    // The length on the wire is one greater than the length of the string data.
    EXPECT_EQ(reader.GetShort(), 10);

    EXPECT_EQ(DeserializeFromBytes<map::MapSign>(bytes), sign);
}

TEST(MapTest, EmfRoundTrip)
{
    map::Emf emf;
    emf.rid = {5, 6};
    emf.name = "Aeven";
    emf.type = map::MapType::Pk;
    emf.width = 10;
    emf.height = 12;
    emf.fill_tile = 3;
    emf.map_available = true;
    emf.can_scroll = true;
    emf.relog_x = 4;
    emf.relog_y = 5;

    map::MapTileSpecRow row;
    row.y = 2;
    row.tiles.emplace_back();
    row.tiles.back().x = 3;
    row.tiles.back().tile_spec = map::MapTileSpec::ChairDown;
    emf.tile_spec_rows.push_back(row);

    emf.graphic_layers.resize(9);

    map::MapSign sign;
    sign.coords.x = 1;
    sign.coords.y = 1;
    sign.string_data = "Hello";
    sign.title_length = 2;
    emf.signs.push_back(sign);

    const auto bytes = SerializeToBytes(emf);
    const auto deserialized = DeserializeFromBytes<map::Emf>(bytes);
    EXPECT_EQ(deserialized, emf);
    EXPECT_EQ(SerializeToBytes(deserialized), bytes);
}

TEST(MapTest, EmfRequiresNineGraphicLayers)
{
    map::Emf emf;
    emf.rid = {0, 0};
    data::EoWriter writer;
    EXPECT_THROW(emf.Serialize(writer), SerializationError);
}
