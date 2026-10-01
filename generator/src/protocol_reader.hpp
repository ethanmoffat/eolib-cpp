#pragma once

#include "model.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pugi
{
class xml_node;
}

namespace eolib::generator
{

/// Reads protocol.xml files into the protocol model.
class ProtocolReader
{
public:
    /// Reads all protocol.xml files found (recursively) under the specified root directory, sorted by relative path.
    /// @throws GeneratorError if the root is not a directory, or any file is invalid.
    static std::vector<ProtocolFile> ReadAll(const std::filesystem::path& xml_root);

    /// Reads a single protocol.xml file. relative_dir is the directory of the file relative to the XML root, using '/'
    /// separators, and determines the namespace of the generated code.
    /// @throws GeneratorError if the file is invalid.
    static ProtocolFile ReadFile(const std::filesystem::path& path, const std::string& relative_dir);

private:
    /// The path of the file being read, used as the context of error messages.
    std::string path_;

    explicit ProtocolReader(std::string path);

    ProtocolEnum ReadEnum(const pugi::xml_node& node) const;
    ProtocolStruct ReadStruct(const pugi::xml_node& node) const;
    ProtocolPacket ReadPacket(const pugi::xml_node& node) const;
    std::vector<Instruction> ReadInstructions(const pugi::xml_node& parent, const std::string& context) const;
    Instruction ReadInstruction(const pugi::xml_node& node, const std::string& context) const;
    ProtocolCase ReadCase(const pugi::xml_node& node, const std::string& context) const;

    /// Reads the <comment> child of an element, or nullopt if it has none. The comment text has already been
    /// normalized by CommentRewriter.
    static std::optional<std::string> ReadComment(const pugi::xml_node& node);

    static int ParseInt(const std::string& value, const std::string& context);
};

} // namespace eolib::generator
