#pragma once

#include "eolib/export.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace eolib::data
{

/// Encodes and decodes EO strings.
///
/// Strings are treated as raw byte sequences (Windows-1252 in the official client). No transcoding is performed.
///
/// All functions transform the data in place. To keep the original data, transform a copy.
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/encoding.md#strings
class EOLIB_API StringEncoder
{
public:
    StringEncoder() = delete;

    /// Encodes a sequence of bytes in place.
    ///
    /// @param bytes a pointer to the bytes to encode.
    /// @param length the number of bytes to encode.
    static void EncodeString(std::uint8_t* bytes, std::size_t length);

    /// Encodes a sequence of bytes in place.
    ///
    /// @param bytes the bytes to encode.
    static void EncodeString(std::vector<std::uint8_t>& bytes)
    {
        EncodeString(bytes.data(), bytes.size());
    }

    /// Encodes a string in place.
    ///
    /// @param str the string to encode.
    static void EncodeString(std::string& str)
    {
        EncodeString(reinterpret_cast<std::uint8_t*>(str.data()), str.size());
    }

    /// Decodes a sequence of bytes in place.
    ///
    /// @param bytes a pointer to the bytes to decode.
    /// @param length the number of bytes to decode.
    static void DecodeString(std::uint8_t* bytes, std::size_t length);

    /// Decodes a sequence of bytes in place.
    ///
    /// @param bytes the bytes to decode.
    static void DecodeString(std::vector<std::uint8_t>& bytes)
    {
        DecodeString(bytes.data(), bytes.size());
    }

    /// Decodes a string in place.
    ///
    /// @param str the string to decode.
    static void DecodeString(std::string& str)
    {
        DecodeString(reinterpret_cast<std::uint8_t*>(str.data()), str.size());
    }
};

} // namespace eolib::data
