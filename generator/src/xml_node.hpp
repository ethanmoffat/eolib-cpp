#pragma once

#include <optional>
#include <string>

namespace pugi
{
class xml_node;
}

namespace eolib::generator
{

/// Helpers for reading and writing the content of pugixml nodes.
class XmlNode
{
public:
    XmlNode() = delete;

    /// Gets the text content of an element, excluding the content of child elements.
    static std::string Text(const pugi::xml_node& node);

    /// Gets the text content of an element with whitespace trimmed, or nullopt if it's empty.
    static std::optional<std::string> TrimmedText(const pugi::xml_node& node);

    /// Replaces all children of an element with the specified text.
    static void SetText(pugi::xml_node node, const std::string& text);

    /// Gets the value of an attribute, or nullopt if the element doesn't have it.
    static std::optional<std::string> OptionalAttribute(const pugi::xml_node& node, const char* name);

    /// Gets the value of an attribute.
    /// @throws GeneratorError if the element doesn't have the attribute.
    static std::string RequiredAttribute(const pugi::xml_node& node, const char* name, const std::string& context);

    /// Gets the value of a "true" or "false" attribute, or default_value if the element doesn't have it.
    /// @throws GeneratorError if the attribute has any other value.
    static bool BoolAttribute(const pugi::xml_node& node, const char* name, bool default_value,
                              const std::string& context);
};

} // namespace eolib::generator
