#pragma once

#include "eolib/data/eo_numeric_limits.hpp"
#include "eolib/export.hpp"

#include <algorithm>
#include <random>

namespace eolib::packet
{

namespace detail
{

/// Gets a uniformly distributed random value in the range <c>[0, bound)</c>.
///
/// @param rng the uniform random bit generator to use.
/// @param bound the exclusive upper bound.
/// @return the random value, or 0 if the bound is not positive.
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
///
/// @return the random engine for the calling thread.
EOLIB_API std::mt19937& DefaultRandomEngine();

} // namespace detail

/// A value sent by the server to update the client's sequence start, also known as the "sequence byte".
class EOLIB_API SequenceStart
{
public:
    virtual ~SequenceStart() = default;

    /// Gets the sequence start value.
    ///
    /// @return the sequence start value.
    virtual int Value() const noexcept = 0;

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
    /// Gets the sequence start value.
    ///
    /// @return the sequence start value.
    int Value() const noexcept override;
};

/// A sequence start sent in the <c>InitInitServerPacket</c> (connection initialization).
class EOLIB_API InitSequenceStart final : public SequenceStart
{
public:
    /// Creates an instance from the <c>seq1</c> and <c>seq2</c> values in the <c>InitInitServerPacket</c>.
    ///
    /// @param seq1 the <c>seq1</c> byte value.
    /// @param seq2 the <c>seq2</c> byte value.
    /// @return the sequence start.
    static InitSequenceStart FromInitValues(int seq1, int seq2) noexcept;

    /// The exclusive upper bound of generated values.
    static constexpr int MaxValue = 1757;

    /// Generates an instance with a random value in the range <c>[0, MaxValue)</c>.
    ///
    /// @param rng the uniform random bit generator to use, e.g. <c>std::mt19937</c>.
    /// @return the generated instance.
    template <typename Urbg>
    static InitSequenceStart Generate(Urbg& rng)
    {
        const int value = detail::RandomBelow(rng, MaxValue);
        const int seq1_max = (value + 13) / 7;
        const int seq1_min =
            (std::max)(0, (value - (static_cast<int>(data::EoNumericLimits::CharMax) - 1) + 13 + 6) / 7);

        const int seq1 = detail::RandomBelow(rng, seq1_max - seq1_min) + seq1_min;
        const int seq2 = value - (seq1 * 7) + 13;

        return InitSequenceStart(value, seq1, seq2);
    }

    /// Generates an instance with a random value in the range <c>[0, MaxValue)</c>, using a thread-local random
    /// engine.
    ///
    /// @return the generated instance.
    static InitSequenceStart Generate();

    /// Gets the sequence start value.
    ///
    /// @return the sequence start value.
    int Value() const noexcept override;

    /// Gets the <c>seq1</c> byte value sent in the <c>InitInitServerPacket</c>.
    ///
    /// @return the <c>seq1</c> value.
    int Seq1() const noexcept;

    /// Gets the <c>seq2</c> byte value sent in the <c>InitInitServerPacket</c>.
    ///
    /// @return the <c>seq2</c> value.
    int Seq2() const noexcept;

private:
    InitSequenceStart(int value, int seq1, int seq2) noexcept;

    int value_;
    int seq1_;
    int seq2_;
};

/// A sequence start sent in the <c>ConnectionPlayerServerPacket</c> (ping).
class EOLIB_API PingSequenceStart final : public SequenceStart
{
public:
    /// Creates an instance from the <c>seq1</c> and <c>seq2</c> values in the <c>ConnectionPlayerServerPacket</c>.
    ///
    /// @param seq1 the <c>seq1</c> short value.
    /// @param seq2 the <c>seq2</c> char value.
    /// @return the sequence start.
    static PingSequenceStart FromPingValues(int seq1, int seq2) noexcept;

    /// The exclusive upper bound of generated values.
    static constexpr int MaxValue = 1757;

    /// Generates an instance with a random value in the range <c>[0, MaxValue)</c>.
    ///
    /// @param rng the uniform random bit generator to use, e.g. <c>std::mt19937</c>.
    /// @return the generated instance.
    template <typename Urbg>
    static PingSequenceStart Generate(Urbg& rng)
    {
        const int value = detail::RandomBelow(rng, MaxValue);
        const int seq1 = value + detail::RandomBelow(rng, static_cast<int>(data::EoNumericLimits::CharMax) - 1);
        const int seq2 = seq1 - value;

        return PingSequenceStart(value, seq1, seq2);
    }

    /// Generates an instance with a random value in the range <c>[0, MaxValue)</c>, using a thread-local random
    /// engine.
    ///
    /// @return the generated instance.
    static PingSequenceStart Generate();

    /// Gets the sequence start value.
    ///
    /// @return the sequence start value.
    int Value() const noexcept override;

    /// Gets the <c>seq1</c> short value sent in the <c>ConnectionPlayerServerPacket</c>.
    ///
    /// @return the <c>seq1</c> value.
    int Seq1() const noexcept;

    /// Gets the <c>seq2</c> char value sent in the <c>ConnectionPlayerServerPacket</c>.
    ///
    /// @return the <c>seq2</c> value.
    int Seq2() const noexcept;

private:
    PingSequenceStart(int value, int seq1, int seq2) noexcept;

    int value_;
    int seq1_;
    int seq2_;
};

/// A sequence start sent in the <c>AccountReplyServerPacket</c>, used by the official EO server.
class EOLIB_API AccountReplySequenceStart final : public SequenceStart
{
public:
    /// Creates an instance with the specified value.
    ///
    /// @param value the sequence start value.
    /// @return the sequence start.
    static AccountReplySequenceStart FromValue(int value) noexcept;

    /// The exclusive upper bound of generated values.
    static constexpr int MaxValue = 240;

    /// Generates an instance with a random value in the range <c>[0, MaxValue)</c>.
    ///
    /// @param rng the uniform random bit generator to use, e.g. <c>std::mt19937</c>.
    /// @return the generated instance.
    template <typename Urbg>
    static AccountReplySequenceStart Generate(Urbg& rng)
    {
        return AccountReplySequenceStart(detail::RandomBelow(rng, MaxValue));
    }

    /// Generates an instance with a random value in the range <c>[0, MaxValue)</c>, using a thread-local random
    /// engine.
    ///
    /// @return the generated instance.
    static AccountReplySequenceStart Generate();

    /// Gets the sequence start value.
    ///
    /// @return the sequence start value.
    int Value() const noexcept override;

private:
    explicit AccountReplySequenceStart(int value) noexcept;

    int value_;
};

} // namespace eolib::packet
