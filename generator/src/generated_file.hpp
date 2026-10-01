#pragma once

#include "code_writer.hpp"

#include <filesystem>
#include <string>

namespace eolib::generator
{

/// A file produced by the generator.
struct OutputFile
{
    /// Path relative to the output root, e.g. "include/eolib/protocol/net/enums.hpp".
    std::string path;
    std::string content;

    /// Writes the content to the path under the output root, unless the file already has the same content. Skipping
    /// unchanged files avoids rebuilding the code that depends on them.
    ///
    /// @return true if the file was written, or false if it was unchanged.
    /// @throws std::runtime_error if the file can't be written.
    bool WriteIfChanged(const std::filesystem::path& output_root) const;
};

/// The code for a generated file. Starts with a banner that marks the file as generated, and for header files, the
/// include guard.
class GeneratedFile : public CodeWriter
{
public:
    enum class Kind
    {
        Header,
        Source,
    };

    /// Starts a generated file at the specified path, relative to the output root.
    GeneratedFile(std::string path, Kind kind);

    /// Gets the output file with the code written so far.
    OutputFile ToOutput() const;

private:
    std::string path_;
};

} // namespace eolib::generator
