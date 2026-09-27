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
/// A reader either owns its data (the constructors, which copy or take the data) or refers to data owned by the caller
/// (<c>View</c>, which doesn't copy). Copies and slices of a reader share its data, and are views if it is a view.
///
/// See: https://github.com/Cirras/eo-protocol/blob/master/docs/chunks.md
class EOLIB_API EoReader
{
public:
    /// Creates a reader over the specified data.
    ///
    /// @param data the data to read.
    explicit EoReader(std::vector<std::uint8_t> data);

    /// Creates a reader over a copy of the specified data.
    ///
    /// @param data the data to read, as a byte string.
    explicit EoReader(std::string_view data);

    /// Creates a reader over a copy of the specified data.
    ///
    /// @param data a pointer to the data to read.
    /// @param length the number of bytes to read from <c>data</c>.
    EoReader(const std::uint8_t* data, std::size_t length);

    /// Creates a reader that refers to the specified data without copying it.
    ///
    /// The data must outlive the reader, and all copies and slices of it. Generated types copy their fields out of the
    /// reader, so the data only needs to live until deserialization is complete.
    ///
    /// @param data the data to read, as a byte string.
    /// @return a reader over the data.
    static EoReader View(std::string_view data) noexcept;

    /// Creates a reader that refers to the specified data without copying it.
    ///
    /// The data must outlive the reader, and all copies and slices of it. Generated types copy their fields out of the
    /// reader, so the data only needs to live until deserialization is complete.
    ///
    /// @param data a pointer to the data to read.
    /// @param length the number of bytes to read from <c>data</c>.
    /// @return a reader over the data.
    static EoReader View(const std::uint8_t* data, std::size_t length) noexcept;

    /// Creates a reader that refers to the specified data without copying it.
    ///
    /// The data must outlive the reader, and all copies and slices of it. Generated types copy their fields out of the
    /// reader, so the data only needs to live until deserialization is complete.
    ///
    /// @param data the data to read.
    /// @return a reader over the data.
    static EoReader View(const std::vector<std::uint8_t>& data) noexcept;

    /// Deleted to prevent creating a view over a temporary, which would dangle. Use the constructor instead.
    static EoReader View(std::vector<std::uint8_t>&& data) = delete;

    /// Creates a reader that shares this reader's data, with an independent copy of its position and chunked reading
    /// state.
    ///
    /// @param other the reader to copy.
    EoReader(const EoReader& other) = default;

    /// Creates a reader that takes over another reader's data and state. The other reader is left empty.
    ///
    /// @param other the reader to move from.
    EoReader(EoReader&& other) noexcept;

    /// Replaces this reader's data and state with a copy of another reader's. The data is shared.
    ///
    /// @param other the reader to copy.
    /// @return this reader.
    EoReader& operator=(const EoReader& other) = default;

    /// Replaces this reader's data and state with another reader's. The other reader is left empty.
    ///
    /// @param other the reader to move from.
    /// @return this reader.
    EoReader& operator=(EoReader&& other) noexcept;

    ~EoReader() = default;

    /// Creates a new reader from a slice of this reader's data, starting at the current position.
    ///
    /// The new reader shares the underlying data but has independent position and chunked reading state.
    ///
    /// @return the new reader, covering the remaining data.
    EoReader Slice() const;

    /// Creates a new reader from a slice of this reader's data, starting at the specified index.
    ///
    /// The new reader shares the underlying data but has independent position and chunked reading state.
    ///
    /// @param index the index in this reader's data where the slice starts. Indexes past the end produce an empty
    ///              reader.
    /// @return the new reader, covering the data from <c>index</c> to the end.
    /// @throws std::invalid_argument if the index is negative.
    EoReader Slice(int index) const;

    /// Creates a new reader from a slice of this reader's data, with the specified index and length.
    ///
    /// The new reader shares the underlying data but has independent position and chunked reading state.
    ///
    /// @param index the index in this reader's data where the slice starts.
    /// @param length the length of the slice. The slice is truncated if it extends past the end of the data.
    /// @return the new reader, covering the specified range.
    /// @throws std::invalid_argument if the index or length is negative.
    EoReader Slice(int index, int length) const;

