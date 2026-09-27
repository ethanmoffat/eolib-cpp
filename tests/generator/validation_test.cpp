#include "generator_test_utils.hpp"

using eolib::generator::test::FindOutput;
using eolib::generator::test::Generate;

TEST(GeneratorValidationTest, ValidProtocolGeneratesExpectedFiles)
{
    const auto outputs = Generate(R"(
        <enum name="Color" type="char">
            <value name="Red">0</value>
            <value name="Green">1</value>
        </enum>
        <struct name="Thing">
            <field name="color" type="Color"/>
            <length name="name_length" type="char"/>
            <field name="name" type="string" length="name_length"/>
        </struct>)");

    EXPECT_NE(FindOutput(outputs, "include/eolib/protocol/enums.hpp"), nullptr);
    EXPECT_NE(FindOutput(outputs, "src/eolib/protocol/enums.cpp"), nullptr);
    EXPECT_NE(FindOutput(outputs, "include/eolib/protocol/structs.hpp"), nullptr);
    EXPECT_NE(FindOutput(outputs, "src/eolib/protocol/structs.cpp"), nullptr);
    EXPECT_NE(FindOutput(outputs, "include/eolib/protocol.hpp"), nullptr);
    EXPECT_EQ(FindOutput(outputs, "include/eolib/protocol/packets.hpp"), nullptr);

    const auto* structs = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    ASSERT_NE(structs, nullptr);
    EXPECT_NE(structs->content.find("class EOLIB_API Thing final : public Serializable"), std::string::npos);
    EXPECT_NE(structs->content.find("Color color{};"), std::string::npos);
    EXPECT_NE(structs->content.find("std::string name{};"), std::string::npos);
    EXPECT_EQ(structs->content.find("name_length{}"), std::string::npos);
}

TEST(GeneratorValidationTest, OutputIsDeterministic)
{
    const std::string protocol = R"(
        <struct name="B"><field name="a" type="A"/></struct>
        <struct name="A"><field name="x" type="short"/></struct>)";
    const auto first = Generate(protocol);
    const auto second = Generate(protocol);
    ASSERT_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); ++i)
    {
        EXPECT_EQ(first[i].path, second[i].path);
        EXPECT_EQ(first[i].content, second[i].content);
    }
}

TEST(GeneratorValidationTest, StructsAreSortedByDependency)
{
    const auto outputs = Generate(R"(
        <struct name="Outer"><field name="inner" type="Inner"/></struct>
        <struct name="Inner"><field name="x" type="short"/></struct>)");
    const auto* structs = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    ASSERT_NE(structs, nullptr);
    EXPECT_LT(structs->content.find("class EOLIB_API Inner"), structs->content.find("class EOLIB_API Outer"));
}

TEST(GeneratorValidationTest, KeywordFieldNamesAreEscaped)
{
    const auto outputs = Generate(R"(<struct name="S"><field name="class" type="char"/></struct>)");
    const auto* structs = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    ASSERT_NE(structs, nullptr);
    EXPECT_NE(structs->content.find("int class_{};"), std::string::npos);
}

TEST(GeneratorValidationTest, UnknownType)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="Missing"/></struct>)", "Missing");
}

TEST(GeneratorValidationTest, DuplicateTypeName)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S"><field name="x" type="char"/></struct>
        <struct name="S"><field name="y" type="char"/></struct>)",
                           "S");
}

TEST(GeneratorValidationTest, RecursiveStruct)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="self" type="S" optional="true"/></struct>)", "S");
}

TEST(GeneratorValidationTest, DuplicateFieldName)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="char"/><field name="x" type="char"/></struct>)",
                           "Cannot redefine x field");
}

TEST(GeneratorValidationTest, NonOptionalAfterOptional)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="a" type="char" optional="true"/>
            <field name="b" type="char"/>
        </struct>)",
                           "Optional fields may not be followed by non-optional fields");
}

TEST(GeneratorValidationTest, UnnamedFieldWithoutValue)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field type="char"/></struct>)",
                           "Unnamed fields must specify a hardcoded field value");
}

TEST(GeneratorValidationTest, HardcodedValueOnCustomType)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="char"><value name="A">0</value></enum>
        <struct name="S"><field name="e" type="E">A</field></struct>)",
                           "Hardcoded field values are not allowed");
}

