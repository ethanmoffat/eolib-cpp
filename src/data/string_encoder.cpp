#include "eolib/data/string_encoder.hpp"

#include <algorithm>

namespace eolib::data
{

namespace
{

void InvertCharacters(std::uint8_t* bytes, std::size_t length)
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

void StringEncoder::EncodeInPlace(std::uint8_t* bytes, std::size_t length)
{
    InvertCharacters(bytes, length);
    std::reverse(bytes, bytes + length);
}

void StringEncoder::DecodeInPlace(std::uint8_t* bytes, std::size_t length)
{
    std::reverse(bytes, bytes + length);
    InvertCharacters(bytes, length);
}

std::vector<std::uint8_t> StringEncoder::EncodeString(std::vector<std::uint8_t> bytes)
{
    EncodeInPlace(bytes.data(), bytes.size());
    return bytes;
}

std::string StringEncoder::EncodeString(std::string str)
{
    EncodeInPlace(reinterpret_cast<std::uint8_t*>(str.data()), str.size());
    return str;
}

std::vector<std::uint8_t> StringEncoder::DecodeString(std::vector<std::uint8_t> bytes)
{
    DecodeInPlace(bytes.data(), bytes.size());
    return bytes;
}

std::string StringEncoder::DecodeString(std::string str)
{
    DecodeInPlace(reinterpret_cast<std::uint8_t*>(str.data()), str.size());
    return str;
}

} // namespace eolib::data
