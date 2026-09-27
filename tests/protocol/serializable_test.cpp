#include "eolib/protocol/serializable.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

using eolib::data::EoReader;
using eolib::data::EoWriter;
using eolib::protocol::Serializable;

namespace
{

class TestSerializable final : public Serializable
{
public:
    int value = 0;

    void Serialize(EoWriter& writer) const override
    {
        writer.AddShort(value);
    }

    void Deserialize(EoReader& reader) override
    {
        const int start = reader.Position();
        value = reader.GetShort();
        byte_size_ = reader.Position() - start;
    }

    int ByteSize() const noexcept override
    {
        return byte_size_;
    }

    std::string ToString() const override
    {
        return "TestSerializable{value=" + std::to_string(value) + "}";
    }

private:
    int byte_size_ = 0;
};

static_assert(std::is_abstract_v<Serializable>);
static_assert(std::has_virtual_destructor_v<Serializable>);
static_assert(std::is_copy_constructible_v<TestSerializable>);

TEST(SerializableTest, RoundTripThroughInterface)
{
    TestSerializable source;
    source.value = 12345;

    EoWriter writer;
    static_cast<const Serializable&>(source).Serialize(writer);

    EoReader reader(writer.ToByteArray());
    std::unique_ptr<Serializable> target = std::make_unique<TestSerializable>();
    target->Deserialize(reader);

    EXPECT_EQ(target->ByteSize(), 2);
    EXPECT_EQ(target->ToString(), "TestSerializable{value=12345}");
}

TEST(SerializableTest, StreamOutputUsesToString)
{
    TestSerializable value;
    value.value = 7;
    std::ostringstream stream;
    stream << value;
    EXPECT_EQ(stream.str(), "TestSerializable{value=7}");
}

} // namespace
