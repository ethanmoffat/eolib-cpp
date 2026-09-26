#pragma once

#include "emitter.hpp"
#include "model.hpp"
#include "types.hpp"

#include <vector>

namespace eolib::generator
{

/// Generates test support code that builds packets from the property trees in eo-captured-packets JSON files, so the
/// tests can compare deserialized field values and serialize packets that were not produced by deserialization.
///
/// The generated file, "captured_packet_properties.cpp", implements the functions declared in the eolib tests'
/// "protocol/captured_packet_properties.hpp" header, and depends on nlohmann/json.
std::vector<OutputFile> GenerateCapturedPacketProperties(const std::vector<ProtocolFile>& files, TypeRegistry& types);

} // namespace eolib::generator
