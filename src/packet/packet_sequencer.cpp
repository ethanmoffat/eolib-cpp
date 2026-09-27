#include "eolib/packet/packet_sequencer.hpp"

namespace eolib::packet
{

PacketSequencer::PacketSequencer(const SequenceStart& start) noexcept
    : start_(start.Value())
{
}

PacketSequencer::PacketSequencer(int start, int counter) noexcept
    : start_(start),
      counter_(counter)
{
}

int PacketSequencer::NextSequence() noexcept
{
    const int result = start_ + counter_;
    counter_ = (counter_ + 1) % 10;
    return result;
}

void PacketSequencer::SetSequenceStart(const SequenceStart& start) noexcept
{
    start_ = start.Value();
}

PacketSequencer PacketSequencer::WithSequenceStart(const SequenceStart& start) const noexcept
{
    return PacketSequencer(start.Value(), counter_);
}

} // namespace eolib::packet
