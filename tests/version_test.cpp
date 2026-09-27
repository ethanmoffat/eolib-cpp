#include "eolib/version.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(VersionTest, GetVersionStringMatchesHeader)
{
    EXPECT_EQ(std::string(EOLIB_VERSION_STRING), std::string(eolib::GetVersionString()));
}

TEST(VersionTest, VersionStringStartsWithNumericVersion)
{
    std::string expected = std::to_string(EOLIB_VERSION_MAJOR) + "." + std::to_string(EOLIB_VERSION_MINOR) + "." +
                           std::to_string(EOLIB_VERSION_PATCH);
    EXPECT_EQ(0u, std::string(EOLIB_VERSION_STRING).rfind(expected, 0));
}
