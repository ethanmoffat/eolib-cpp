#pragma once

#include "eolib/export.hpp"

#include <cstdint>
#include <vector>

namespace eolib::encrypt
{

/// Functions for encrypting and decrypting EO data.
///
/// Encryption is applied to packet data after the 2-byte packet ID and before the packet length is prepended. The
/// server encrypts with <c>FlipMsb</c>, <c>Interleave</c> and <c>SwapMultiples</c>; decryption applies the inverse in
/// reverse order.
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/encryption.md
class EOLIB_API DataEncrypter
{
public:
    DataEncrypter() = delete;

    /// Interleaves a sequence of bytes. When encrypting EO data, this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 5, 1, 4, 2, 3}</c>.
    [[nodiscard]] static std::vector<std::uint8_t> Interleave(const std::vector<std::uint8_t>& data);

    /// Deinterleaves a sequence of bytes. This is the reverse of <c>Interleave</c>. When decrypting EO data, this
    /// function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 2, 4, 5, 3, 1}</c>.
    [[nodiscard]] static std::vector<std::uint8_t> Deinterleave(const std::vector<std::uint8_t>& data);

    /// Flips the most significant bits of each byte in a sequence of bytes. Values <c>0x00</c> and <c>0x80</c> are
    /// not flipped. When encrypting and decrypting EO data, this function is called first.
    [[nodiscard]] static std::vector<std::uint8_t> FlipMsb(const std::vector<std::uint8_t>& data);

    /// Swaps the order of contiguous bytes in a sequence of bytes that are divisible by a given multiple value. When
    /// encrypting and decrypting EO data, this function is called third.
    ///
    /// @param multiple the multiple value; a value of 0 leaves the data unchanged.
    /// @throws std::invalid_argument if the multiple is negative.
    [[nodiscard]] static std::vector<std::uint8_t> SwapMultiples(const std::vector<std::uint8_t>& data, int multiple);
};

} // namespace eolib::encrypt
