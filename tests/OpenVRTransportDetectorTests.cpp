#include <catch2/catch_test_macros.hpp>

#include "OpenVRTransportDetector.h"

TEST_CASE("OpenVR transport is unknown when its property cannot be read")
{
    CHECK(OpenVRTransportDetector::DetermineTransport(false, false) ==
          OpenVRTransportDetector::Transport::kUnknown);
    CHECK(OpenVRTransportDetector::DetermineTransport(false, true) ==
          OpenVRTransportDetector::Transport::kUnknown);
}

TEST_CASE("OpenVR transport follows a successfully read wireless property")
{
    CHECK(OpenVRTransportDetector::DetermineTransport(true, false) ==
          OpenVRTransportDetector::Transport::kWired);
    CHECK(OpenVRTransportDetector::DetermineTransport(true, true) ==
          OpenVRTransportDetector::Transport::kWireless);
}
