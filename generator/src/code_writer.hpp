#pragma once

#include <set>
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
    void Line(std::string_view text = {});

    /// Adds a line followed by an opening brace, and increases the indentation level.
    void Open(std::string_view text);

    /// Decreases the indentation level and adds a closing brace, optionally followed by a suffix (e.g. ";").
    void Close(std::string_view suffix = {});

    void Indent();

    void Dedent();

    /// Adds Doxygen comment lines for the specified (possibly multi-line) text.
    void DocComment(std::string_view text);

    /// Adds #include lines for local headers, then system headers, followed by a blank line. A blank line separates
    /// the two groups.
    void Includes(const std::set<std::string>& local, const std::set<std::string>& system);

    /// Opens a namespace block, followed by a blank line.
    void BeginNamespace(const std::string& name);

    /// Closes a namespace block opened with BeginNamespace, replacing any trailing blank lines with a single one.
    void EndNamespace(const std::string& name);

    /// Appends all lines from another writer, relative to the current indentation level.
    void Append(const CodeWriter& other);

    bool Empty() const;

    /// Removes trailing blank lines, if present.
    void TrimTrailingBlankLine();

    std::string ToString() const;

private:
    std::vector<std::pair<int, std::string>> lines_;
    int indent_ = 0;
};

} // namespace eolib::generator