TEST(GeneratorValidationTest, HardcodedStringLengthMismatch)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field type="string" length="3">TOOLONG</field></struct>)",
                           "Expected length of 3");
}

TEST(GeneratorValidationTest, UnreferencedLengthField)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><length name="len" type="char"/></struct>)",
                           "Length field \"len\" must be referenced by another field");
}

TEST(GeneratorValidationTest, LengthReferencesNonLengthField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <field name="s" type="string" length="n"/>
        </struct>)",
                           "must be a numeric literal, or refer to a length field");
}

TEST(GeneratorValidationTest, NonNumericLengthField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <length name="len" type="string"/>
            <field name="s" type="string" length="len"/>
        </struct>)",
                           "not a numeric type");
}

TEST(GeneratorValidationTest, DelimitedArrayOutsideChunked)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><array name="a" type="string" delimited="true"/></struct>)",
                           "delimited array instruction unless chunked reading is enabled");
}

TEST(GeneratorValidationTest, UnboundedNonDelimitedArray)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><array name="a" type="string"/></struct>)", "Unbounded element type");
}

TEST(GeneratorValidationTest, TrailingDelimiterOnNonDelimitedArray)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><array name="a" type="char" trailing-delimiter="true"/></struct>)",
                           "Only delimited arrays can have a trailing delimiter");
}

TEST(GeneratorValidationTest, BreakOutsideChunked)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="a" type="char"/><break/></struct>)",
                           "break instruction unless chunked reading is enabled");
}

TEST(GeneratorValidationTest, ElementAfterDummy)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><dummy type="char">0</dummy><field name="a" type="char"/></struct>)",
                           "<dummy> elements must not be followed by any other elements");
}

TEST(GeneratorValidationTest, SwitchOnMissingField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <switch field="missing"><case default="true"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "Referenced missing field is not accessible");
}

TEST(GeneratorValidationTest, SwitchOnStringField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="s" type="string"/>
            <switch field="s"><case default="true"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "must be a numeric or enumeration type");
}

TEST(GeneratorValidationTest, SwitchDefaultNotLast)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n">
                <case default="true"><field name="a" type="char"/></case>
                <case value="1"><field name="b" type="char"/></case>
            </switch>
        </struct>)",
                           "Only the last case in a switch can be the default case");
}

TEST(GeneratorValidationTest, SwitchCaseReferencesDefinedEnumValueByNumber)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="char"><value name="A">0</value></enum>
        <struct name="S">
            <field name="e" type="E"/>
            <switch field="e"><case value="0"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "must be referred to by name (A)");
}

TEST(GeneratorValidationTest, SwitchCaseInvalidEnumValue)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="char"><value name="A">0</value></enum>
        <struct name="S">
            <field name="e" type="E"/>
            <switch field="e"><case value="B"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "\"B\" is not a valid value for enum type E");
}

TEST(GeneratorValidationTest, PacketsOutsideClientOrServerFile)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="PacketFamily" type="byte"><value name="Connection">1</value></enum>
        <enum name="PacketAction" type="byte"><value name="Request">1</value></enum>
        <packet family="Connection" action="Request"><field name="a" type="char"/></packet>)",
                           "packets are only allowed in client or server protocol files");
}

TEST(GeneratorValidationTest, HardcodedFieldReferencesLengthField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <length name="len" type="char"/>
            <field type="string" length="len">AB</field>
        </struct>)",
                           "Hardcoded fields must not reference a length field.");
}

TEST(GeneratorValidationTest, InvalidHardcodedIntegerValue)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field type="char">abc</field></struct>)",
                           "\"abc\" is not a valid integer value.");
}

TEST(GeneratorValidationTest, InvalidHardcodedBoolValue)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field type="bool">yes</field></struct>)",
                           "\"yes\" is not a valid bool value.");
}

TEST(GeneratorValidationTest, LengthOnNonStringField)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="char" length="2"/></struct>)",
                           "Only string types may specify a length");
}

TEST(GeneratorValidationTest, PaddedOnNonStringField)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="char" padded="true"/></struct>)",
                           "length and padded attributes are only allowed for string types.");
}

TEST(GeneratorValidationTest, PaddedFieldWithoutLength)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="s" type="string" padded="true"/></struct>)",
                           "Padded fields must specify a length.");
}

