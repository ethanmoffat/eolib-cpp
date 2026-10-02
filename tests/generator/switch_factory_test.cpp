#include "generator_test_utils.hpp"

#include <string>

using eolib::generator::test::FindOutput;
using eolib::generator::test::Generate;

namespace
{

const std::string REPLY_ENUM = R"(
    <enum name="Reply" type="char">
        <value name="Ok">1</value>
        <value name="Empty">2</value>
        <value name="Missing">3</value>
        <value name="Nested">4</value>
    </enum>
    <enum name="Kind" type="char">
        <value name="First">1</value>
        <value name="Second">2</value>
    </enum>)";

std::string GenerateStructs(const std::string& protocol, const std::string& file = "include")
{
    const auto outputs = Generate(REPLY_ENUM + protocol);
    const auto* output = FindOutput(outputs, file == "include" ? "include/eolib/protocol/structs.hpp"
                                                               : "src/eolib/protocol/structs.cpp");
    return output != nullptr ? output->content : std::string();
}

bool Contains(const std::string& content, const std::string& text)
{
    return content.find(text) != std::string::npos;
}

} // namespace

TEST(GeneratorSwitchFactoryTest, NamedValue_CaseWithData_FactoryTakesData)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"><field name="value" type="short"/></case>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "static Thing ForOk(ReplyDataOk data);"));
}

TEST(GeneratorSwitchFactoryTest, NamedValue_EmptyOrMissingCase_FactoryHasNoParameters)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"><field name="value" type="short"/></case>
                <case value="Empty"/>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "static Thing ForEmpty();"));
    EXPECT_TRUE(Contains(header, "static Thing ForMissing();"));
    EXPECT_TRUE(Contains(header, "static Thing ForNested();"));
}

TEST(GeneratorSwitchFactoryTest, NamedValue_CaseWithoutMembers_EmplacesCaseData)
{
    const auto source = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"><field type="string">NO</field></case>
            </switch>
        </struct>)",
                                        "src");

    EXPECT_TRUE(Contains(source, "Thing Thing::ForOk()\n{\n    Thing result;\n    result.reply = Reply::Ok;\n"
                                 "    result.reply_data.emplace<ReplyDataOk>();\n    return result;\n}"));
}

TEST(GeneratorSwitchFactoryTest, NamedValue_WithoutCaseAndWithDefault_UsesDefaultFactory)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"/>
                <case default="true"><field name="value" type="short"/></case>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "static Thing ForOk();"));
    EXPECT_FALSE(Contains(header, "ForMissing"));
    EXPECT_TRUE(Contains(header, "static Thing ForReplyDefault(Reply code, ReplyDataDefault data);"));
}

TEST(GeneratorSwitchFactoryTest, DefaultCase_RejectsCaseValues)
{
    const auto source = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"/>
                <case value="9"/>
                <case default="true"><field name="value" type="short"/></case>
            </switch>
        </struct>)",
                                        "src");

    EXPECT_TRUE(Contains(source, "switch (static_cast<int>(code))\n    {\n"
                                 "        case static_cast<int>(Reply::Ok):\n        case 9:\n"
                                 "            throw std::invalid_argument("));
}

TEST(GeneratorSwitchFactoryTest, IntegerCase_WithData_NamedByCaseType)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="0"><field name="value" type="short"/></case>
                <case value="9"/>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "static Thing ForReplyData0(ReplyData0 data);"));
    EXPECT_FALSE(Contains(header, "ForReplyData9"));
}

TEST(GeneratorSwitchFactoryTest, IntegerSwitchField_FactoriesForDataCasesOnly)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="number" type="char"/>
            <switch field="number">
                <case value="1"><field name="value" type="short"/></case>
                <case value="2"/>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "static Thing ForNumberData1(NumberData1 data);"));
    EXPECT_FALSE(Contains(header, "ForNumberData2"));
}

TEST(GeneratorSwitchFactoryTest, NestedSwitch_FactoriesLiftedToTopLevel)
{
    const std::string protocol = R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Nested">
                    <field name="kind" type="Kind"/>
                    <switch field="kind">
                        <case value="First"><field name="value" type="short"/></case>
                        <case value="0"><field name="other" type="short"/></case>
                    </switch>
                </case>
            </switch>
        </struct>)";
    const auto header = GenerateStructs(protocol);
    const auto source = GenerateStructs(protocol, "src");

    EXPECT_TRUE(Contains(header, "static Thing ForNestedFirst(ReplyDataNested::KindDataFirst data);"));
    EXPECT_TRUE(Contains(header, "static Thing ForNestedSecond();"));
    EXPECT_TRUE(Contains(header, "static Thing ForKindData0(ReplyDataNested::KindData0 data);"));
    EXPECT_FALSE(Contains(header, "static ReplyDataNested For"));
    EXPECT_FALSE(Contains(header, "ForNested()"));
    EXPECT_TRUE(Contains(source,
                         "    result.reply = Reply::Nested;\n"
                         "    auto& case_data_0 = result.reply_data.emplace<ReplyDataNested>();\n"
                         "    case_data_0.kind = Kind::First;\n"
                         "    case_data_0.kind_data.emplace<ReplyDataNested::KindDataFirst>(std::move(data));\n"));
}

