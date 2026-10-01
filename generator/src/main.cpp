#include "errors.hpp"
#include "generated_file.hpp"
#include "property_generator.hpp"
#include "protocol_generator.hpp"
#include "protocol_reader.hpp"
#include "types.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace eolib::generator;

constexpr const char* USAGE =
    "Usage: eolib-protocol-gen --input <xml dir> --output <dir> [--stamp <file>] [--mode <mode>]\n"
    "\n"
    "Generates C++ code for the eo-protocol XML files found under <xml dir>. Files are only rewritten if their\n"
    "content changes. If specified, <file> is touched on success.\n"
    "\n"
    "Modes:\n"
    "  protocol          (default) the protocol code. Headers are written to <dir>/include and sources to\n"
    "                    <dir>/src.\n"
    "  test-properties   test support code for the eolib tests, which builds packets from the properties in\n"
    "                    eo-captured-packets files. Written to <dir>/captured_packet_properties.cpp.\n";

int main(int argc, char* argv[])
{
    fs::path input;
    fs::path output;
    fs::path stamp;
    std::string mode = "protocol";

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            std::cout << USAGE;
            return 0;
        }
        if (i + 1 >= argc)
        {
            std::cerr << "Missing value for " << arg << "\n\n" << USAGE;
            return 2;
        }
        if (arg == "--input")
        {
            input = argv[++i];
        }
        else if (arg == "--output")
        {
            output = argv[++i];
        }
        else if (arg == "--stamp")
        {
            stamp = argv[++i];
        }
        else if (arg == "--mode")
        {
            mode = argv[++i];
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n\n" << USAGE;
            return 2;
        }
    }

    if (input.empty() || output.empty() || (mode != "protocol" && mode != "test-properties"))
    {
        std::cerr << USAGE;
        return 2;
    }

    try
    {
        const auto files = ProtocolReader::ReadAll(input);
        TypeRegistry types(files);
        const auto outputs = mode == "protocol" ? ProtocolGenerator(files, types).Generate()
                                                : PropertyGenerator(files, types).Generate();

        int written = 0;
        for (const auto& file : outputs)
        {
            if (file.WriteIfChanged(output))
            {
                ++written;
            }
        }

        if (!stamp.empty())
        {
            fs::create_directories(stamp.parent_path());
            std::ofstream stream(stamp, std::ios::trunc);
            stream << outputs.size() << " files\n";
            if (!stream)
            {
                throw std::runtime_error("Failed to write " + stamp.string() + ".");
            }
        }

        std::cout << "eolib-protocol-gen: generated " << outputs.size() << " files (" << written << " updated)\n";
        return 0;
    }
    catch (const GeneratorError& e)
    {
        std::cerr << "eolib-protocol-gen: error: " << e.what() << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "eolib-protocol-gen: unexpected error: " << e.what() << "\n";
    }
    return 1;
}
