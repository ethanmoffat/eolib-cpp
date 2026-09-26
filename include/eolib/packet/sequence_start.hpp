#pragma once

#include "eolib/data/eo_numeric_limits.hpp"
#include "eolib/export.hpp"

#include <algorithm>
#include <random>

namespace eolib::packet
{

namespace detail
{

/// Returns a uniformly distributed random value in the range <c>[0, bound)</c>. Returns 0 if bound is not positive.
template <typename Urbg>
int RandomBelow(Urbg& rng, int bound)
{
    if (bound <= 0)
    {
        return 0;
    }
    return std::uniform_int_distribution<int>(0, bound - 1)(rng);
}

/// Gets a thread-local random engine, seeded from <c>std::random_device</c>.
EOLIB_API std::mt19937& DefaultRandomEngine();

} // namespace detail

/// A value sent by the server to update the client's sequence start, also known as the "sequence byte".
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/sequence.md
class EOLIB_API SequenceStart
{
public:
    virtual ~SequenceStart() = default;

    /// Gets the sequence start value.
    virtual int Value() const = 0;

protected:
    SequenceStart() = default;
    SequenceStart(const SequenceStart&) = default;
    SequenceStart(SequenceStart&&) = default;
    SequenceStart& operator=(const SequenceStart&) = default;
    SequenceStart& operator=(SequenceStart&&) = default;
};

/// A sequence start with a value of 0.
class EOLIB_API ZeroSequenceStart final : public SequenceStart
{
public:
    int Value() const override;
};

/// A sequence start sent in the <c>InitInitServerPacket</c> (connection initialization).
class EOLIB_API InitSequenceStart final : public SequenceStart
{
public:
    /// Creates an instance from the <c>seq1</c> and <c>seq2</c> values in the <c>InitInitServerPacket</c>.
    static InitSequenceStart FromInitValues(int seq1, int seq2);

    /// Generates an instance with a random value in the range <c>[0, 1757)</c>.
    template <typename Urbg>
    static InitSequenceStart Generate(Urbg& rng)
    {
        const int value = detail::RandomBelow(rng, 1757);
        const int seq1_max = (value + 13) / 7;
        const int seq1_min = std::max(0, (value - (static_cast<int>(data::EoNumericLimits::CharMax) - 1) + 13 + 6) / 7);

        const int seq1 = detail::RandomBelow(rng, seq1_max - seq1_min) + seq1_min;
        const int seq2 = value - seq1 * 7 + 13;

        return InitSequenceStart(value, seq1, seq2);
    }

    /// Generates an instance using the default random engine.
    static InitSequenceStart Generate();

    int Value() const override;

    /// Gets the <c>seq1</c> byte value sent in the <c>InitInitServerPacket</c>.
    int Seq1() const;

    /// Gets the <c>seq2</c> byte value sent in the <c>InitInitServerPacket</c>.
    int Seq2() const;

private:
    InitSequenceStart(int value, int seq1, int seq2);

    int value_;
    int seq1_;
    int seq2_;
};

/// A sequence start sent in the <c>ConnectionPlayerServerPacket</c> (ping).
class EOLIB_API PingSequenceStart final : public SequenceStart
{
public:
    /// Creates an instance from the <c>seq1</c> and <c>seq2</c> values in the <c>ConnectionPlayerServerPacket</c>.
    static PingSequenceStart FromPingValues(int seq1, int seq2);

    /// Generates an instance with a random value in the range <c>[0, 1757)</c>.
    template <typename Urbg>
    static PingSequenceStart Generate(Urbg& rng)
    {
        const int value = detail::RandomBelow(rng, 1757);
        const int seq1 = value + detail::RandomBelow(rng, static_cast<int>(data::EoNumericLimits::CharMax) - 1);
        const int seq2 = seq1 - value;

        return PingSequenceStart(value, seq1, seq2);
    }

    /// Generates an instance using the default random engine.
    static PingSequenceStart Generate();

    int Value() const override;

    /// Gets the <c>seq1</c> short value sent in the <c>ConnectionPlayerServerPacket</c>.
    int Seq1() const;

    /// Gets the <c>seq2</c> char value sent in the <c>ConnectionPlayerServerPacket</c>.
    int Seq2() const;

private:
    PingSequenceStart(int value, int seq1, int seq2);

    int value_;
    int seq1_;
    int seq2_;
};

/// A sequence start sent in the <c>AccountReplyServerPacket</c>, used by the official EO server.
class EOLIB_API AccountReplySequenceStart final : public SequenceStart
{
public:
    /// Creates an instance with the specified value.
    static AccountReplySequenceStart FromValue(int value);

    /// Generates an instance with a random value in the range <c>[0, 240)</c>.
    template <typename Urbg>
    static AccountReplySequenceStart Generate(Urbg& rng)
    {
        return AccountReplySequenceStart(detail::RandomBelow(rng, 240));
    }

    /// Generates an instance using the default random engine.
    static AccountReplySequenceStart Generate();

    int Value() const override;

private:
    explicit AccountReplySequenceStart(int value);

    int value_;
};

} // namespace eolib::packet
