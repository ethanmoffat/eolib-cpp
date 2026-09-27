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
/// <b>Important:</b> Strings are treated as raw byte sequences. No character encoding or transcoding
/// is performed by these functions.
///
/// The official Endless Online client uses <b>Windows-1252</b> encoding for text. If your application
/// uses UTF-8 or another encoding, you are responsible for converting to/from Windows-1252 before/after
/// using these functions.
///
/// @par Example - Handling Windows-1252 Encoding:
/// @code{.cpp}
/// // Pseudo-code - use a proper encoding library (e.g., iconv, ICU, or platform APIs)
/// std::string utf8_text = "Hello";
/// std::vector<uint8_t> win1252_bytes = convert_utf8_to_windows1252(utf8_text);
/// std::vector<uint8_t> encoded = StringEncoder::EncodeString(win1252_bytes);
/// // ... transmit encoded bytes ...
/// std::vector<uint8_t> decoded = StringEncoder::DecodeString(received_bytes);
/// std::string utf8_result = convert_windows1252_to_utf8(decoded);
/// @endcode
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/encoding.md#strings
class EOLIB_API StringEncoder
{
public:
    StringEncoder() = delete;

    /// Encodes a string by modifying it in place.
    static void EncodeInPlace(std::uint8_t* bytes, std::size_t length);

    /// Decodes a string by modifying it in place.
    static void DecodeInPlace(std::uint8_t* bytes, std::size_t length);

    /// Encodes a string and returns the result.
    [[nodiscard]] static std::vector<std::uint8_t> EncodeString(std::vector<std::uint8_t> bytes);

    /// Encodes a string and returns the result.
    [[nodiscard]] static std::string EncodeString(std::string str);

    /// Decodes a string and returns the result.
    [[nodiscard]] static std::vector<std::uint8_t> DecodeString(std::vector<std::uint8_t> bytes);

    /// Decodes a string and returns the result.
    [[nodiscard]] static std::string DecodeString(std::string str);
};

} // namespace eolib::data
