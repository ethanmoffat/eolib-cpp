#include "code_writer.hpp"

namespace eolib::generator
{

void CodeWriter::Line(std::string_view text)
{
    lines_.emplace_back(text.empty() ? 0 : indent_, std::string(text));
}

void CodeWriter::Open(std::string_view text)
{
    Line(text);
    Line("{");
    ++indent_;
}

void CodeWriter::Close(std::string_view suffix)
{
    --indent_;
    Line("}" + std::string(suffix));
}

void CodeWriter::Indent()
{
    ++indent_;
}

void CodeWriter::Dedent()
{
    --indent_;
}

void CodeWriter::DocComment(std::string_view text)
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

void CodeWriter::Includes(const std::set<std::string>& local, const std::set<std::string>& system)
{
    for (const auto& include : local)
    {
        Line("#include \"" + include + "\"");
    }
    if (!local.empty() && !system.empty())
    {
        Line();
    }
    for (const auto& include : system)
    {
        Line("#include <" + include + ">");
    }
    Line();
}

void CodeWriter::BeginNamespace(const std::string& name)
{
    Line("namespace " + name);
    Line("{");
    Line();
}

void CodeWriter::EndNamespace(const std::string& name)
{
    TrimTrailingBlankLine();
    Line();
    Line("} // namespace " + name);
}

void CodeWriter::Append(const CodeWriter& other)
{
    for (const auto& [indent, text] : other.lines_)
    {
        lines_.emplace_back(text.empty() ? 0 : indent + indent_, text);
    }
}

bool CodeWriter::Empty() const
{
    return lines_.empty();
}

void CodeWriter::TrimTrailingBlankLine()
{
    while (!lines_.empty() && lines_.back().second.empty())
    {
        lines_.pop_back();
    }
}

std::string CodeWriter::ToString() const
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

} // namespace eolib::generator