TEST(GeneratorValidationTest, UnnamedOptionalField)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field type="char" optional="true">1</field></struct>)",
                           "Unnamed fields may not be optional.");
}

TEST(GeneratorValidationTest, LengthFieldReferencedByMultipleFields)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <length name="len" type="char"/>
            <field name="a" type="string" length="len"/>
            <field name="b" type="string" length="len"/>
        </struct>)",
                           "Length field \"len\" must not be referenced by multiple fields.");
}

TEST(GeneratorValidationTest, DummyWithoutValue)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><dummy type="char"/></struct>)",
                           "<dummy> elements must specify a value.");
}

TEST(GeneratorValidationTest, DummyWithNonBasicType)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="Inner"><field name="x" type="char"/></struct>
        <struct name="S"><dummy type="Inner">0</dummy></struct>)",
                           "Dummy fields must be a basic type.");
}

TEST(GeneratorValidationTest, SwitchOnArrayField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <array name="n" type="char" length="2"/>
            <switch field="n"><case value="1"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "\"n\" field referenced by switch must not be an array.");
}

TEST(GeneratorValidationTest, SwitchOnOptionalField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char" optional="true"/>
            <switch field="n"><case value="1"><field name="a" type="char" optional="true"/></case></switch>
        </struct>)",
                           "\"n\" field referenced by switch must be unconditionally present");
}

TEST(GeneratorValidationTest, SwitchCaseInvalidIntegerValue)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n"><case value="one"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "\"one\" is not a valid integer value.");
}

TEST(GeneratorValidationTest, DefaultCaseWithValue)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n"><case default="true" value="1"><field name="a" type="char"/></case></switch>
        </struct>)",
                           "default <case> elements must not specify a value.");
}

TEST(GeneratorValidationTest, NonDefaultCaseWithoutValue)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n"><case><field name="a" type="char"/></case></switch>
        </struct>)",
                           "non-default <case> elements must specify a value.");
}

TEST(GeneratorValidationTest, DuplicateIntegerCaseValue)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n">
                <case value="1"><field name="a" type="char"/></case>
                <case value="1"><field name="b" type="char"/></case>
            </switch>
        </struct>)",
                           "Duplicate case value 1 in switch on n.");
}

TEST(GeneratorValidationTest, DuplicateEnumCaseValue)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="char"><value name="A">0</value></enum>
        <struct name="S">
            <field name="e" type="E"/>
            <switch field="e">
                <case value="A"><field name="a" type="char"/></case>
                <case value="A"><field name="b" type="char"/></case>
            </switch>
        </struct>)",
                           "Duplicate case value A in switch on e.");
}

TEST(GeneratorValidationTest, SwitchCaseMayReferenceUndefinedEnumValueByNumber)
{
    EXPECT_NO_THROW(Generate(R"(
        <enum name="E" type="char"><value name="A">0</value></enum>
        <struct name="S">
            <field name="e" type="E"/>
            <switch field="e"><case value="5"><field name="a" type="char"/></case></switch>
        </struct>)"));
}

TEST(GeneratorValidationTest, OptionalFieldInCaseFollowedByNonOptionalField)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n"><case value="1"><field name="a" type="char" optional="true"/></case></switch>
            <field name="b" type="char"/>
        </struct>)",
                           "Optional fields may not be followed by non-optional fields.");
}

TEST(GeneratorValidationTest, EnumValueWithoutOrdinal)
{
    EXPECT_GENERATOR_ERROR(R"(<enum name="E" type="char"><value name="A"/></enum>)",
                           "enum values must specify an ordinal value.");
}

TEST(GeneratorValidationTest, EnumDuplicateOrdinal)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="char"><value name="A">0</value><value name="B">0</value></enum>
        <struct name="S"><field name="e" type="E"/></struct>)",
                           "E.B cannot redefine ordinal value 0.");
}

TEST(GeneratorValidationTest, EnumDuplicateValueName)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="char"><value name="A">0</value><value name="A">1</value></enum>
        <struct name="S"><field name="e" type="E"/></struct>)",
                           "E enum cannot redefine value name A.");
}

