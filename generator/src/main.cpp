#include "emitter.hpp"
#include "errors.hpp"
#include "model.hpp"
#include "property_emitter.hpp"
#include "types.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace eolib::generator;

namespace
{

constexpr const char* kUsage =
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

std::string ReadFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool WriteFileIfChanged(const fs::path& path, const std::string& content)
{
    if (fs::exists(path) && ReadFile(path) == content)
    {
        return false;
    }

    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        throw std::runtime_error("Failed to open " + path.string() + " for writing.");
    }
    stream << content;
    if (!stream)
    {
        throw std::runtime_error("Failed to write " + path.string() + ".");
    }
    return true;
}

} // namespace

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
            std::cout << kUsage;
            return 0;
        }
        if (i + 1 >= argc)
        {
            std::cerr << "Missing value for " << arg << "\n\n" << kUsage;
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
            std::cerr << "Unknown argument: " << arg << "\n\n" << kUsage;
            return 2;
        }
    }

    if (input.empty() || output.empty() || (mode != "protocol" && mode != "test-properties"))
    {
        std::cerr << kUsage;
        return 2;
    }

    try
    {
        const auto files = LoadProtocolFiles(input);
        TypeRegistry types(files);
        const auto outputs =
            mode == "protocol" ? GenerateProtocol(files, types) : GenerateCapturedPacketProperties(files, types);

        int written = 0;
        for (const auto& file : outputs)
        {
            if (WriteFileIfChanged(output / file.path, file.content))
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
