#pragma once

#include "generated_file.hpp"
#include "model.hpp"
#include "types.hpp"

#include <vector>

namespace eolib::generator
{

/// Generates the PacketFactory class for the packets of a client or server protocol file, which creates packets from
/// their family and action.
class PacketFactoryGenerator
{
public:
    explicit PacketFactoryGenerator(TypeRegistry& types);

    /// Generates packet_factory.hpp and packet_factory.cpp for a protocol file, and adds them to result.
    /// @throws GeneratorError if the file is not a client or server protocol file.
    void Generate(const ProtocolFile& file, std::vector<OutputFile>& result);

private:
    TypeRegistry& types_;

    static void WriteDeclaration(const std::string& side, CodeWriter& header);
};

} // namespace eolib::generator
