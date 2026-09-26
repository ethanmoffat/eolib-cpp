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
    EXPECT_NE(structs->content.find("class EOLIB_API Thing : public Serializable"), std::string::npos);
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
