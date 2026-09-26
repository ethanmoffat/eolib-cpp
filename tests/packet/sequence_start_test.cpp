#include "eolib/packet/sequence_start.hpp"

#include <gtest/gtest.h>

#include <random>

using namespace eolib::packet;

namespace
{

TEST(ZeroSequenceStartTest, Value)
{
    EXPECT_EQ(ZeroSequenceStart().Value(), 0);
}

TEST(InitSequenceStartTest, FromInitValues)
{
    const auto start = InitSequenceStart::FromInitValues(244, 34);
    EXPECT_EQ(start.Value(), 1729);
    EXPECT_EQ(start.Seq1(), 244);
    EXPECT_EQ(start.Seq2(), 34);
}

TEST(SequenceStartTest, MaxValuesMatchProtocol)
{
    EXPECT_EQ(InitSequenceStart::MaxValue, 1757);
    EXPECT_EQ(PingSequenceStart::MaxValue, 1757);
    EXPECT_EQ(AccountReplySequenceStart::MaxValue, 240);
}

// Random distributions are implementation-defined, so generated values are validated against their invariants
// rather than fixed expected values.
TEST(InitSequenceStartTest, Generate)
{
    std::mt19937 rng(123);
    for (int i = 0; i < 1000; ++i)
    {
        const auto start = InitSequenceStart::Generate(rng);
        EXPECT_GE(start.Value(), 0);
        EXPECT_LT(start.Value(), 1757);
        EXPECT_GE(start.Seq1(), 0);
        EXPECT_LT(start.Seq1(), 253);
        EXPECT_GE(start.Seq2(), 0);
        EXPECT_LT(start.Seq2(), 253);
        EXPECT_EQ(InitSequenceStart::FromInitValues(start.Seq1(), start.Seq2()).Value(), start.Value());
    }
}

TEST(InitSequenceStartTest, GenerateWithDefaultEngine)
{
    const auto start = InitSequenceStart::Generate();
    EXPECT_EQ(InitSequenceStart::FromInitValues(start.Seq1(), start.Seq2()).Value(), start.Value());
}

TEST(PingSequenceStartTest, FromPingValues)
{
    const auto start = PingSequenceStart::FromPingValues(1763, 34);
    EXPECT_EQ(start.Value(), 1729);
    EXPECT_EQ(start.Seq1(), 1763);
    EXPECT_EQ(start.Seq2(), 34);
}

TEST(PingSequenceStartTest, Generate)
{
    std::mt19937 rng(123);
    for (int i = 0; i < 1000; ++i)
    {
        const auto start = PingSequenceStart::Generate(rng);
        EXPECT_GE(start.Value(), 0);
        EXPECT_LT(start.Value(), 1757);
        EXPECT_GE(start.Seq2(), 0);
        EXPECT_LT(start.Seq2(), 253);
        EXPECT_EQ(PingSequenceStart::FromPingValues(start.Seq1(), start.Seq2()).Value(), start.Value());
    }
}

TEST(AccountReplySequenceStartTest, FromValue)
{
    EXPECT_EQ(AccountReplySequenceStart::FromValue(42).Value(), 42);
}

TEST(AccountReplySequenceStartTest, Generate)
{
    std::mt19937 rng(123);
    for (int i = 0; i < 1000; ++i)
    {
        const auto start = AccountReplySequenceStart::Generate(rng);
        EXPECT_GE(start.Value(), 0);
        EXPECT_LT(start.Value(), 240);
    }
}

} // namespace
