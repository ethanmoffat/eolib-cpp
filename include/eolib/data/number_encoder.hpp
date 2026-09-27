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
    /// @param number the number to encode. It is treated as an unsigned 32-bit integer, so values up to
    ///               <c>EoNumericLimits::IntMax - 1</c> may be passed as (wrapped) negative numbers.
    /// @return the encoded bytes. Unused high-order bytes are <c>0xFE</c>.
    static std::array<std::uint8_t, 4> EncodeNumber(int number) noexcept;

    /// Decodes a number from a sequence of up to 4 bytes.
    ///
    /// @param bytes a pointer to the encoded bytes.
    /// @param length the number of bytes in <c>bytes</c>. Only the first 4 bytes are decoded.
    /// @return the decoded number. Values above <c>INT_MAX</c> wrap to negative numbers, and malformed <c>0x00</c>
    /// bytes
    ///         decode as -1.
    static int DecodeNumber(const std::uint8_t* bytes, std::size_t length) noexcept;

    /// Decodes a number from a sequence of up to 4 bytes.
    ///
    /// @param bytes the encoded bytes. Only the first 4 bytes are decoded.
    /// @return the decoded number. Values above <c>INT_MAX</c> wrap to negative numbers, and malformed <c>0x00</c>
    /// bytes
    ///         decode as -1.
    static int DecodeNumber(const std::vector<std::uint8_t>& bytes) noexcept;
};

} // namespace eolib::data
