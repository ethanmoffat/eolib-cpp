#pragma once

// Implementation details shared by the generated protocol code. Not part of the public API.

#include "eolib/data/eo_reader.hpp"
#include "eolib/data/eo_writer.hpp"
#include "eolib/protocol/serializable.hpp"

#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace eolib::protocol::detail
{

/// Restores the chunked reading mode of a reader when destroyed.
class ChunkedReadingModeGuard
{
public:
    explicit ChunkedReadingModeGuard(data::EoReader& reader)
        : reader_(reader),
          old_mode_(reader.GetChunkedReadingMode())
    {
    }

    ~ChunkedReadingModeGuard()
    {
        reader_.SetChunkedReadingMode(old_mode_);
    }

    ChunkedReadingModeGuard(const ChunkedReadingModeGuard&) = delete;
    ChunkedReadingModeGuard& operator=(const ChunkedReadingModeGuard&) = delete;

private:
    data::EoReader& reader_;
    bool old_mode_;
};

/// Restores the string sanitization mode of a writer when destroyed.
class StringSanitizationGuard
{
public:
    explicit StringSanitizationGuard(data::EoWriter& writer)
        : writer_(writer),
          old_mode_(writer.GetStringSanitization())
    {
    }

    ~StringSanitizationGuard()
    {
        writer_.SetStringSanitization(old_mode_);
    }

    StringSanitizationGuard(const StringSanitizationGuard&) = delete;
    StringSanitizationGuard& operator=(const StringSanitizationGuard&) = delete;

private:
    data::EoWriter& writer_;
    bool old_mode_;
};

inline std::string FormatValue(const std::string& value)
{
    return "\"" + value + "\"";
}

inline std::string FormatValue(bool value)
{
    return value ? "true" : "false";
}

inline std::string FormatValue(const std::monostate&)
{
    return "null";
}

template <typename T>
std::string FormatValue(const T& value);

template <typename T>
std::string FormatValue(const std::vector<T>& values);

template <typename T>
std::string FormatValue(const std::optional<T>& value);

template <typename... Ts>
std::string FormatValue(const std::variant<Ts...>& value);

template <typename T>
std::string FormatValue(const T& value)
{
    if constexpr (std::is_enum_v<T>)
    {
        // Found via argument-dependent lookup in the enum's namespace.
        return ToString(value);
    }
    else if constexpr (std::is_base_of_v<Serializable, T>)
    {
        return value.ToString();
    }
    else
    {
        static_assert(std::is_integral_v<T>, "Unsupported type for FormatValue");
        return std::to_string(value);
    }
}

template <typename T>
std::string FormatValue(const std::vector<T>& values)
{
    std::string result = "[";
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            result += ", ";
        }
        result += FormatValue(values[i]);
    }
    result += "]";
    return result;
}

template <typename T>
std::string FormatValue(const std::optional<T>& value)
{
    return value.has_value() ? FormatValue(*value) : "null";
}

template <typename... Ts>
std::string FormatValue(const std::variant<Ts...>& value)
{
    return std::visit([](const auto& alternative) { return FormatValue(alternative); }, value);
}

} // namespace eolib::protocol::detail
