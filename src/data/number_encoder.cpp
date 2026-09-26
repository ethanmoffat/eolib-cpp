#include "eolib/data/number_encoder.hpp"

#include "eolib/data/eo_numeric_limits.hpp"

#include <algorithm>

namespace eolib::data
{

std::array<std::uint8_t, 4> NumberEncoder::EncodeNumber(int number)
{
    auto value = static_cast<unsigned int>(number);
    const auto original = value;

    unsigned int d = 0xFE;
    if (original >= EoNumericLimits::ThreeMax)
    {
        d = value / EoNumericLimits::ThreeMax + 1;
        value %= EoNumericLimits::ThreeMax;
    }

    unsigned int c = 0xFE;
    if (original >= EoNumericLimits::ShortMax)
    {
        c = value / EoNumericLimits::ShortMax + 1;
        value %= EoNumericLimits::ShortMax;
    }

    unsigned int b = 0xFE;
    if (original >= EoNumericLimits::CharMax)
    {
        b = value / EoNumericLimits::CharMax + 1;
        value %= EoNumericLimits::CharMax;
    }

    const unsigned int a = value + 1;

    return {static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(c),
            static_cast<std::uint8_t>(d)};
}

int NumberEncoder::DecodeNumber(const std::uint8_t* bytes, std::size_t length)
{
    unsigned int result = 0;
    const std::size_t count = std::min<std::size_t>(length, 4);

    for (std::size_t i = 0; i < count; ++i)
    {
        const std::uint8_t b = bytes[i];
        if (b == 0xFE)
        {
            break;
        }

        const unsigned int value = static_cast<unsigned int>(b) - 1;

        switch (i)
        {
            case 0:
                result += value;
                break;
            case 1:
                result += EoNumericLimits::CharMax * value;
                break;
            case 2:
                result += EoNumericLimits::ShortMax * value;
                break;
            case 3:
                result += EoNumericLimits::ThreeMax * value;
                break;
            default:
                break;
        }
    }

    return static_cast<int>(result);
}

int NumberEncoder::DecodeNumber(const std::vector<std::uint8_t>& bytes)
{
    return DecodeNumber(bytes.data(), bytes.size());
}

} // namespace eolib::data
