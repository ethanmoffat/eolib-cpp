#include "model.hpp"

#include "errors.hpp"

namespace eolib::generator
{

std::string ProtocolFile::Namespace() const
{
    std::string result = "eolib::protocol";
    for (const auto& part : namespace_parts)
    {
        result += "::" + part;
    }
    return result;
}

std::string ProtocolFile::IncludeDir() const
{
    std::string result = "eolib/protocol";
    for (const auto& part : namespace_parts)
    {
        result += "/" + part;
    }
    return result;
}

std::string ProtocolFile::IncludePath(const std::string& file_name) const
{
    return IncludeDir() + "/" + file_name;
}

std::string ProtocolFile::PacketSide() const
{
    const std::string last = namespace_parts.empty() ? "" : namespace_parts.back();
    if (last == "client" || last == "server")
    {
        return last;
    }
    throw GeneratorError(path.string() + ": packets are only allowed in client or server protocol files.");
}

std::vector<const Instruction*> FlattenChunked(const std::vector<Instruction>& instructions)
{
    std::vector<const Instruction*> result;
    for (const auto& instruction : instructions)
    {
        result.push_back(&instruction);
        if (instruction.kind == InstructionKind::Chunked)
        {
            const auto chunked = FlattenChunked(instruction.instructions);
            result.insert(result.end(), chunked.begin(), chunked.end());
        }
    }
    return result;
}

std::vector<const Instruction*> FlattenAll(const std::vector<Instruction>& instructions)
{
    std::vector<const Instruction*> result;
    for (const auto* instruction : FlattenChunked(instructions))
    {
        result.push_back(instruction);
        if (instruction->kind != InstructionKind::Switch)
        {
            continue;
        }
        for (const auto& protocol_case : instruction->cases)
        {
            const auto case_instructions = FlattenAll(protocol_case.instructions);
            result.insert(result.end(), case_instructions.begin(), case_instructions.end());
        }
    }
    return result;
}

} // namespace eolib::generator