TEST(GeneratorValidationTest, EnumWithNonNumericType)
{
    EXPECT_GENERATOR_ERROR(R"(
        <enum name="E" type="string"><value name="A">0</value></enum>
        <struct name="S"><field name="e" type="E"/></struct>)",
                           "string is not a numeric type, so it cannot be specified as an underlying type.");
}

TEST(GeneratorValidationTest, UnderlyingTypeOnStruct)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="Inner"><field name="x" type="char"/></struct>
        <struct name="S"><field name="inner" type="Inner:short"/></struct>)",
                           "Inner has no underlying type, so short is not allowed as an underlying type override.");
}

TEST(GeneratorValidationTest, UnderlyingTypeOnBasicNumericType)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="char:short"/></struct>)",
                           "char has no underlying type, so short is not allowed as an underlying type override.");
}

TEST(GeneratorValidationTest, NonNumericUnderlyingType)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="bool:string"/></struct>)",
                           "string is not a numeric type, so it cannot be specified as an underlying type.");
}

TEST(GeneratorValidationTest, UnderlyingTypeIsItself)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="short:short"/></struct>)",
                           "short type cannot specify itself as an underlying type.");
}

TEST(GeneratorValidationTest, UnderlyingTypeWithMultipleColons)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><field name="x" type="bool:char:short"/></struct>)",
                           "(Only one colon is allowed)");
}

TEST(GeneratorValidationTest, UnderlyingTypeOverridesSerialization)
{
    const auto outputs = Generate(R"(
        <enum name="E" type="char"><value name="A">0</value></enum>
        <struct name="S">
            <field name="flag" type="bool:short"/>
            <field name="e" type="E:three"/>
        </struct>)");
    const auto* source = FindOutput(outputs, "src/eolib/protocol/structs.cpp");
    ASSERT_NE(source, nullptr);
    EXPECT_NE(source->content.find("writer.AddShort(flag ? 1 : 0);"), std::string::npos);
    EXPECT_NE(source->content.find("flag = reader.GetShort() != 0;"), std::string::npos);
    EXPECT_NE(source->content.find("writer.AddThree(static_cast<int>(e));"), std::string::npos);
    EXPECT_NE(source->content.find("e = static_cast<E>(reader.GetThree());"), std::string::npos);
}

TEST(GeneratorValidationTest, OptionalFieldFollowedByNonOptionalFieldInNextChunk)
{
    EXPECT_NO_THROW(Generate(R"(
        <struct name="S">
            <chunked>
                <field name="a" type="char" optional="true"/>
                <break/>
                <field name="b" type="char"/>
            </chunked>
        </struct>)"));
}

TEST(GeneratorValidationTest, OptionalFieldFollowedByNonOptionalFieldInSameChunk)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <chunked>
                <field name="a" type="char" optional="true"/>
                <field name="b" type="char"/>
                <break/>
            </chunked>
        </struct>)",
                           "Optional fields may not be followed by non-optional fields.");
}

TEST(GeneratorValidationTest, UnsizedArrayFollowedByElement)
{
    EXPECT_GENERATOR_ERROR(R"(<struct name="S"><array name="a" type="char"/><field name="b" type="char"/></struct>)",
                           "Non-delimited arrays without a length must be the final element");
}

TEST(GeneratorValidationTest, UnsizedArrayFollowedByElementInSameChunk)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <chunked>
                <array name="a" type="char"/>
                <field name="b" type="char"/>
            </chunked>
        </struct>)",
                           "Non-delimited arrays without a length must be the final element");
}

TEST(GeneratorValidationTest, UnsizedArrayFollowedByBreak)
{
    EXPECT_NO_THROW(Generate(R"(
        <struct name="S">
            <chunked>
                <array name="a" type="char"/>
                <break/>
                <field name="b" type="char"/>
            </chunked>
        </struct>)"));
}

TEST(GeneratorValidationTest, UnsizedArrayInCaseFollowedByElementAfterSwitch)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="S">
            <field name="n" type="char"/>
            <switch field="n"><case value="1"><array name="a" type="char"/></case></switch>
            <field name="b" type="char"/>
        </struct>)",
                           "Non-delimited arrays without a length must be the final element");
}

