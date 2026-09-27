#include "eolib/packet/sequence_start.hpp"

namespace eolib::packet
{

std::mt19937& detail::DefaultRandomEngine()
{
    thread_local std::mt19937 engine{std::random_device{}()};
    return engine;
}

int ZeroSequenceStart::Value() const noexcept
{
    return 0;
}

InitSequenceStart::InitSequenceStart(int value, int seq1, int seq2) noexcept
    : value_(value),
      seq1_(seq1),
      seq2_(seq2)
{
}

InitSequenceStart InitSequenceStart::FromInitValues(int seq1, int seq2) noexcept
{
    return InitSequenceStart((seq1 * 7) + seq2 - 13, seq1, seq2);
}

InitSequenceStart InitSequenceStart::Generate()
{
    return Generate(detail::DefaultRandomEngine());
}

int InitSequenceStart::Value() const noexcept
{
    return value_;
}

int InitSequenceStart::Seq1() const noexcept
{
    return seq1_;
}

int InitSequenceStart::Seq2() const noexcept
{
    return seq2_;
}

PingSequenceStart::PingSequenceStart(int value, int seq1, int seq2) noexcept
    : value_(value),
      seq1_(seq1),
      seq2_(seq2)
{
}

PingSequenceStart PingSequenceStart::FromPingValues(int seq1, int seq2) noexcept
{
    return PingSequenceStart(seq1 - seq2, seq1, seq2);
}

PingSequenceStart PingSequenceStart::Generate()
{
    return Generate(detail::DefaultRandomEngine());
}

int PingSequenceStart::Value() const noexcept
{
    return value_;
}

int PingSequenceStart::Seq1() const noexcept
{
    return seq1_;
}

int PingSequenceStart::Seq2() const noexcept
{
    return seq2_;
}

AccountReplySequenceStart::AccountReplySequenceStart(int value) noexcept
    : value_(value)
{
}

AccountReplySequenceStart AccountReplySequenceStart::FromValue(int value) noexcept
{
    return AccountReplySequenceStart(value);
}

AccountReplySequenceStart AccountReplySequenceStart::Generate()
{
    return Generate(detail::DefaultRandomEngine());
}

int AccountReplySequenceStart::Value() const noexcept
{
    return value_;
}

} // namespace eolib::packet