TEST(GeneratorSwitchFactoryTest, NestedSwitch_DefaultFactoryTakesCode)
{
    const std::string protocol = R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Nested">
                    <field name="kind" type="Kind"/>
                    <switch field="kind">
                        <case value="First"/>
                        <case default="true"><field name="value" type="short"/></case>
                    </switch>
                </case>
            </switch>
        </struct>)";
    const auto header = GenerateStructs(protocol);
    const auto source = GenerateStructs(protocol, "src");

    EXPECT_TRUE(Contains(header, "static Thing ForKindDefault(Kind code, ReplyDataNested::KindDataDefault data);"));
    EXPECT_TRUE(Contains(source, "    switch (code)\n    {\n        case Kind::First:\n"));
    EXPECT_TRUE(Contains(source, "    case_data_0.kind = code;\n"));
}

TEST(GeneratorSwitchFactoryTest, MultipleSwitches_Throws)
{
    EXPECT_GENERATOR_ERROR(REPLY_ENUM + R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"><field name="value" type="short"/></case>
            </switch>
            <field name="kind" type="Kind"/>
            <switch field="kind">
                <case value="First"/>
            </switch>
        </struct>)",
                           "multiple switches in one scope (switches on reply and kind)");
}

TEST(GeneratorSwitchFactoryTest, MultipleSwitchesAcrossChunks_Throws)
{
    EXPECT_GENERATOR_ERROR(REPLY_ENUM + R"(
        <struct name="Thing">
            <chunked>
                <field name="reply" type="Reply"/>
                <switch field="reply">
                    <case value="Ok"><field name="value" type="short"/></case>
                </switch>
                <break/>
                <field name="kind" type="Kind"/>
                <switch field="kind">
                    <case value="First"/>
                </switch>
            </chunked>
        </struct>)",
                           "multiple switches in one scope (switches on reply and kind)");
}

TEST(GeneratorSwitchFactoryTest, MultipleSwitchesInCase_Throws)
{
    EXPECT_GENERATOR_ERROR(REPLY_ENUM + R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Nested">
                    <field name="kind" type="Kind"/>
                    <switch field="kind">
                        <case value="First"><field name="value" type="short"/></case>
                    </switch>
                    <field name="other" type="Kind"/>
                    <switch field="other">
                        <case value="First"/>
                    </switch>
                </case>
            </switch>
        </struct>)",
                           "multiple switches in one scope (switches on kind and other)");
}

TEST(GeneratorSwitchFactoryTest, DefaultCase_WithoutMembers_TakesOnlyCode)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"/>
                <case default="true"><field type="string">OK</field></case>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "static Thing ForReplyDefault(Reply code);"));
}

TEST(GeneratorSwitchFactoryTest, Accessors_NamedByCase)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Ok"><field name="value" type="short"/></case>
                <case value="Empty"/>
                <case value="0"><field name="zero" type="short"/></case>
                <case default="true"><field name="other" type="short"/></case>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "const ReplyDataOk* AsOk() const noexcept"));
    EXPECT_TRUE(Contains(header, "ReplyDataOk* AsOk() noexcept"));
    EXPECT_TRUE(Contains(header, "const ReplyData0* AsReplyData0() const noexcept"));
    EXPECT_TRUE(Contains(header, "const ReplyDataDefault* AsReplyDefault() const noexcept"));
    EXPECT_FALSE(Contains(header, "AsEmpty"));
}

TEST(GeneratorSwitchFactoryTest, NestedSwitch_AccessorsOnCaseClass)
{
    const auto header = GenerateStructs(R"(
        <struct name="Thing">
            <field name="reply" type="Reply"/>
            <switch field="reply">
                <case value="Nested">
                    <field name="kind" type="Kind"/>
                    <switch field="kind">
                        <case value="First"><field name="value" type="short"/></case>
                    </switch>
                </case>
            </switch>
        </struct>)");

    EXPECT_TRUE(Contains(header, "const ReplyDataNested* AsNested() const noexcept"));
    EXPECT_TRUE(Contains(header, "const KindDataFirst* AsFirst() const noexcept"));
}