TEST(GeneratorValidationTest, SizedAndDelimitedArraysMayBeFollowedByElements)
{
    EXPECT_NO_THROW(Generate(R"(
        <struct name="S">
            <array name="a" type="char" length="2"/>
            <chunked>
                <array name="b" type="string" delimited="true" trailing-delimiter="true"/>
                <field name="c" type="char"/>
            </chunked>
        </struct>)"));
}

TEST(GeneratorValidationTest, UnboundedStructInNonDelimitedArray)
{
    EXPECT_GENERATOR_ERROR(R"(
        <struct name="Inner"><field name="name" type="string"/></struct>
        <struct name="S"><array name="a" type="Inner" length="2"/></struct>)",
                           "Unbounded element type (Inner) forbidden in non-delimited array.");
}

TEST(GeneratorValidationTest, UnnamedHardcodedFieldHasNoMember)
{
    const auto outputs = Generate(R"(<struct name="S"><field type="string">ABC</field></struct>)");
    const auto* header = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    const auto* source = FindOutput(outputs, "src/eolib/protocol/structs.cpp");
    ASSERT_NE(header, nullptr);
    ASSERT_NE(source, nullptr);
    EXPECT_EQ(header->content.find("ABC"), std::string::npos);
    EXPECT_NE(source->content.find("writer.AddString(\"ABC\");"), std::string::npos);
}

TEST(GeneratorValidationTest, NamedHardcodedFieldIsConstant)
{
    const auto outputs = Generate(R"(<struct name="S"><field name="tag" type="string">ABC</field></struct>)");
    const auto* header = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    ASSERT_NE(header, nullptr);
    EXPECT_NE(header->content.find("static constexpr std::string_view tag = \"ABC\";"), std::string::npos);
    EXPECT_EQ(header->content.find("std::string tag{};"), std::string::npos);
}

namespace
{

constexpr const char* PACKET_IDS = R"(
    <enum name="PacketFamily" type="byte"><value name="Connection">1</value></enum>
    <enum name="PacketAction" type="byte"><value name="Request">1</value><value name="Accept">2</value></enum>)";

} // namespace

TEST(GeneratorValidationTest, ClientPacketGeneratesPacketClass)
{
    const auto outputs = eolib::generator::test::ProtocolFixture(
                             {{"net", PACKET_IDS}, {"net/client", R"(<packet family="Connection" action="Request">
                                                   <field name="a" type="char"/>
                                               </packet>)"}})
                             .Generate();
    const auto* header = FindOutput(outputs, "include/eolib/protocol/net/client/packets.hpp");
    ASSERT_NE(header, nullptr);
    EXPECT_NE(header->content.find("class EOLIB_API ConnectionRequestClientPacket final : public net::Packet"),
              std::string::npos);
}

TEST(GeneratorValidationTest, DuplicatePacket)
{
    const std::vector<eolib::generator::test::ProtocolFixture::File> files = {{"net", PACKET_IDS}, {"net/client", R"(
            <packet family="Connection" action="Request"><field name="a" type="char"/></packet>
            <packet family="Connection" action="Request"><field name="b" type="char"/></packet>)"}};
    EXPECT_GENERATOR_ERROR(files, "Connection_Request packet cannot be redefined in the same file.");
}

TEST(GeneratorValidationTest, SamePacketIdInClientAndServerFiles)
{
    const std::string packet = R"(<packet family="Connection" action="Request"><field name="a" type="char"/></packet>)";
    EXPECT_NO_THROW(
        eolib::generator::test::ProtocolFixture({{"net", PACKET_IDS}, {"net/client", packet}, {"net/server", packet}})
            .Generate());
}

TEST(GeneratorValidationTest, InvalidPacketFamily)
{
    const std::vector<eolib::generator::test::ProtocolFixture::File> files = {
        {"net", PACKET_IDS},
        {"net/server", R"(<packet family="Walk" action="Request"><field name="a" type="char"/></packet>)"}};
    EXPECT_GENERATOR_ERROR(files, "Walk is not a valid PacketFamily.");
}

TEST(GeneratorValidationTest, InvalidPacketAction)
{
    const std::vector<eolib::generator::test::ProtocolFixture::File> files = {
        {"net", PACKET_IDS},
        {"net/server", R"(<packet family="Connection" action="Player"><field name="a" type="char"/></packet>)"}};
    EXPECT_GENERATOR_ERROR(files, "Player is not a valid PacketAction.");
}
