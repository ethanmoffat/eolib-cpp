#include "names.hpp"

#include <gtest/gtest.h>

using namespace eolib::generator;

TEST(NamesTest, SnakeCaseToPascalCase)
{
    EXPECT_EQ(SnakeCaseToPascalCase("reply_code"), "ReplyCode");
    EXPECT_EQ(SnakeCaseToPascalCase("sub_type"), "SubType");
    EXPECT_EQ(SnakeCaseToPascalCase("x"), "X");
    EXPECT_EQ(SnakeCaseToPascalCase("already_Pascal"), "AlreadyPascal");
    EXPECT_EQ(SnakeCaseToPascalCase("trailing_"), "Trailing");
    EXPECT_EQ(SnakeCaseToPascalCase(""), "");
}

TEST(NamesTest, IsCppKeyword)
{
    EXPECT_TRUE(IsCppKeyword("class"));
    EXPECT_TRUE(IsCppKeyword("default"));
    EXPECT_TRUE(IsCppKeyword("and"));
    EXPECT_FALSE(IsCppKeyword("coords"));
    EXPECT_FALSE(IsCppKeyword("Class"));
}

TEST(NamesTest, FieldIdentifierAppendsUnderscoreToKeywords)
{
    EXPECT_EQ(FieldIdentifier("class"), "class_");
    EXPECT_EQ(FieldIdentifier("new"), "new_");
    EXPECT_EQ(FieldIdentifier("player_id"), "player_id");
}

TEST(NamesTest, MemberIdentifierRenamesKeywordsAndReservedIdentifiers)
{
    EXPECT_EQ(MemberIdentifier("class"), "class_");
    EXPECT_EQ(MemberIdentifier("writer"), "writer_");
    EXPECT_EQ(MemberIdentifier("byte_size"), "byte_size");
    EXPECT_EQ(MemberIdentifier("byte_size_"), "byte_size__");
    EXPECT_EQ(MemberIdentifier("player_id"), "player_id");
}

TEST(NamesTest, StringLiteralEscapesSpecialCharacters)
{
    EXPECT_EQ(StringLiteral("NEW"), "\"NEW\"");
    EXPECT_EQ(StringLiteral("a\"b\\c"), "\"a\\\"b\\\\c\"");
    EXPECT_EQ(StringLiteral(std::string("\xFF", 1)), "\"\\xFF\"");
}

TEST(NamesTest, DocTextEscapesDoxygenCommandCharacters)
{
    EXPECT_EQ(DocText("Walking with #nowall"), "Walking with \\#nowall");
    EXPECT_EQ(DocText("a@b\\c"), "a\\@b\\\\c");
    EXPECT_EQ(DocText("reply_code > 9"), "reply_code > 9");
}

TEST(NamesTest, StringLiteralSplitsHexEscapeFollowedByHexDigit)
{
    const std::string literal = StringLiteral(std::string("\x01"
                                                          "a",
                                                          2));
    EXPECT_EQ(literal, "\"\\x01\" \"a\"");
}
