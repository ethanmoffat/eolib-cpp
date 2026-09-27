#include "eolib/data/eo_writer.hpp"

#include "eolib/data/eo_numeric_limits.hpp"
#include "eolib/data/number_encoder.hpp"
#include "eolib/data/string_encoder.hpp"

#include <stdexcept>
#include <utility>

namespace eolib::data
{

namespace
{

void CheckNumberSize(int number, unsigned int max)
{
    if (static_cast<unsigned int>(number) > max)
    {
        throw std::invalid_argument("Value " + std::to_string(number) + " exceeds maximum of " + std::to_string(max) +
                                    ".");
    }
}

void CheckStringLength(std::string_view str, int length, bool padded)
{
    if (length < 0)
    {
        throw std::invalid_argument("Length must not be negative.");
    }

    const auto expected = static_cast<std::size_t>(length);
    if (padded)
    {
        if (expected >= str.size())
        {
            return;
        }
        throw std::invalid_argument("Padded string \"" + std::string(str) + "\" is too large for a length of " +
                                    std::to_string(length) + ".");
    }

    if (str.size() != expected)
    {
        throw std::invalid_argument("String \"" + std::string(str) + "\" does not have expected length of " +
                                    std::to_string(length) + ".");
    }
}

void AddPadding(std::vector<std::uint8_t>& bytes, int length)
{
    bytes.resize(static_cast<std::size_t>(length), 0xFF);
}

} // namespace

EoWriter::EoWriter(std::size_t capacity)
{
    data_.reserve(capacity);
}

void EoWriter::AddByte(int value)
{
    CheckNumberSize(value, 0xFF);
    data_.push_back(static_cast<std::uint8_t>(value));
}

void EoWriter::AddBytes(const std::vector<std::uint8_t>& bytes)
{
    data_.insert(data_.end(), bytes.begin(), bytes.end());
}

void EoWriter::AddBytes(const std::uint8_t* bytes, std::size_t length)
{
    data_.insert(data_.end(), bytes, bytes + length);
}

void EoWriter::AddChar(int number)
{
    AddEncodedNumber(number, EoNumericLimits::CharMax - 1, 1);
}

void EoWriter::AddShort(int number)
{
    AddEncodedNumber(number, EoNumericLimits::ShortMax - 1, 2);
}

void EoWriter::AddThree(int number)
{
    AddEncodedNumber(number, EoNumericLimits::ThreeMax - 1, 3);
}

void EoWriter::AddInt(int number)
{
    AddEncodedNumber(number, EoNumericLimits::IntMax - 1, 4);
}

void EoWriter::AddString(std::string_view str)
{
    AddBytes(PrepareString(str));
}

void EoWriter::AddFixedString(std::string_view str, int length, bool padded)
{
    CheckStringLength(str, length, padded);
    auto bytes = PrepareString(str);
    if (padded)
    {
        AddPadding(bytes, length);
    }
    AddBytes(bytes);
}

void EoWriter::AddEncodedString(std::string_view str)
{
    auto bytes = PrepareString(str);
    StringEncoder::EncodeString(bytes);
    AddBytes(bytes);
}

void EoWriter::AddFixedEncodedString(std::string_view str, int length, bool padded)
{
    CheckStringLength(str, length, padded);
    auto bytes = PrepareString(str);
    if (padded)
    {
        AddPadding(bytes, length);
    }
    StringEncoder::EncodeString(bytes);
    AddBytes(bytes);
}

bool EoWriter::GetStringSanitization() const noexcept
{
    return string_sanitization_;
}

void EoWriter::SetStringSanitization(bool string_sanitization) noexcept
{
    string_sanitization_ = string_sanitization;
}

int EoWriter::Length() const noexcept
{
    return static_cast<int>(data_.size());
}

std::vector<std::uint8_t> EoWriter::ToByteArray() const&
{
    return data_;
}

std::vector<std::uint8_t> EoWriter::ToByteArray() &&
{
    std::vector<std::uint8_t> result = std::move(data_);
    data_.clear();
    return result;
}

std::string EoWriter::ToByteString() const
{
    return std::string(data_.begin(), data_.end());
}

const std::vector<std::uint8_t>& EoWriter::Data() const noexcept
{
    return data_;
}

void EoWriter::AddEncodedNumber(int number, unsigned int max, std::size_t size)
{
    CheckNumberSize(number, max);
    const auto bytes = NumberEncoder::EncodeNumber(number);
    data_.insert(data_.end(), bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(size));
}

std::vector<std::uint8_t> EoWriter::PrepareString(std::string_view str) const
{
    std::vector<std::uint8_t> bytes(str.begin(), str.end());
    if (string_sanitization_)
    {
        for (auto& b : bytes)
        {
            if (b == 0xFF)
            {
                b = 0x79;
            }
        }
    }
    return bytes;
}

} // namespace eolib::data
