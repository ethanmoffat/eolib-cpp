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
    static std::vector<std::uint8_t> EncodeString(std::vector<std::uint8_t> bytes);

    /// Encodes a string, returning the encoded copy.
    static std::string EncodeString(std::string str);

    /// Decodes a sequence of bytes, returning the decoded copy.
    static std::vector<std::uint8_t> DecodeString(std::vector<std::uint8_t> bytes);

    /// Decodes a string, returning the decoded copy.
    static std::string DecodeString(std::string str);

    /// Encodes a sequence of bytes in place.
    static void EncodeInPlace(std::uint8_t* bytes, std::size_t length);

    /// Decodes a sequence of bytes in place.
    static void DecodeInPlace(std::uint8_t* bytes, std::size_t length);
};

} // namespace eolib::data
