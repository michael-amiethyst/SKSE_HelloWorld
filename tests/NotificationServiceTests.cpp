#include <catch2/catch_test_macros.hpp>

#include "NotificationService.h"

#include <string_view>

TEST_CASE("NotificationService selects the wired transport message")
{
    CHECK(std::string_view(NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWired)) ==
          "Zenithar's courier travels through the earth.");
}

TEST_CASE("NotificationService selects the wireless transport message")
{
    CHECK(std::string_view(NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWireless)) ==
          "Zenithar's courier travels through the air.");
}

TEST_CASE("NotificationService selects the fallback transport message")
{
    CHECK(std::string_view(NotificationService::GetMessage_(HeadMountedDisplay::Transport::kUnknown)) ==
          "Zenithar's courier is confused");
}
