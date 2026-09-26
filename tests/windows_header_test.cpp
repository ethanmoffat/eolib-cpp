// Verifies that the public headers compile after <windows.h> without NOMINMAX or WIN32_LEAN_AND_MEAN.
#include <windows.h>

#include <eolib/eolib.hpp>
#include <eolib/protocol.hpp>

#include <gtest/gtest.h>

TEST(WindowsHeaderTest, PublicHeadersCompileAfterWindowsHeader)
{
    eolib::protocol::net::Weight weight;
    weight.max = 100;
    EXPECT_EQ(weight.max, 100);
}
