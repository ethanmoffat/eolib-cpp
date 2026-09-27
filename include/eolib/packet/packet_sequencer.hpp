#pragma once

#include "eolib/export.hpp"
#include "eolib/packet/sequence_start.hpp"

namespace eolib::packet
{

/// A class for generating packet sequences.
class EOLIB_API PacketSequencer
{
public:
    /// Creates a sequencer with the specified sequence start.
    ///
    /// @param start the sequence start.
    explicit PacketSequencer(const SequenceStart& start);

    /// Returns the next sequence value, updating the sequence counter in the process.
    ///
    /// This is not a pure function; it should only be called once per packet.
    ///
    /// @return the sequence value for the next packet.
    int NextSequence();

    /// Sets the sequence start, also known as the "starting counter ID". The sequence counter is preserved.
    ///
    /// @param start the new sequence start.
    void SetSequenceStart(const SequenceStart& start);

    /// Creates a new sequencer with the specified sequence start, preserving this sequencer's counter.
    ///
    /// @param start the sequence start for the new sequencer.
    /// @return the new sequencer.
    PacketSequencer WithSequenceStart(const SequenceStart& start) const;

private:
    PacketSequencer(int start, int counter);

    int start_;
    int counter_ = 0;
};

} // namespace eolib::packet
