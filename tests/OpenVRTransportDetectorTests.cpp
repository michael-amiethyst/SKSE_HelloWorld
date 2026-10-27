#include <catch2/catch_test_macros.hpp>

#include "HeadMountedDisplay.h"

TEST_CASE("OpenVR transport is unknown when its property cannot be read")
{
    CHECK(HeadMountedDisplay::DetermineTransport_(false, false) ==
          HeadMountedDisplay::Transport::kUnknown);
    CHECK(HeadMountedDisplay::DetermineTransport_(false, true) ==
          HeadMountedDisplay::Transport::kUnknown);
}

TEST_CASE("OpenVR transport follows a successfully read wireless property")
{
    CHECK(HeadMountedDisplay::DetermineTransport_(true, false) ==
          HeadMountedDisplay::Transport::kWired);
    CHECK(HeadMountedDisplay::DetermineTransport_(true, true) ==
          HeadMountedDisplay::Transport::kWireless);
}
