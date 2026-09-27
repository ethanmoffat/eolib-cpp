#include "eolib/data/eo_reader.hpp"

#include "eolib/data/number_encoder.hpp"
#include "eolib/data/string_encoder.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace eolib::data
{

namespace
{

void RemovePadding(std::vector<std::uint8_t>& bytes)
{
    const auto it = std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0xFF));
    bytes.erase(it, bytes.end());
}

std::string ToString(const std::vector<std::uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

void CheckLength(int length)
{
    if (length < 0)
    {
        throw std::invalid_argument("Length must not be negative.");
    }
}

} // namespace

EoReader::EoReader(std::vector<std::uint8_t> data)
    : data_(std::make_shared<const std::vector<std::uint8_t>>(std::move(data))),
      offset_(0),
      limit_(static_cast<int>(data_->size()))
{
}

EoReader::EoReader(std::string_view data)
    : EoReader(std::vector<std::uint8_t>(data.begin(), data.end()))
{
}

EoReader::EoReader(const std::uint8_t* data, std::size_t length)
    : EoReader(std::vector<std::uint8_t>(data, data + length))
{
}

EoReader::EoReader(EoReader&& other) noexcept
    : data_(std::move(other.data_)),
      offset_(other.offset_),
      limit_(other.limit_),
      position_(other.position_),
      chunked_reading_mode_(other.chunked_reading_mode_),
      chunk_start_(other.chunk_start_),
      next_break_(other.next_break_)
{
    other.Reset();
}

EoReader& EoReader::operator=(EoReader&& other) noexcept
{
    if (this != &other)
    {
        data_ = std::move(other.data_);
        offset_ = other.offset_;
        limit_ = other.limit_;
        position_ = other.position_;
        chunked_reading_mode_ = other.chunked_reading_mode_;
        chunk_start_ = other.chunk_start_;
        next_break_ = other.next_break_;
        other.Reset();
    }
    return *this;
}

EoReader::EoReader(std::shared_ptr<const std::vector<std::uint8_t>> data, int offset, int limit)
    : data_(std::move(data)),
      offset_(offset),
      limit_(limit)
{
}

EoReader EoReader::Slice() const
{
    return Slice(position_);
}

EoReader EoReader::Slice(int index) const
{
    return Slice(index, std::max(0, limit_ - index));
}

EoReader EoReader::Slice(int index, int length) const
{
    if (index < 0)
    {
        throw std::invalid_argument("Index must not be negative.");
    }

    if (length < 0)
    {
        throw std::invalid_argument("Length must not be negative.");
    }

    const int slice_offset = std::max(0, std::min(limit_, index));
    const int slice_limit = std::min(limit_ - slice_offset, length);
    return EoReader(data_, offset_ + slice_offset, slice_limit);
}

int EoReader::GetByte()
{
    return ReadByte();
}

std::vector<std::uint8_t> EoReader::GetBytes(int length)
{
    return ReadBytes(length);
}

int EoReader::GetChar()
{
    return NumberEncoder::DecodeNumber(ReadBytes(1));
}

int EoReader::GetShort()
{
    return NumberEncoder::DecodeNumber(ReadBytes(2));
}

int EoReader::GetThree()
{
    return NumberEncoder::DecodeNumber(ReadBytes(3));
}

int EoReader::GetInt()
{
    return NumberEncoder::DecodeNumber(ReadBytes(4));
}

std::string EoReader::GetString()
{
    return ToString(ReadBytes(Remaining()));
}

std::string EoReader::GetFixedString(int length, bool padded)
{
    CheckLength(length);
    auto bytes = ReadBytes(length);
    if (padded)
    {
        RemovePadding(bytes);
    }
    return ToString(bytes);
}

std::string EoReader::GetEncodedString()
{
    auto bytes = ReadBytes(Remaining());
    StringEncoder::DecodeString(bytes);
    return ToString(bytes);
}

std::string EoReader::GetFixedEncodedString(int length, bool padded)
{
    CheckLength(length);
    auto bytes = ReadBytes(length);
    StringEncoder::DecodeString(bytes);
    if (padded)
    {
        RemovePadding(bytes);
    }
    return ToString(bytes);
}

bool EoReader::GetChunkedReadingMode() const noexcept
{
    return chunked_reading_mode_;
}

void EoReader::SetChunkedReadingMode(bool chunked_reading_mode) noexcept
{
    chunked_reading_mode_ = chunked_reading_mode;
    if (next_break_ == -1)
    {
        next_break_ = FindNextBreakIndex();
    }
}

void EoReader::NextChunk()
{
    if (!chunked_reading_mode_)
    {
        throw std::logic_error("Not in chunked reading mode.");
    }

    position_ = next_break_;
    if (position_ < limit_)
    {
        // Skip the break byte
        ++position_;
    }

    chunk_start_ = position_;
    next_break_ = FindNextBreakIndex();
}

int EoReader::Remaining() const noexcept
{
    if (chunked_reading_mode_)
    {
        return next_break_ - std::min(position_, next_break_);
    }
    return limit_ - position_;
}

int EoReader::Position() const noexcept
{
    return position_;
}

std::uint8_t EoReader::ReadByte()
{
    if (Remaining() > 0)
    {
        const auto index = static_cast<std::size_t>(offset_) + static_cast<std::size_t>(position_++);
        return (*data_)[index];
    }
    return 0;
}

std::vector<std::uint8_t> EoReader::ReadBytes(int length)
{
    length = std::max(0, std::min(length, Remaining()));
    if (length == 0)
    {
        return {};
    }

    const auto begin = data_->begin() + offset_ + position_;
    std::vector<std::uint8_t> result(begin, begin + length);
    position_ += length;
    return result;
}

void EoReader::Reset() noexcept
{
    // A moved-from reader behaves like a reader over empty data. The data may only be null when the limit is 0.
    data_.reset();
    offset_ = 0;
    limit_ = 0;
    position_ = 0;
    chunked_reading_mode_ = false;
    chunk_start_ = 0;
    next_break_ = -1;
}

int EoReader::FindNextBreakIndex() const noexcept
{
    int i = chunk_start_;
    for (; i < limit_; ++i)
    {
        if ((*data_)[static_cast<std::size_t>(offset_) + static_cast<std::size_t>(i)] == 0xFF)
        {
            break;
        }
    }
    return i;
}

} // namespace eolib::data
