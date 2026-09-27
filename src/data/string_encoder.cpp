#include "eolib/data/string_encoder.hpp"

#include <algorithm>

namespace eolib::data
{

namespace
{

void InvertCharacters(std::uint8_t* bytes, std::size_t length) noexcept
{
    bool flippy = length % 2 == 1;

    for (std::size_t i = 0; i < length; ++i)
    {
        const int c = bytes[i];
        int f = 0;

        if (flippy)
        {
            f = 0x2E;
            if (c >= 0x50)
            {
                f *= -1;
            }
        }

        if (c >= 0x22 && c <= 0x7E)
        {
            bytes[i] = static_cast<std::uint8_t>(0x9F - c - f);
        }

        flippy = !flippy;
    }
}

} // namespace

void StringEncoder::EncodeString(std::uint8_t* bytes, std::size_t length) noexcept
{
    InvertCharacters(bytes, length);
    std::reverse(bytes, bytes + length);
}

void StringEncoder::DecodeString(std::uint8_t* bytes, std::size_t length) noexcept
{
    std::reverse(bytes, bytes + length);
    InvertCharacters(bytes, length);
}

} // namespace eolib::data
