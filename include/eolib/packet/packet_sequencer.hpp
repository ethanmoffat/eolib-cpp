#pragma once

#include "eolib/export.hpp"
#include "eolib/packet/sequence_start.hpp"

namespace eolib::packet
{

/// A class for generating packet sequences.
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/sequence.md
class EOLIB_API PacketSequencer
{
public:
    /// Creates a sequencer with the specified sequence start.
    explicit PacketSequencer(const SequenceStart& start);

    /// Returns the next sequence value, updating the sequence counter in the process.
    ///
    /// This is not a pure function; it should only be called once per packet.
    int NextSequence();

    /// Sets the sequence start, also known as the "starting counter ID". The sequence counter is preserved.
    void SetSequenceStart(const SequenceStart& start);

    /// Returns a new sequencer with the specified sequence start, preserving this sequencer's counter.
    PacketSequencer WithSequenceStart(const SequenceStart& start) const;

private:
    PacketSequencer(int start, int counter);

    int start_;
    int counter_ = 0;
};

} // namespace eolib::packet
