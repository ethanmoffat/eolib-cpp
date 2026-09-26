#include "eolib/data/eo_reader.hpp"

#include "eolib/data/number_encoder.hpp"
#include "eolib/data/string_encoder.hpp"

#include <algorithm>
#include <stdexcept>

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
    StringEncoder::DecodeInPlace(bytes.data(), bytes.size());
    return ToString(bytes);
}

std::string EoReader::GetFixedEncodedString(int length, bool padded)
{
    CheckLength(length);
    auto bytes = ReadBytes(length);
    StringEncoder::DecodeInPlace(bytes.data(), bytes.size());
    if (padded)
    {
        RemovePadding(bytes);
    }
    return ToString(bytes);
}

bool EoReader::GetChunkedReadingMode() const
{
    return chunked_reading_mode_;
}

void EoReader::SetChunkedReadingMode(bool chunked_reading_mode)
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

int EoReader::Remaining() const
{
    if (chunked_reading_mode_)
    {
        return next_break_ - std::min(position_, next_break_);
    }
    return limit_ - position_;
}

int EoReader::Position() const
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
    const auto begin = data_->begin() + offset_ + position_;
    std::vector<std::uint8_t> result(begin, begin + length);
    position_ += length;
    return result;
}

int EoReader::FindNextBreakIndex() const
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
