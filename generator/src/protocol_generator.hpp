#pragma once

#include "generated_file.hpp"
#include "model.hpp"
#include "object_generator.hpp"
#include "types.hpp"

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace eolib::generator
{

/// Describes which generated files exist for a protocol file.
struct FileLayout
{
    bool has_enums = false;
    bool has_structs = false;
    bool has_packets = false;

    static FileLayout Of(const ProtocolFile& file)
    {
        return FileLayout{!file.enums.empty(), !file.structs.empty(), !file.packets.empty()};
    }
};

/// Generates C++ code for all protocol files.
///
/// For each protocol file under namespace <c>ns</c> (e.g. "net/client"), the following files are generated:
/// - include/eolib/protocol/ns/enums.hpp and src/eolib/protocol/ns/enums.cpp (if the file defines enums)
/// - include/eolib/protocol/ns/structs.hpp and src/eolib/protocol/ns/structs.cpp (if the file defines structs)
/// - include/eolib/protocol/ns/packets.hpp, packet_factory.hpp and matching sources (if the file defines packets)
/// - include/eolib/protocol/ns.hpp: an umbrella header (except for the root file)
///
/// Additionally, include/eolib/protocol.hpp includes everything.
class ProtocolGenerator
{
public:
    ProtocolGenerator(const std::vector<ProtocolFile>& files, TypeRegistry& types);

    /// Generates the code for all protocol files.
    /// @throws GeneratorError if any protocol file is invalid.
    std::vector<OutputFile> Generate();

private:
    /// A struct or packet class to generate.
    struct ObjectDefinition
    {
        std::string class_name;
        const std::optional<std::string>& comment;
        const std::vector<Instruction>& instructions;
        std::optional<PacketInfo> packet;
    };

    const std::vector<ProtocolFile>& files_;
    TypeRegistry& types_;

    void GenerateStructs(const ProtocolFile& file, std::vector<OutputFile>& result);
    void GeneratePackets(const ProtocolFile& file, std::vector<OutputFile>& result);

    /// Generates the header and source for the struct or packet classes of a file. name is the base name of the
    /// files, e.g. "structs", and includes are the local includes of the header (in addition to the headers that
    /// define the custom types referenced by the classes).
    void GenerateObjectFiles(const ProtocolFile& file, const std::string& name, std::set<std::string> includes,
                             const std::vector<ObjectDefinition>& objects, std::vector<OutputFile>& result);

    /// Sorts structs so that each struct is defined after the structs it contains (from the same file).
    /// @throws GeneratorError if structs have a circular dependency.
    std::vector<const ProtocolStruct*> SortStructs(const ProtocolFile& file);

    void ValidatePacketIds(const ProtocolFile& file);

    void GenerateUmbrella(const ProtocolFile& file, std::vector<OutputFile>& result) const;
    void GenerateRootUmbrella(std::vector<OutputFile>& result) const;

    /// Gets the include paths of the headers generated for a file.
    static std::vector<std::string> FileHeaders(const ProtocolFile& file);

    static OutputFile UmbrellaHeader(const std::string& path, std::vector<std::string> headers);
};

} // namespace eolib::generator
