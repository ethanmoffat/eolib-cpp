#pragma once

#include "emitter.hpp"
#include "errors.hpp"
#include "model.hpp"
#include "types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace eolib::generator::test
{

/// A temporary directory containing a single protocol.xml file, removed on destruction.
class ProtocolFixture
{
public:
    explicit ProtocolFixture(const std::string& protocol_body)
    {
        static std::atomic<int> counter{0};
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir_ = std::filesystem::temp_directory_path() /
               ("eolib_generator_test_" + std::string(info != nullptr ? info->name() : "fixture") + "_" +
                std::to_string(counter++));
        std::filesystem::remove_all(dir_);
        std::filesystem::create_directories(dir_);
        std::ofstream stream(dir_ / "protocol.xml", std::ios::binary);
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<protocol>\n" << protocol_body << "\n</protocol>\n";
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
        const auto files = LoadProtocolFiles(dir_);
        TypeRegistry types(files);
        return GenerateProtocol(files, types);
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
#define EXPECT_GENERATOR_ERROR(protocol_body, message)                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            ::eolib::generator::test::Generate(protocol_body);                                                         \
            ADD_FAILURE() << "Expected GeneratorError containing: " << (message);                                      \
        }                                                                                                              \
        catch (const ::eolib::generator::GeneratorError& e)                                                            \
        {                                                                                                              \
            EXPECT_NE(std::string(e.what()).find(message), std::string::npos) << "Actual message: " << e.what();       \
        }                                                                                                              \
    } while (false)
