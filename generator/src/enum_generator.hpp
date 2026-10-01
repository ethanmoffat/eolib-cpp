#pragma once

#include "generated_file.hpp"
#include "model.hpp"
#include "types.hpp"

#include <vector>

namespace eolib::generator
{

/// Generates the enums of a protocol file, with a ToString function and a stream operator for each enum.
class EnumGenerator
{
public:
    explicit EnumGenerator(TypeRegistry& types);

    /// Generates enums.hpp and enums.cpp for a protocol file, and adds them to result.
    /// @throws GeneratorError if an enum is invalid.
    void Generate(const ProtocolFile& file, std::vector<OutputFile>& result);

private:
    TypeRegistry& types_;
};

} // namespace eolib::generator
