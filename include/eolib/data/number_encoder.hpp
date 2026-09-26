#pragma once

#include "eolib/export.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace eolib::data
{

/// Encodes and decodes EO numbers.
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/encoding.md#numbers
class EOLIB_API NumberEncoder
{
public:
    NumberEncoder() = delete;

    /// Encodes a number to a sequence of 4 bytes.
    ///
    /// Values are treated as unsigned 32-bit integers, so values up to <c>EoNumericLimits::IntMax - 1</c> may be
    /// passed as (wrapped) negative numbers.
    static std::array<std::uint8_t, 4> EncodeNumber(int number);

    /// Decodes a number from a sequence of up to 4 bytes.
    static int DecodeNumber(const std::uint8_t* bytes, std::size_t length);

    /// Decodes a number from a sequence of up to 4 bytes.
    static int DecodeNumber(const std::vector<std::uint8_t>& bytes);
};

} // namespace eolib::data
