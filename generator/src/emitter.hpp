#pragma once

#include "model.hpp"
#include "types.hpp"

#include <string>
#include <vector>

namespace eolib::generator
{

struct OutputFile
{
    /// Path relative to the output root, e.g. "include/eolib/protocol/net/enums.hpp".
    std::string path;
    std::string content;
};

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
std::vector<OutputFile> GenerateProtocol(const std::vector<ProtocolFile>& files, TypeRegistry& types);

} // namespace eolib::generator
