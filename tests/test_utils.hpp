#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace eolib::test
{

inline std::vector<std::uint8_t> Bytes(std::string_view str)
{
    return std::vector<std::uint8_t>(str.begin(), str.end());
}

inline std::vector<std::uint8_t> Bytes(std::initializer_list<int> values)
{
    std::vector<std::uint8_t> result;
    result.reserve(values.size());
    for (const int value : values)
    {
        result.push_back(static_cast<std::uint8_t>(value));
    }
    return result;
}

} // namespace eolib::test
