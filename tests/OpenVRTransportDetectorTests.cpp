#include <catch2/catch_test_macros.hpp>

#include "OpenVRTransportDetector.h"

TEST_CASE("OpenVR transport is unknown when its property cannot be read")
{
    CHECK(OpenVRTransportDetector::DetermineTransport_(false, false) ==
          OpenVRTransportDetector::Transport::kUnknown);
    CHECK(OpenVRTransportDetector::DetermineTransport_(false, true) ==
          OpenVRTransportDetector::Transport::kUnknown);
}

TEST_CASE("OpenVR transport follows a successfully read wireless property")
{
    CHECK(OpenVRTransportDetector::DetermineTransport_(true, false) ==
          OpenVRTransportDetector::Transport::kWired);
    CHECK(OpenVRTransportDetector::DetermineTransport_(true, true) ==
          OpenVRTransportDetector::Transport::kWireless);
}
