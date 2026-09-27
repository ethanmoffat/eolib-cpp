#include "eolib/packet/packet_sequencer.hpp"

#include <gtest/gtest.h>

using namespace eolib::packet;

namespace
{

constexpr int START_VALUE = 123;

TEST(PacketSequencerTest, NextSequenceLoopsOverValuesZeroToNineAboveStartValue)
{
    PacketSequencer sequencer(AccountReplySequenceStart::FromValue(START_VALUE));

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(sequencer.NextSequence(), START_VALUE + i);
    }

    EXPECT_EQ(sequencer.NextSequence(), START_VALUE);
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
