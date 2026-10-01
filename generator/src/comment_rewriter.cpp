#include "comment_rewriter.hpp"

#include "names.hpp"
#include "xml_node.hpp"

#include <pugixml.hpp>

#include <sstream>

namespace eolib::generator
{

void CommentRewriter::RewriteInPlace(pugi::xml_node element)
{
    std::vector<std::string> pending_comments;
    for (auto child = element.first_child(); child;)
    {
        const auto next = child.next_sibling();
        if (child.type() == pugi::node_comment)
        {
            pending_comments.emplace_back(child.value());
            element.remove_child(child);
        }
        else if (child.type() == pugi::node_element)
        {
            if (std::string(child.name()) == "comment")
            {
                XmlNode::SetText(child, NormalizeComment(XmlNode::Text(child)));
            }
            else
            {
                RewriteInPlace(child);
                AppendComments(child, pending_comments);
                pending_comments.clear();
            }
        }
        child = next;
    }

    AppendComments(element, pending_comments);
}

std::string CommentRewriter::NormalizeComment(const std::string& comment)
{
    std::istringstream stream(comment);
    std::string result;
    std::string line;
    while (std::getline(stream, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }
        if (!result.empty())
        {
            result += ' ';
        }
        result += line;
    }
    return result;
}

void CommentRewriter::AppendComments(pugi::xml_node element, const std::vector<std::string>& comments)
{
    std::vector<std::string> normalized;
    for (const auto& comment : comments)
    {
        auto text = NormalizeComment(comment);
        if (!text.empty())
        {
            normalized.push_back(std::move(text));
        }
    }

    if (normalized.empty())
    {
        return;
    }

    auto comment_element = element.child("comment");
    if (!comment_element)
    {
        comment_element = element.prepend_child("comment");
    }
    else if (auto existing = XmlNode::Text(comment_element); !existing.empty())
    {
        normalized.insert(normalized.begin(), std::move(existing));
    }

    XmlNode::SetText(comment_element, Join(normalized, "\n"));
}

} // namespace eolib::generator
