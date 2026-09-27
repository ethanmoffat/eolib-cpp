#include "protocol/captured_packet_properties.hpp"

#include <stdexcept>

namespace eolib::test
{

std::vector<std::uint8_t> DecodeBase64(const std::string& input)
{
    static const std::string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<std::uint8_t> result;
    int buffer = 0;
    int bits = 0;
    for (const char c : input)
    {
        if (c == '=')
        {
            break;
        }
        const auto index = ALPHABET.find(c);
        if (index == std::string::npos)
        {
            continue;
        }
        buffer = ((buffer << 6) | static_cast<int>(index)) & 0xFFFFFF;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            result.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFF));
        }
    }
    return result;
}

namespace detail
{

namespace
{

std::string Describe(const Json& property)
{
    return property.contains("name") ? "\"" + property["name"].get<std::string>() + "\"" : property.dump();
}

const Json& Value(const Json& property)
{
    const auto it = property.find("value");
    if (it == property.end() || it->is_null())
    {
        throw std::invalid_argument("Property " + Describe(property) + " has no value.");
    }
    return *it;
}

/// Converts UTF-8 to single-byte characters. The captured strings are all within the Latin-1 range, where Unicode
/// code points and Windows-1252 bytes match (except 0x80-0x9F, which the captures don't use).
std::string ToSingleByte(const std::string& utf8)
{
    std::string result;
    for (std::size_t i = 0; i < utf8.size(); ++i)
    {
        const auto byte = static_cast<unsigned char>(utf8[i]);
        if (byte < 0x80)
        {
            result += static_cast<char>(byte);
        }
        else if ((byte & 0xE0) == 0xC0 && i + 1 < utf8.size() && byte <= 0xC3)
        {
            const auto next = static_cast<unsigned char>(utf8[++i]);
            result += static_cast<char>(((byte & 0x1F) << 6) | (next & 0x3F));
        }
        else
        {
            throw std::invalid_argument("String contains characters outside of the single-byte range: " + utf8);
        }
    }
    return result;
}

} // namespace

const std::string& Name(const Json& property)
{
    return property.at("name").get_ref<const std::string&>();
}

std::string TypeName(const Json& property)
{
    const auto& type = property.at("type").get_ref<const std::string&>();
    const auto separator = type.rfind("::");
    return separator == std::string::npos ? type : type.substr(separator + 2);
}

const Json& Children(const Json& property)
{
    static const Json EMPTY = Json::array();
    const auto it = property.find("children");
    return it == property.end() || it->is_null() ? EMPTY : *it;
}

bool HasValue(const Json& property)
{
    const auto value = property.find("value");
    const auto children = property.find("children");
    return (value != property.end() && !value->is_null()) || (children != property.end() && !children->is_null());
}

int IntValue(const Json& property)
{
    // Values of EO int fields can exceed INT_MAX. They wrap, the same as when they are deserialized.
    return static_cast<int>(static_cast<std::uint32_t>(Value(property).get<long long>()));
}

bool BoolValue(const Json& property)
{
    return Value(property).get<bool>();
}

std::string StringValue(const Json& property)
{
    return ToSingleByte(Value(property).get<std::string>());
}

std::vector<std::uint8_t> BlobValue(const Json& property)
{
    return DecodeBase64(Value(property).get<std::string>());
}

void UnknownProperty(const Json& property, const std::string& owner)
{
    throw std::invalid_argument("Unknown property " + Describe(property) + " for " + owner + ".");
}

void UnknownType(const Json& property, const std::string& owner)
{
    throw std::invalid_argument("Unknown type \"" + property.value("type", std::string()) + "\" for property " +
                                Describe(property) + " of " + owner + ".");
}

} // namespace detail

} // namespace eolib::test
