#pragma once

#include "model.hpp"
#include "types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace eolib::generator
{

/// Builds the text of a generated Doxygen comment: a list of paragraphs, followed by a "@note" list if any notes were
/// added. All text is escaped with DocText when it's added, so it's shown as written.
class DocComment
{
public:
    /// Adds a paragraph, if text has a value.
    void AddParagraph(const std::optional<std::string>& text);

    /// Adds an item to the note list.
    void AddNote(const std::string& note);

    /// Adds a note for the comment on each instruction that has no public member (dummy, length, break and chunked
    /// instructions), describing where the instruction is relative to the public members. Comments on unnamed fields
    /// are skipped, since they document hardcoded values that aren't visible to consumers.
    void AddInstructionNotes(const std::vector<Instruction>& instructions);

    /// Adds a paragraph for each distinct comment on the empty cases of a switch, e.g. "When `field` is 0, 4 or 9:
    /// text".
    void AddEmptyCaseParagraphs(const Instruction& switch_instruction, const std::string& field_identifier);

    /// Adds notes for the length and value range constraints of a field or array member.
    ///
    /// If the length of the member comes from a length field, length_field_max is the largest length that field can
    /// hold (its maximum value plus its offset).
    void AddConstraintNotes(const Instruction& instruction, const Type& type, bool array,
                            std::optional<long long> length_field_max);

    /// Checks whether no paragraphs or notes were added.
    bool Empty() const;

    /// Gets the comment text, with paragraphs separated by blank lines.
    std::string Text() const;

private:
    std::vector<std::string> paragraphs_;
    std::vector<std::string> notes_;
};

} // namespace eolib::generator
