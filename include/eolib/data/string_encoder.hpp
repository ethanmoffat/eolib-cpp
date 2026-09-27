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
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/encoding.md#strings
class EOLIB_API StringEncoder
{
public:
    StringEncoder() = delete;

    /// Encodes a sequence of bytes, returning the encoded copy.
    ///
    /// @param bytes the bytes to encode.
    /// @return the encoded bytes.
    static std::vector<std::uint8_t> EncodeString(std::vector<std::uint8_t> bytes);

    /// Encodes a string, returning the encoded copy.
    ///
    /// @param str the string to encode.
    /// @return the encoded string.
    static std::string EncodeString(std::string str);

    /// Decodes a sequence of bytes, returning the decoded copy.
    ///
    /// @param bytes the bytes to decode.
    /// @return the decoded bytes.
    static std::vector<std::uint8_t> DecodeString(std::vector<std::uint8_t> bytes);

    /// Decodes a string, returning the decoded copy.
    ///
    /// @param str the string to decode.
    /// @return the decoded string.
    static std::string DecodeString(std::string str);

    /// Encodes a sequence of bytes in place.
    ///
    /// @param bytes a pointer to the bytes to encode.
    /// @param length the number of bytes to encode.
    static void EncodeInPlace(std::uint8_t* bytes, std::size_t length);

    /// Decodes a sequence of bytes in place.
    ///
    /// @param bytes a pointer to the bytes to decode.
    /// @param length the number of bytes to decode.
    static void DecodeInPlace(std::uint8_t* bytes, std::size_t length);
};

} // namespace eolib::data
