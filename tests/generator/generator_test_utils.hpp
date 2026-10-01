#pragma once

#include "errors.hpp"
#include "model.hpp"
#include "protocol_generator.hpp"
#include "protocol_reader.hpp"
#include "types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace eolib::generator::test
{

/// A temporary directory of protocol.xml files, removed on destruction.
class ProtocolFixture
{
public:
    /// A protocol.xml file, identified by its directory relative to the fixture root (e.g. "net/client").
    using File = std::pair<std::string, std::string>;

    explicit ProtocolFixture(const std::string& protocol_body)
        : ProtocolFixture(std::vector<File>{{"", protocol_body}})
    {
    }

    explicit ProtocolFixture(const std::vector<File>& files)
    {
        static std::atomic<int> counter{0};
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir_ = std::filesystem::temp_directory_path() /
               ("eolib_generator_test_" + std::string(info != nullptr ? info->name() : "fixture") + "_" +
                std::to_string(counter++));
        std::filesystem::remove_all(dir_);
        for (const auto& [relative_dir, protocol_body] : files)
        {
            const auto file_dir = dir_ / relative_dir;
            std::filesystem::create_directories(file_dir);
            std::ofstream stream(file_dir / "protocol.xml", std::ios::binary);
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<protocol>\n" << protocol_body << "\n</protocol>\n";
        }
    }

    ~ProtocolFixture()
    {
        std::error_code ignored;
        std::filesystem::remove_all(dir_, ignored);
    }

    ProtocolFixture(const ProtocolFixture&) = delete;
    ProtocolFixture& operator=(const ProtocolFixture&) = delete;

    std::vector<OutputFile> Generate() const
    {
        const auto files = ProtocolReader::ReadAll(dir_);
        TypeRegistry types(files);
        return ProtocolGenerator(files, types).Generate();
    }

private:
    std::filesystem::path dir_;
};

inline std::vector<OutputFile> Generate(const std::string& protocol_body)
{
    return ProtocolFixture(protocol_body).Generate();
}

inline const OutputFile* FindOutput(const std::vector<OutputFile>& outputs, const std::string& path)
{
    for (const auto& output : outputs)
    {
        if (output.path == path)
        {
            return &output;
        }
    }
    return nullptr;
}

} // namespace eolib::generator::test

/// Asserts that generating the protocol throws a GeneratorError with a message containing the specified text.
#define EXPECT_GENERATOR_ERROR(protocol, message)                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            ::eolib::generator::test::ProtocolFixture(protocol).Generate();                                            \
            ADD_FAILURE() << "Expected GeneratorError containing: " << (message);                                      \
        }                                                                                                              \
        catch (const ::eolib::generator::GeneratorError& e)                                                            \
        {                                                                                                              \
            EXPECT_NE(std::string(e.what()).find(message), std::string::npos) << "Actual message: " << e.what();       \
        }                                                                                                              \
    } while (false)
