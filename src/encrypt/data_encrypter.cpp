#include "eolib/encrypt/data_encrypter.hpp"

#include <stdexcept>
#include <utility>

namespace eolib::encrypt
{

std::vector<std::uint8_t> DataEncrypter::Interleave(const std::vector<std::uint8_t>& data)
{
    const std::ptrdiff_t length = static_cast<std::ptrdiff_t>(data.size());
    std::vector<std::uint8_t> result(data.size());

    std::ptrdiff_t i = 0;
    std::ptrdiff_t next = 0;
    for (; i < length; i += 2)
    {
        result[static_cast<std::size_t>(i)] = data[static_cast<std::size_t>(next++)];
    }

    --i;
    if (length % 2 != 0)
    {
        i -= 2;
    }

    for (; i >= 0; i -= 2)
    {
        result[static_cast<std::size_t>(i)] = data[static_cast<std::size_t>(next++)];
    }

    return result;
}

std::vector<std::uint8_t> DataEncrypter::Deinterleave(const std::vector<std::uint8_t>& data)
{
    const std::ptrdiff_t length = static_cast<std::ptrdiff_t>(data.size());
    std::vector<std::uint8_t> result(data.size());

    std::ptrdiff_t i = 0;
    std::ptrdiff_t next = 0;
    for (; i < length; i += 2)
    {
        result[static_cast<std::size_t>(next++)] = data[static_cast<std::size_t>(i)];
    }

    --i;
    if (length % 2 != 0)
    {
        i -= 2;
    }

    for (; i >= 0; i -= 2)
    {
        result[static_cast<std::size_t>(next++)] = data[static_cast<std::size_t>(i)];
    }

    return result;
}

std::vector<std::uint8_t> DataEncrypter::FlipMsb(const std::vector<std::uint8_t>& data)
{
    std::vector<std::uint8_t> result(data);
    for (auto& b : result)
    {
        if (b != 0x00 && b != 0x80)
        {
            b = static_cast<std::uint8_t>(b ^ 0x80);
        }
    }
    return result;
}

std::vector<std::uint8_t> DataEncrypter::SwapMultiples(const std::vector<std::uint8_t>& data, int multiple)
{
    if (multiple < 0)
    {
        throw std::invalid_argument("multiple must not be less than zero");
    }

    std::vector<std::uint8_t> result(data);
    if (multiple == 0)
    {
        return result;
    }

    std::size_t sequence_length = 0;
    for (std::size_t i = 0; i <= data.size(); ++i)
    {
        if (i != data.size() && data[i] % multiple == 0)
        {
            ++sequence_length;
            continue;
        }

        if (sequence_length > 1)
        {
            for (std::size_t j = 0; j < sequence_length / 2; ++j)
            {
                std::swap(result[i - sequence_length + j], result[i - j - 1]);
            }
        }
        sequence_length = 0;
    }

    return result;
}

} // namespace eolib::encrypt