    /// Reads a raw byte from the input data.
    ///
    /// @return the byte value, or 0 if there is no data remaining.
    int GetByte();

    /// Reads an array of raw bytes from the input data.
    ///
    /// @param length the number of bytes to read.
    /// @return the bytes read, which may be fewer than <c>length</c> if there isn't enough data remaining.
    std::vector<std::uint8_t> GetBytes(int length);

    /// Reads an encoded 1-byte integer from the input data.
    ///
    /// @return the decoded value. If fewer bytes remain, it is decoded from the bytes that are available (0 if there
    ///         are none).
    int GetChar();

    /// Reads an encoded 2-byte integer from the input data.
    ///
    /// @return the decoded value. If fewer bytes remain, it is decoded from the bytes that are available (0 if there
    ///         are none).
    int GetShort();

    /// Reads an encoded 3-byte integer from the input data.
    ///
    /// @return the decoded value. If fewer bytes remain, it is decoded from the bytes that are available (0 if there
    ///         are none).
    int GetThree();

    /// Reads an encoded 4-byte integer from the input data.
    ///
    /// @return the decoded value. If fewer bytes remain, it is decoded from the bytes that are available (0 if there
    ///         are none).
    int GetInt();

    /// Reads a string from the input data.
    ///
    /// @return all remaining data, or the rest of the current chunk in chunked reading mode, as a string.
    std::string GetString();

    /// Reads a fixed-length string from the input data.
    ///
    /// @param length the length of the string.
    /// @param padded true if the string is padded with trailing <c>0xFF</c> bytes, which are removed.
    /// @return the string, which may be shorter than <c>length</c> if there isn't enough data remaining.
    /// @throws std::invalid_argument if the length is negative.
    std::string GetFixedString(int length, bool padded = false);

    /// Reads an encoded string from the input data.
    ///
    /// @return all remaining data, or the rest of the current chunk in chunked reading mode, as a decoded string.
    std::string GetEncodedString();

    /// Reads a fixed-length encoded string from the input data.
    ///
    /// @param length the length of the string.
    /// @param padded true if the string is padded with trailing <c>0xFF</c> bytes, which are removed.
    /// @return the decoded string, which may be shorter than <c>length</c> if there isn't enough data remaining.
    /// @throws std::invalid_argument if the length is negative.
    std::string GetFixedEncodedString(int length, bool padded = false);

    /// Gets the chunked reading mode for the reader.
    ///
    /// @return true if the reader is in chunked reading mode.
    bool GetChunkedReadingMode() const noexcept;

    /// Sets the chunked reading mode for the reader.
    ///
    /// In chunked reading mode, the reader will treat <c>0xFF</c> bytes as the boundaries between chunks of data.
    ///
    /// @param chunked_reading_mode true to enable chunked reading mode.
    void SetChunkedReadingMode(bool chunked_reading_mode) noexcept;

    /// Moves the reader position to the start of the next chunk.
    ///
    /// @throws std::logic_error if not in chunked reading mode.
    void NextChunk();

    /// Gets the number of bytes remaining in the input data.
    ///
    /// @return the number of bytes remaining, or the number remaining in the current chunk in chunked reading mode.
    int Remaining() const noexcept;

    /// Gets the current position in the input data.
    ///
    /// @return the number of bytes read from the start of the input data.
    int Position() const noexcept;

private:
    EoReader(std::shared_ptr<const void> owner, const std::uint8_t* data, int limit) noexcept;

    /// Keeps owned data alive; null for views.
    std::shared_ptr<const void> owner_;
    const std::uint8_t* data_ = nullptr;
    int limit_ = 0;
    int position_ = 0;
    bool chunked_reading_mode_ = false;
    int chunk_start_ = 0;
    int next_break_ = -1;

    std::uint8_t ReadByte();
    std::vector<std::uint8_t> ReadBytes(int length);
    int FindNextBreakIndex() const noexcept;
    void Reset() noexcept;
};

} // namespace eolib::data
