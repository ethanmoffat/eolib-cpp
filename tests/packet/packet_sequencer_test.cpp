#include "eolib/packet/packet_sequencer.hpp"

#include <gtest/gtest.h>

using namespace eolib::packet;

namespace
{

TEST(PacketSequencerTest, NextSequenceLoopsOverValuesZeroToNineAboveStartValue)
{
    constexpr int kStartValue = 123;
    PacketSequencer sequencer(AccountReplySequenceStart::FromValue(kStartValue));

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(sequencer.NextSequence(), kStartValue + i);
    }

    EXPECT_EQ(sequencer.NextSequence(), kStartValue);
}

TEST(PacketSequencerTest, WithSequenceStartPreservesCounter)
{
    PacketSequencer sequencer(AccountReplySequenceStart::FromValue(100));
    EXPECT_EQ(sequencer.NextSequence(), 100);

    auto sequencer2 = sequencer.WithSequenceStart(AccountReplySequenceStart::FromValue(200));
    EXPECT_EQ(sequencer2.NextSequence(), 201);
    EXPECT_EQ(sequencer.NextSequence(), 101);
}

TEST(PacketSequencerTest, SetSequenceStartPreservesCounter)
{
    PacketSequencer sequencer(ZeroSequenceStart{});
    EXPECT_EQ(sequencer.NextSequence(), 0);
    EXPECT_EQ(sequencer.NextSequence(), 1);

    sequencer.SetSequenceStart(PingSequenceStart::FromPingValues(1763, 34));
    EXPECT_EQ(sequencer.NextSequence(), 1731);
}

} // namespace
