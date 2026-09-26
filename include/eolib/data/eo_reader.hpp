#pragma once

#include "eolib/export.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace eolib::data
{

/// A class for reading EO data from a sequence of bytes.
///
/// <c>EoReader</c> features a chunked reading mode, which is important for accurate emulation of the official game
/// client.
///
/// Reads past the end of the data (or the current chunk) do not throw; numeric reads return 0 and string/byte reads
/// return truncated results.
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/chunks.md
class EOLIB_API EoReader
{
public:
    /// Creates a reader over the specified data.
    explicit EoReader(std::vector<std::uint8_t> data);

    /// Creates a reader over a copy of the specified data.
    explicit EoReader(std::string_view data);

    /// Creates a reader over a copy of the specified data.
    EoReader(const std::uint8_t* data, std::size_t length);

    /// Creates a new reader from a slice of this reader's data, starting at the current position.
    ///
    /// The new reader shares the underlying data but has independent position and chunked reading state.
    EoReader Slice() const;

    /// Creates a new reader from a slice of this reader's data, starting at the specified index.
    /// @throws std::invalid_argument if the index is negative.
    EoReader Slice(int index) const;

    /// Creates a new reader from a slice of this reader's data, with the specified index and length.
    /// @throws std::invalid_argument if the index or length is negative.
    EoReader Slice(int index, int length) const;

    /// Reads a raw byte from the input data.
    int GetByte();

    /// Reads an array of raw bytes from the input data.
    std::vector<std::uint8_t> GetBytes(int length);

    /// Reads an encoded 1-byte integer from the input data.
    int GetChar();

    /// Reads an encoded 2-byte integer from the input data.
    int GetShort();

    /// Reads an encoded 3-byte integer from the input data.
    int GetThree();

    /// Reads an encoded 4-byte integer from the input data.
    int GetInt();

    /// Reads a string from the input data (all remaining data, or the rest of the current chunk).
    std::string GetString();

    /// Reads a fixed-length string from the input data.
    /// @param padded true if the string is padded with trailing <c>0xFF</c> bytes.
    /// @throws std::invalid_argument if the length is negative.
    std::string GetFixedString(int length, bool padded = false);

    /// Reads an encoded string from the input data (all remaining data, or the rest of the current chunk).
    std::string GetEncodedString();

    /// Reads a fixed-length encoded string from the input data.
    /// @param padded true if the string is padded with trailing <c>0xFF</c> bytes.
    /// @throws std::invalid_argument if the length is negative.
    std::string GetFixedEncodedString(int length, bool padded = false);

    /// Gets the chunked reading mode for the reader.
    bool GetChunkedReadingMode() const;

    /// Sets the chunked reading mode for the reader.
    ///
    /// In chunked reading mode, the reader will treat <c>0xFF</c> bytes as the boundaries between chunks of data.
    void SetChunkedReadingMode(bool chunked_reading_mode);

    /// Moves the reader position to the start of the next chunk.
    /// @throws std::logic_error if not in chunked reading mode.
    void NextChunk();

    /// Gets the number of bytes remaining in the input data (or the current chunk in chunked reading mode).
    int Remaining() const;

    /// Gets the current position in the input data.
    int Position() const;

private:
    EoReader(std::shared_ptr<const std::vector<std::uint8_t>> data, int offset, int limit);

    std::shared_ptr<const std::vector<std::uint8_t>> data_;
    int offset_ = 0;
    int limit_ = 0;
    int position_ = 0;
    bool chunked_reading_mode_ = false;
    int chunk_start_ = 0;
    int next_break_ = -1;

    std::uint8_t ReadByte();
    std::vector<std::uint8_t> ReadBytes(int length);
    int FindNextBreakIndex() const;
};

} // namespace eolib::data
