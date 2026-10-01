#include "doc_comment.hpp"

#include "errors.hpp"
#include "names.hpp"

#include <algorithm>
#include <utility>

namespace eolib::generator
{

void DocComment::AddParagraph(const std::optional<std::string>& text)
{
    if (text)
    {
        paragraphs_.push_back(DocText(*text));
    }
}

void DocComment::AddNote(const std::string& note)
{
    notes_.push_back(DocText(note));
}

void DocComment::AddInstructionNotes(const std::vector<Instruction>& instructions)
{
    const auto flattened = FlattenChunked(instructions);

    for (std::size_t i = 0; i < flattened.size(); ++i)
    {
        const Instruction& instruction = *flattened[i];
        if (!instruction.comment || instruction.kind == InstructionKind::Field ||
            instruction.kind == InstructionKind::Switch || !InstructionMemberIdentifier(instruction).empty())
        {
            continue;
        }

        std::string comment = *instruction.comment;
        std::replace(comment.begin(), comment.end(), '\n', ' ');
        AddNote(DescribeInstruction(flattened, i) + ": " + comment);
    }
}

void DocComment::AddEmptyCaseParagraphs(const Instruction& switch_instruction, const std::string& field_identifier)
{
    std::vector<std::pair<std::string, std::vector<std::string>>> values_by_comment;
    for (const auto& protocol_case : switch_instruction.cases)
    {
        if (!protocol_case.instructions.empty() || !protocol_case.comment)
        {
            continue;
        }

        const std::string value = protocol_case.is_default ? "any other value" : *protocol_case.value;
        const auto it = std::find_if(values_by_comment.begin(), values_by_comment.end(),
                                     [&](const auto& x) { return x.first == *protocol_case.comment; });
        if (it == values_by_comment.end())
        {
            values_by_comment.emplace_back(*protocol_case.comment, std::vector<std::string>{value});
        }
        else
        {
            it->second.push_back(value);
        }
    }

    for (const auto& [comment, values] : values_by_comment)
    {
        std::string joined = values.front();
        for (std::size_t i = 1; i < values.size(); ++i)
        {
            joined += (i + 1 == values.size() ? " or " : ", ") + values[i];
        }
        AddParagraph("When `" + field_identifier + "` is " + joined + ": " + comment);
    }
}

void DocComment::AddConstraintNotes(const Instruction& instruction, const Type& type, bool array,
                                    std::optional<long long> length_field_max)
{
    if (instruction.length)
    {
        std::string description;
        if (length_field_max)
        {
            description = std::to_string(*length_field_max) + " or less";
        }
        else
        {
            description = "`" + *instruction.length + "`";
            if (instruction.padded)
            {
                description += " or less";
            }
        }
        AddNote(std::string(array ? "Size" : "Length") + " must be " + description + ".");
    }

    if (type.kind == TypeKind::Integer)
    {
        AddNote(std::string(array ? "Element value" : "Value") + " range is 0-" + std::to_string(MaxValueOf(type)) +
                ".");
    }
}

bool DocComment::Empty() const
{
    return paragraphs_.empty() && notes_.empty();
}

std::string DocComment::Text() const
{
    std::string result;
    for (const auto& paragraph : paragraphs_)
    {
        if (!result.empty())
        {
            result += "\n\n";
        }
        result += paragraph;
    }

    if (!notes_.empty())
    {
        if (!result.empty())
        {
            result += "\n\n";
        }
        result += "@note";
        for (const auto& note : notes_)
        {
            result += "\n- " + note;
        }
    }

    return result;
}

std::string DocComment::InstructionMemberIdentifier(const Instruction& instruction)
{
    switch (instruction.kind)
    {
        case InstructionKind::Field:
        case InstructionKind::Array:
            return instruction.name ? MemberIdentifier(*instruction.name) : std::string();
        case InstructionKind::Switch:
            return MemberIdentifier(SwitchDataName(instruction.switch_field));
        default:
            return {};
    }
}

std::string DocComment::DescribeInstructionPosition(const std::vector<const Instruction*>& instructions,
                                                    std::size_t index)
{
    for (std::size_t i = index; i > 0; --i)
    {
        const auto identifier = InstructionMemberIdentifier(*instructions[i - 1]);
        if (!identifier.empty())
        {
            return "after `" + identifier + "`";
        }
    }

    for (std::size_t i = index + 1; i < instructions.size(); ++i)
    {
        const auto identifier = InstructionMemberIdentifier(*instructions[i]);
        if (!identifier.empty())
        {
            return "before `" + identifier + "`";
        }
    }

    return {};
}

std::string DocComment::DescribeInstruction(const std::vector<const Instruction*>& instructions, std::size_t index)
{
    const Instruction& instruction = *instructions[index];

    std::string subject;
    switch (instruction.kind)
    {
        case InstructionKind::Dummy:
            subject = "The dummy " + instruction.type;
            break;
        case InstructionKind::Length:
            return "The `" + *instruction.name + "` length field";
        case InstructionKind::Break:
            subject = "The break byte";
            break;
        case InstructionKind::Chunked:
            subject = "The chunked section";
            break;
        default:
            throw GeneratorError("Unhandled instruction kind for description");
    }

    const auto position = DescribeInstructionPosition(instructions, index);
    if (!position.empty())
    {
        subject += " " + position;
    }

    if (instruction.kind == InstructionKind::Dummy)
    {
        const bool string = instruction.type == "string" || instruction.type == "encoded_string";
        subject += " (always " + (string ? "\"" + *instruction.value + "\"" : *instruction.value) + ")";
    }

    return subject;
}

} // namespace eolib::generator
