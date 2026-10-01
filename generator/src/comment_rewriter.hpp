#pragma once

#include <string>
#include <vector>

namespace pugi
{
class xml_node;
}

namespace eolib::generator
{

/// Turns XML comments in a protocol file into <comment> elements, so they can be read the same way as the
/// documentation that the protocol files define with <comment> elements.
class CommentRewriter
{
public:
    CommentRewriter() = delete;

    /// Removes XML comments from the children of the specified element (recursively) and attaches them as <comment>
    /// elements. A comment attaches to the next sibling element. A comment with no following sibling element attaches
    /// to its parent. Existing <comment> text is kept first, followed by trailing comments, then preceding comments.
    ///
    /// All comment text is normalized: each comment is joined into a single line, and multiple comments on an element
    /// are separated by newlines.
    static void RewriteInPlace(pugi::xml_node element);

private:
    /// Trims each line of a comment and joins the non-empty lines with a space. Comment lines are wrapped prose, so a
    /// sentence spanning multiple lines is kept together.
    static std::string NormalizeComment(const std::string& comment);

    static void AppendComments(pugi::xml_node element, const std::vector<std::string>& comments);
};

} // namespace eolib::generator
