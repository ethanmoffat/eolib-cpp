#include "eolib/encrypt/data_encrypter.hpp"

#include <algorithm>
#include <stdexcept>

namespace eolib::encrypt
{

void DataEncrypter::Interleave(std::uint8_t* data, std::size_t length)
{
    const std::vector<std::uint8_t> source(data, data + length);

    std::size_t next = 0;
    for (std::size_t i = 0; i < length; i += 2)
    {
        data[i] = source[next++];
    }

    for (std::size_t i = length - length % 2; i > 0; i -= 2)
    {
        data[i - 1] = source[next++];
    }
}

void DataEncrypter::Deinterleave(std::uint8_t* data, std::size_t length)
{
    const std::vector<std::uint8_t> source(data, data + length);

    std::size_t next = 0;
    for (std::size_t i = 0; i < length; i += 2)
    {
        data[next++] = source[i];
    }

    for (std::size_t i = length - length % 2; i > 0; i -= 2)
    {
        data[next++] = source[i - 1];
    }
}

void DataEncrypter::FlipMsb(std::uint8_t* data, std::size_t length) noexcept
{
    for (std::size_t i = 0; i < length; ++i)
    {
        if (data[i] != 0x00 && data[i] != 0x80)
        {
            data[i] = static_cast<std::uint8_t>(data[i] ^ 0x80);
        }
    }
}

void DataEncrypter::SwapMultiples(std::uint8_t* data, std::size_t length, int multiple)
{
    if (multiple < 0)
    {
        throw std::invalid_argument("multiple must not be less than zero");
    }

    if (multiple == 0)
    {
        return;
    }

    // Swapping a sequence of multiples only reorders bytes before the current index, so the remaining bytes can still
    // be checked in place.
    std::size_t sequence_length = 0;
    for (std::size_t i = 0; i <= length; ++i)
    {
        if (i != length && data[i] % multiple == 0)
        {
            ++sequence_length;
            continue;
        }

        std::reverse(data + (i - sequence_length), data + i);
        sequence_length = 0;
    }
}

} // namespace eolib::encrypt
