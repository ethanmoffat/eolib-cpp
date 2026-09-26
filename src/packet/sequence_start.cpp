#include "eolib/packet/sequence_start.hpp"

namespace eolib::packet
{

std::mt19937& detail::DefaultRandomEngine()
{
    thread_local std::mt19937 engine{std::random_device{}()};
    return engine;
}

int ZeroSequenceStart::Value() const
{
    return 0;
}

InitSequenceStart::InitSequenceStart(int value, int seq1, int seq2)
    : value_(value),
      seq1_(seq1),
      seq2_(seq2)
{
}

InitSequenceStart InitSequenceStart::FromInitValues(int seq1, int seq2)
{
    return InitSequenceStart((seq1 * 7) + seq2 - 13, seq1, seq2);
}

InitSequenceStart InitSequenceStart::Generate()
{
    return Generate(detail::DefaultRandomEngine());
}

int InitSequenceStart::Value() const
{
    return value_;
}

int InitSequenceStart::Seq1() const
{
    return seq1_;
}

int InitSequenceStart::Seq2() const
{
    return seq2_;
}

PingSequenceStart::PingSequenceStart(int value, int seq1, int seq2)
    : value_(value),
      seq1_(seq1),
      seq2_(seq2)
{
}

PingSequenceStart PingSequenceStart::FromPingValues(int seq1, int seq2)
{
    return PingSequenceStart(seq1 - seq2, seq1, seq2);
}

PingSequenceStart PingSequenceStart::Generate()
{
    return Generate(detail::DefaultRandomEngine());
}

int PingSequenceStart::Value() const
{
    return value_;
}

int PingSequenceStart::Seq1() const
{
    return seq1_;
}

int PingSequenceStart::Seq2() const
{
    return seq2_;
}

AccountReplySequenceStart::AccountReplySequenceStart(int value)
    : value_(value)
{
}

AccountReplySequenceStart AccountReplySequenceStart::FromValue(int value)
{
    return AccountReplySequenceStart(value);
}

AccountReplySequenceStart AccountReplySequenceStart::Generate()
{
    return Generate(detail::DefaultRandomEngine());
}

int AccountReplySequenceStart::Value() const
{
    return value_;
}

} // namespace eolib::packet
