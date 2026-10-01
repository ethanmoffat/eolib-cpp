#include "xml_node.hpp"

#include "errors.hpp"
#include "names.hpp"

#include <pugixml.hpp>

namespace eolib::generator
{

std::string XmlNode::Text(const pugi::xml_node& node)
{
    std::string text;
    for (const auto& child : node.children())
    {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata)
        {
            text += child.value();
        }
    }
    return text;
}

std::optional<std::string> XmlNode::TrimmedText(const pugi::xml_node& node)
{
    auto text = Trim(Text(node));
    if (text.empty())
    {
        return std::nullopt;
    }
    return text;
}

void XmlNode::SetText(pugi::xml_node node, const std::string& text)
{
    while (node.first_child())
    {
        node.remove_child(node.first_child());
    }
    node.append_child(pugi::node_pcdata).set_value(text.c_str());
}

std::optional<std::string> XmlNode::OptionalAttribute(const pugi::xml_node& node, const char* name)
{
    const auto attribute = node.attribute(name);
    if (!attribute)
    {
        return std::nullopt;
    }
    return std::string(attribute.value());
}

std::string XmlNode::RequiredAttribute(const pugi::xml_node& node, const char* name, const std::string& context)
{
    const auto attribute = node.attribute(name);
    if (!attribute)
    {
        throw GeneratorError(context + ": <" + node.name() + "> is missing required attribute \"" + name + "\".");
    }
    return attribute.value();
}

bool XmlNode::BoolAttribute(const pugi::xml_node& node, const char* name, bool default_value,
                            const std::string& context)
{
    const auto attribute = node.attribute(name);
    if (!attribute)
    {
        return default_value;
    }

    const std::string value = attribute.value();
    if (value == "true")
    {
        return true;
    }
    if (value == "false")
    {
        return false;
    }
    throw GeneratorError(context + ": attribute \"" + name + "\" must be \"true\" or \"false\" (got \"" + value +
                         "\").");
}

} // namespace eolib::generator
