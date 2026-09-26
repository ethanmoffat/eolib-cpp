#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace eolib::generator
{

/// Accumulates lines of code with indentation (4 spaces per level) and Allman-style braces.
class CodeWriter
{
public:
    /// Adds a line at the current indentation level. An empty string adds a blank line.
    void Line(std::string_view text = {})
    {
        lines_.emplace_back(text.empty() ? 0 : indent_, std::string(text));
    }

    /// Adds a line followed by an opening brace, and increases the indentation level.
    void Open(std::string_view text)
    {
        Line(text);
        Line("{");
        ++indent_;
    }

    /// Decreases the indentation level and adds a closing brace, optionally followed by a suffix (e.g. ";").
    void Close(std::string_view suffix = {})
    {
        --indent_;
        Line("}" + std::string(suffix));
    }

    void Indent()
    {
        ++indent_;
    }

    void Dedent()
    {
        --indent_;
    }

    /// Adds Doxygen comment lines for the specified (possibly multi-line) text.
    void DocComment(std::string_view text)
    {
        std::size_t start = 0;
        while (start <= text.size())
        {
            const auto end = text.find('\n', start);
            const auto line = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
            Line(line.empty() ? std::string("///") : "/// " + std::string(line));
            if (end == std::string_view::npos)
            {
                break;
            }
            start = end + 1;
        }
    }

    /// Appends all lines from another writer, relative to the current indentation level.
    void Append(const CodeWriter& other)
    {
        for (const auto& [indent, text] : other.lines_)
        {
            lines_.emplace_back(text.empty() ? 0 : indent + indent_, text);
        }
    }

    bool Empty() const
    {
        return lines_.empty();
    }

    /// Removes a trailing blank line, if present.
    void TrimTrailingBlankLine()
    {
        while (!lines_.empty() && lines_.back().second.empty())
        {
            lines_.pop_back();
        }
    }

    std::string ToString() const
    {
        std::string result;
        for (const auto& [indent, text] : lines_)
        {
            if (!text.empty())
            {
                result.append(static_cast<std::size_t>(indent) * 4, ' ');
                result += text;
            }
            result += '\n';
        }
        return result;
    }

private:
    std::vector<std::pair<int, std::string>> lines_;
    int indent_ = 0;
};

} // namespace eolib::generator
