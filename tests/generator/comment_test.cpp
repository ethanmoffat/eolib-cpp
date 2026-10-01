#include "generator_test_utils.hpp"

#include <string>

using eolib::generator::test::FindOutput;
using eolib::generator::test::Generate;

namespace
{

std::string GenerateStructsHeader(const std::string& protocol)
{
    const auto outputs = Generate(protocol);
    const auto* header = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    return header != nullptr ? header->content : std::string();
}

} // namespace

TEST(GeneratorCommentTest, XmlCommentDocumentsNextElement)
{
    const auto header = GenerateStructsHeader(R"(
        <!-- Struct comment. -->
        <struct name="S">
            <!-- Field comment. -->
            <field name="a" type="string"/>
        </struct>)");
    EXPECT_NE(header.find("/// Struct comment.\nclass EOLIB_API S final"), std::string::npos);
    EXPECT_NE(header.find("    /// Field comment.\n    std::string a{};"), std::string::npos);
}

TEST(GeneratorCommentTest, TrailingXmlCommentDocumentsParent)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <field name="a" type="byte"/>
            <!-- Trailing comment. -->
        </struct>)");
    const std::string doc = "/// Trailing comment.";
    const auto position = header.find(doc + "\nclass EOLIB_API S final");
    ASSERT_NE(position, std::string::npos);
    EXPECT_EQ(header.find("Trailing comment.", position + doc.size()), std::string::npos);
}

TEST(GeneratorCommentTest, XmlCommentsMergeAfterCommentElement)
{
    const auto header = GenerateStructsHeader(R"(
        <!-- Preceding. -->
        <struct name="S">
            <comment>Explicit.</comment>
            <field name="a" type="byte"/>
            <!-- Trailing. -->
        </struct>)");
    EXPECT_NE(header.find("/// Explicit.\n/// Trailing.\n/// Preceding.\nclass EOLIB_API S final"), std::string::npos);
}

TEST(GeneratorCommentTest, WrappedCommentLinesAreJoined)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <comment>
                A comment that is
                wrapped.
            </comment>
            <!--
                An XML comment that is
                also wrapped.
            -->
            <field name="a" type="string"/>
        </struct>)");
    EXPECT_NE(header.find("/// A comment that is wrapped.\nclass EOLIB_API S final"), std::string::npos);
    EXPECT_NE(header.find("    /// An XML comment that is also wrapped.\n    std::string a{};"), std::string::npos);
}

TEST(GeneratorCommentTest, XmlCommentInFieldContentIsNotPartOfValue)
{
    const auto outputs = Generate(R"(<struct name="S"><field type="char"><!-- Value comment. -->5</field></struct>)");
    const auto* source = FindOutput(outputs, "src/eolib/protocol/structs.cpp");
    ASSERT_NE(source, nullptr);
    EXPECT_NE(source->content.find("writer.AddChar(5);"), std::string::npos);
}

TEST(GeneratorCommentTest, XmlCommentDocumentsEnumValue)
{
    const auto outputs = Generate(R"(
        <enum name="E" type="char">
            <!-- Value comment. -->
            <value name="A">1</value>
        </enum>)");
    const auto* header = FindOutput(outputs, "include/eolib/protocol/enums.hpp");
    ASSERT_NE(header, nullptr);
    EXPECT_NE(header->content.find("    /// Value comment.\n    A = 1,"), std::string::npos);
}

