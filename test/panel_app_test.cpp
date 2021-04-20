#include <panel_app.hpp>

#include <gtest/gtest.h>

TEST(PanelApp, display)
{
    // Good path.
    EXPECT_EQ(true, display("BLAH"));
}
