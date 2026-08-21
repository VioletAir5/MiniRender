#include "core/AppInfo.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("application metadata is available") {
    REQUIRE(renderlab::applicationName() == "RenderLab");
    REQUIRE_FALSE(renderlab::applicationVersion().empty());
}