TEST(GeneratorCommentTest, EmptyCaseCommentsDocumentSwitchDataMember)
{
    const auto outputs = Generate(R"(
        <struct name="S">
            <field name="code" type="char"/>
            <switch field="code">
                <case value="0"><!-- No effect. --></case>
                <case value="1"><field name="a" type="byte"/></case>
                <case value="2"><comment>Ignored.</comment></case>
                <case value="3"><!-- No effect. --></case>
                <case value="4"><!-- No effect. --></case>
                <case default="true"><!-- Ignored. --></case>
            </switch>
        </struct>)");
    const auto* header = FindOutput(outputs, "include/eolib/protocol/structs.hpp");
    const auto* source = FindOutput(outputs, "src/eolib/protocol/structs.cpp");
    ASSERT_NE(header, nullptr);
    ASSERT_NE(source, nullptr);
    EXPECT_NE(header->content.find("    /// Data associated with the `code` field.\n"
                                   "    ///\n"
                                   "    /// When `code` is 0, 3 or 4: No effect.\n"
                                   "    ///\n"
                                   "    /// When `code` is 2 or any other value: Ignored.\n"
                                   "    CodeData code_data{};"),
              std::string::npos);
    EXPECT_NE(source->content.find("        case 0:\n"), std::string::npos);
    EXPECT_NE(source->content.find("        default:\n"), std::string::npos);
}

TEST(GeneratorCommentTest, DummyCommentIsTypeNote)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <comment>Struct comment.</comment>
            <field name="a" type="byte"/>
            <!-- Always sent. -->
            <dummy type="byte">255</dummy>
        </struct>)");
    EXPECT_NE(header.find("/// Struct comment.\n"
                          "///\n"
                          "/// @note\n"
                          "/// - The dummy byte after `a` (always 255): Always sent.\n"
                          "class EOLIB_API S final"),
              std::string::npos);
}

TEST(GeneratorCommentTest, NotesDescribeInstructionsWithoutMembers)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <chunked>
                <!-- Before first. -->
                <break/>
                <!-- Name length. -->
                <length name="name_length" type="char"/>
                <field name="name" type="string" length="name_length"/>
                <!-- After name. -->
                <break/>
            </chunked>
            <!-- Section. -->
            <chunked>
                <!-- String dummy. -->
                <dummy type="string">x</dummy>
            </chunked>
        </struct>)");
    EXPECT_NE(header.find("/// @note\n"
                          "/// - The break byte before `name`: Before first.\n"
                          "/// - The `name_length` length field: Name length.\n"
                          "/// - The break byte after `name`: After name.\n"
                          "/// - The chunked section after `name`: Section.\n"
                          "/// - The dummy string after `name` (always \"x\"): String dummy.\n"
                          "class EOLIB_API S final"),
              std::string::npos);
}

TEST(GeneratorCommentTest, NoteAfterSwitchReferencesDataMember)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <field name="code" type="char"/>
            <switch field="code">
                <case value="1"><field name="a" type="byte"/></case>
            </switch>
            <!-- After switch. -->
            <dummy type="byte">0</dummy>
        </struct>)");
    EXPECT_NE(header.find("/// - The dummy byte after `code_data` (always 0): After switch.\n"), std::string::npos);
}

TEST(GeneratorCommentTest, NoteInCaseDocumentsCaseType)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <field name="code" type="char"/>
            <switch field="code">
                <case value="1">
                    <!-- In case. -->
                    <dummy type="byte">0</dummy>
                </case>
            </switch>
        </struct>)");
    EXPECT_NE(header.find("    /// Data associated with code value 1.\n"
                          "    ///\n"
                          "    /// @note\n"
                          "    /// - The dummy byte (always 0): In case.\n"
                          "    class EOLIB_API CodeData1 final"),
              std::string::npos);
}

TEST(GeneratorCommentTest, UnnamedFieldCommentIsOmitted)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <!-- Hidden. -->
            <field type="char">0</field>
            <field name="a" type="string"/>
        </struct>)");
    EXPECT_EQ(header.find("Hidden."), std::string::npos);
    EXPECT_EQ(header.find("@note"), std::string::npos);
}

TEST(GeneratorCommentTest, NoteEscapesDoxygenCommands)
{
    const auto header = GenerateStructsHeader(R"(
        <struct name="S">
            <!-- Sends @value #1. -->
            <dummy type="byte">0</dummy>
        </struct>)");
    EXPECT_NE(header.find("/// - The dummy byte (always 0): Sends \\@value \\#1.\n"), std::string::npos);
}
