#pragma once

#include "eolib/export.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace eolib::encrypt
{

/// Functions for encrypting and decrypting EO data.
///
/// Encryption is applied to packet data after the 2-byte packet ID and before the packet length is prepended. The
/// data is encrypted with <c>SwapMultiples</c>, <c>Interleave</c> and then <c>FlipMsb</c>, and decrypted with
/// <c>FlipMsb</c>, <c>Deinterleave</c> and then <c>SwapMultiples</c>.
///
/// All functions transform the data in place. To keep the original data, transform a copy.
class EOLIB_API DataEncrypter
{
public:
    DataEncrypter() = delete;

    /// Interleaves a sequence of bytes in place. When encrypting EO data, this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 5, 1, 4, 2, 3}</c>.
    ///
    /// @param data a pointer to the data to interleave.
    /// @param length the number of bytes to interleave.
    static void Interleave(std::uint8_t* data, std::size_t length);

    /// Interleaves a sequence of bytes in place. When encrypting EO data, this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 5, 1, 4, 2, 3}</c>.
    ///
    /// @param data the data to interleave.
    static void Interleave(std::vector<std::uint8_t>& data)
    {
        Interleave(data.data(), data.size());
    }

    /// Interleaves a sequence of bytes in place. When encrypting EO data, this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 5, 1, 4, 2, 3}</c>.
    ///
    /// @param data the data to interleave.
    static void Interleave(std::string& data)
    {
        Interleave(reinterpret_cast<std::uint8_t*>(data.data()), data.size());
    }

    /// Deinterleaves a sequence of bytes in place. This is the reverse of <c>Interleave</c>. When decrypting EO data,
    /// this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 2, 4, 5, 3, 1}</c>.
    ///
    /// @param data a pointer to the data to deinterleave.
    /// @param length the number of bytes to deinterleave.
    static void Deinterleave(std::uint8_t* data, std::size_t length);

    /// Deinterleaves a sequence of bytes in place. This is the reverse of <c>Interleave</c>. When decrypting EO data,
    /// this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 2, 4, 5, 3, 1}</c>.
    ///
    /// @param data the data to deinterleave.
    static void Deinterleave(std::vector<std::uint8_t>& data)
    {
        Deinterleave(data.data(), data.size());
    }

    /// Deinterleaves a sequence of bytes in place. This is the reverse of <c>Interleave</c>. When decrypting EO data,
    /// this function is called second.
    ///
    /// Example: <c>{0, 1, 2, 3, 4, 5}</c> becomes <c>{0, 2, 4, 5, 3, 1}</c>.
    ///
    /// @param data the data to deinterleave.
    static void Deinterleave(std::string& data)
    {
        Deinterleave(reinterpret_cast<std::uint8_t*>(data.data()), data.size());
    }

    /// Flips the most significant bits of each byte in a sequence of bytes in place. Values <c>0x00</c> and
    /// <c>0x80</c> are not flipped. This function is called last when encrypting EO data, and first when decrypting it.
    ///
    /// @param data a pointer to the data to flip.
    /// @param length the number of bytes to flip.
    static void FlipMsb(std::uint8_t* data, std::size_t length);

    /// Flips the most significant bits of each byte in a sequence of bytes in place. Values <c>0x00</c> and
    /// <c>0x80</c> are not flipped. This function is called last when encrypting EO data, and first when decrypting it.
    ///
    /// @param data the data to flip.
    static void FlipMsb(std::vector<std::uint8_t>& data)
    {
        FlipMsb(data.data(), data.size());
    }

    /// Flips the most significant bits of each byte in a sequence of bytes in place. Values <c>0x00</c> and
    /// <c>0x80</c> are not flipped. This function is called last when encrypting EO data, and first when decrypting it.
    ///
    /// @param data the data to flip.
    static void FlipMsb(std::string& data)
    {
        FlipMsb(reinterpret_cast<std::uint8_t*>(data.data()), data.size());
    }

    /// Swaps the order of contiguous bytes in a sequence of bytes that are divisible by a given multiple value, in
    /// place. This function is called first when encrypting EO data, and last when decrypting it.
    ///
    /// @param data a pointer to the data to swap.
    /// @param length the number of bytes to swap.
    /// @param multiple the multiple value; a value of 0 leaves the data unchanged.
    /// @throws std::invalid_argument if the multiple is negative. The data is not modified.
    static void SwapMultiples(std::uint8_t* data, std::size_t length, int multiple);

    /// Swaps the order of contiguous bytes in a sequence of bytes that are divisible by a given multiple value, in
    /// place. This function is called first when encrypting EO data, and last when decrypting it.
    ///
    /// @param data the data to swap.
    /// @param multiple the multiple value; a value of 0 leaves the data unchanged.
    /// @throws std::invalid_argument if the multiple is negative. The data is not modified.
    static void SwapMultiples(std::vector<std::uint8_t>& data, int multiple)
    {
        SwapMultiples(data.data(), data.size(), multiple);
    }

    /// Swaps the order of contiguous bytes in a sequence of bytes that are divisible by a given multiple value, in
    /// place. This function is called first when encrypting EO data, and last when decrypting it.
    ///
    /// @param data the data to swap.
    /// @param multiple the multiple value; a value of 0 leaves the data unchanged.
    /// @throws std::invalid_argument if the multiple is negative. The data is not modified.
    static void SwapMultiples(std::string& data, int multiple)
    {
        SwapMultiples(reinterpret_cast<std::uint8_t*>(data.data()), data.size(), multiple);
    }
};

} // namespace eolib::encrypt
