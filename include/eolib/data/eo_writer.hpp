#pragma once

#include "eolib/export.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace eolib::data
{

/// A class for writing EO data to a sequence of bytes.
///
/// Strings are treated as raw byte sequences (Windows-1252 in the official client). No transcoding is performed.
class EOLIB_API EoWriter
{
public:
    EoWriter() = default;

    /// Creates a writer with an initial buffer capacity.
    explicit EoWriter(std::size_t capacity);

    /// Adds a raw byte to the writer data.
    /// @throws std::invalid_argument if the value is above <c>0xFF</c>.
    void AddByte(int value);

    /// Adds an array of raw bytes to the writer data.
    void AddBytes(const std::vector<std::uint8_t>& bytes);

    /// Adds an array of raw bytes to the writer data.
    void AddBytes(const std::uint8_t* bytes, std::size_t length);

    /// Adds an encoded 1-byte integer to the writer data.
    /// @throws std::invalid_argument if the value is not below <c>EoNumericLimits::CharMax</c>.
    void AddChar(int number);

    /// Adds an encoded 2-byte integer to the writer data.
    /// @throws std::invalid_argument if the value is not below <c>EoNumericLimits::ShortMax</c>.
    void AddShort(int number);

    /// Adds an encoded 3-byte integer to the writer data.
    /// @throws std::invalid_argument if the value is not below <c>EoNumericLimits::ThreeMax</c>.
    void AddThree(int number);

    /// Adds an encoded 4-byte integer to the writer data.
    /// @throws std::invalid_argument if the value is not below <c>EoNumericLimits::IntMax</c>.
    void AddInt(int number);

    /// Adds a string to the writer data.
    void AddString(std::string_view str);

    /// Adds a fixed-length string to the writer data.
    /// @param padded true if the string should be padded to the length with trailing <c>0xFF</c> bytes.
    /// @throws std::invalid_argument if the string does not have the expected length.
    void AddFixedString(std::string_view str, int length, bool padded = false);

    /// Adds an encoded string to the writer data.
    void AddEncodedString(std::string_view str);

    /// Adds a fixed-length encoded string to the writer data.
    /// @param padded true if the string should be padded to the length with trailing <c>0xFF</c> bytes.
    /// @throws std::invalid_argument if the string does not have the expected length.
    void AddFixedEncodedString(std::string_view str, int length, bool padded = false);

    /// Gets the string sanitization mode for the writer.
    ///
    /// With string sanitization enabled, the writer will switch <c>0xFF</c> bytes in strings (ÿ) to <c>0x79</c> (y).
    ///
    /// See: https://github.com/Cirras/eo-protocol/blob/master/docs/chunks.md#sanitization
    bool GetStringSanitization() const;

    /// Sets the string sanitization mode for the writer.
    void SetStringSanitization(bool string_sanitization);

    /// Gets the length of the writer data.
    int Length() const;

    /// Gets a copy of the writer data as a byte array.
    std::vector<std::uint8_t> ToByteArray() const;

    /// Gets a copy of the writer data as a byte string.
    std::string ToByteString() const;

    /// Gets a read-only view of the writer data.
    const std::vector<std::uint8_t>& Data() const;

private:
    std::vector<std::uint8_t> data_;
    bool string_sanitization_ = false;

    void AddEncodedNumber(int number, unsigned int max, std::size_t size);
    std::vector<std::uint8_t> PrepareString(std::string_view str) const;
};

} // namespace eolib::data
